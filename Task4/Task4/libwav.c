#include "libwav.h"

uint8_t		*wav_getbuffIO(size_t len)
{
	//uint8_t *rwbuff = calloc(len, sizeof(uint8_t));
	if (g_buffIO == NULL)
		g_buffIO = calloc(len, sizeof(uint8_t));
	return (g_buffIO);
}

int			wav_initbuff(t_wavbuffer * buffer, t_wavheader * header, size_t datalen)
{
	if (buffer == NULL || header == NULL)
		return 1;
	buffer->datalen = datalen;
	buffer->channels = header->channels;
	buffer->samplen = header->block_align / header->channels;
	buffer->data = malloc(buffer->channels * sizeof(uint8_t*));
	for (size_t i = 0; i < buffer->channels; i++)
		buffer->data[i] = calloc(buffer->datalen, buffer->samplen);
	return 0;
}

t_wavfile	*wav_rdopen(const char *path, t_wavbuffer *buffer)
{
	t_wavfile *wavfile;

	wavfile = malloc(sizeof(t_wavfile));
	if (fopen_s(&wavfile->fs, path, "rb"))
	{
		printf("%s: Unable to open file for reading\n", path);
		exit(1);
	}
	if (fread(&wavfile->header, sizeof(uint8_t), 44, wavfile->fs) < 44)
	{
		printf("%s: Invalid file\n", path);
		exit(1);
	}
	wav_info(path, &wavfile->header);
	if (buffer == NULL)
	{
		wavfile->buffer = malloc(sizeof(t_wavbuffer));
		wav_initbuff(wavfile->buffer, &wavfile->header, 100);
	}
	else
		wavfile->buffer = buffer;
	return (wavfile);
}

t_wavfile	*wav_wropen(const char *path, t_wavheader *header, t_wavbuffer *buffer)
{
	t_wavfile *wavfile;

	wavfile = malloc(sizeof(t_wavfile));
	if (fopen_s(&wavfile->fs, path, "wb"))
	{
		printf("%s: Unable to open file for writing\n", path);
		exit(1);
	}
	memcpy(&wavfile->header, header, sizeof(t_wavheader));
	if (fwrite(&wavfile->header, sizeof(uint8_t), 44, wavfile->fs) < 44)
	{
		printf("%s: Writing error\n", path);
		exit(1);
	}
	wav_info(path, &wavfile->header);
	if (buffer == NULL)
	{
		wavfile->buffer = malloc(sizeof(t_wavbuffer));
		wav_initbuff(wavfile->buffer, &wavfile->header, 100);
	}
	else
		wavfile->buffer = buffer;
	return (wavfile);
}

size_t		wav_read(t_wavfile * file)
{
	if (file == NULL || file->fs == NULL)
		return 0;
	g_buffIO = wav_getbuffIO(file->header.block_align * file->buffer->datalen);
	size_t len = fread(g_buffIO, file->header.block_align, file->buffer->datalen, file->fs);
	size_t datalen = wav_rwbuffsplit(g_buffIO, file->buffer, len * file->header.block_align);
	return datalen;
}

size_t		wav_write(t_wavfile *file, size_t datalen)
{
	if (file == NULL || file->fs == NULL)
		return 0;
	g_buffIO = wav_getbuffIO(file->header.block_align * file->buffer->datalen);
	memset(g_buffIO, 0, file->header.block_align * file->buffer->datalen);
	wav_rwbuffmerge(g_buffIO, file->buffer);
	size_t len = fwrite(g_buffIO, file->header.block_align, datalen, file->fs);
	return len / file->header.block_align;
}

size_t		wav_rwbuffsplit(uint8_t *buffIO, t_wavbuffer *buffer, size_t len)
{
	size_t		block_align = buffer->channels * buffer->samplen;
	uint8_t		*ptr;

	/* for dubugging */
	//printf("before:\n");
	//log_memory(buffIO, len);
	for (size_t j = 0; j < buffer->channels; j++)
	{
		ptr = buffIO + j * buffer->samplen;
		memset(buffer->data[j], 0, buffer->samplen * buffer->datalen);
		for (size_t i = 0; i < buffer->datalen * buffer->samplen; i+=buffer->samplen)
		{
			memcpy(&buffer->data[j][i], ptr, buffer->samplen);
			ptr += block_align;
		}
	}
	return len / block_align;
}

size_t		wav_rwbuffmerge(uint8_t *buffIO, t_wavbuffer *buffer)
{
	size_t		block_align = buffer->channels * buffer->samplen;
	uint8_t		*ptr;

	for (size_t j = 0; j < buffer->channels; j++)
	{
		ptr = buffIO + j * buffer->samplen;
		for (size_t i = 0; i < buffer->datalen * buffer->samplen; i+=buffer->samplen)
		{
			memcpy(ptr, &buffer->data[j][i], buffer->samplen);
			ptr += block_align;
		}
	}
	/* for dubugging */
	//printf("after:\n");
	//log_memory(buffIO, block_align * buffer->datalen);
	return block_align * buffer->datalen;
}

void		wav_close(t_wavfile **wavfile)
{
	if (!wavfile && !*wavfile)
		return;
	fclose((*wavfile)->fs);
	printf("file closing\n");
	free(*wavfile);
	*wavfile = NULL;
}

void		wav_info(const char *filename, t_wavheader *header)
{
	if (filename == NULL || header == NULL)
		return;
	printf("\nfilename: %s\n", filename);

	printf("riff:\t%.4s \n", header->riff); 
	printf("overall_size:\t%u b (%u Kb)\n", header->overall_size, header->overall_size / 1024); 
	printf("wave:\t%.4s \n", header->wave); 
	printf("fmt_chunk_marker:\t%.4s \n", header->fmt_chunk_marker); 
	printf("length_of_fmt:\t%u\n", header->length_of_fmt); 
	printf("format_type:\t%u\n", header->format_type); 
	printf("channels:\t%u\n", header->channels); 
	printf("sample_rate:\t%u\n", header->sample_rate); 
	printf("byterate:\t%u\n", header->byterate); 
	printf("block_align:\t%u\n", header->block_align); 
	printf("bits_per_sample:\t%u\n", header->bits_per_sample); 
	printf("data_chunk_header:\t%.4s \n", header->data_chunk_header); 
	printf("data_size:\t%u\n", header->data_size); 

	long num_samples = (8 * header->data_size) / (header->channels * header->bits_per_sample);
	printf("Number of samples:\t%lu \n", num_samples);

	long size_of_each_sample = (header->channels * header->bits_per_sample) / 8;
	printf("Size of each sample:\t%ld bytes\n", size_of_each_sample);

	float duration_in_seconds = (float) header->overall_size / header->byterate;
	printf("duration = %f sec\n", duration_in_seconds);
}

uint16_t	swap_uint16(uint16_t val)
{
    return (val << 8) | (val >> 8 );
}

int16_t		swap_int16(int16_t val) 
{
    return (val << 8) | ((val >> 8) & 0xFF);
}

uint32_t	swap_uint32(uint32_t val)
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

int32_t		swap_int32(int32_t val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

void		log_memory(uint8_t *memory, size_t len)
{
	size_t width = 32;
	for (size_t i = 0; i < len; i++)
	{
		if (i % width == 0)
			printf("%.3p : ", (uint8_t *)(memory + i));
		printf("%.2x ", memory[i]);
		if ((i+1)  % width == 0)
			printf("\n");
	}
	printf("\n");
}
