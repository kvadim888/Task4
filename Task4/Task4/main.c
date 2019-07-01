#define	_CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "getopt.h"
#include "libfix.h"

#define	MONO	1
#define STEREO	2

#define SIGNAL_AMPLITUDE	(1U << 0)
#define SIGNAL_FREQUENCY	(1U << 1)
#define SIGNAL_PHASE		(1U << 2)
#define SIGNAL_RATE			(1U << 3)

#define TYPE_TONE			(1U << 0)
#define TYPE_LINSWEEP		(1U << 1)
#define TYPE_EXPSWEEP		(1U << 2)
#define TYPE_NOISE			(1U << 3)

/*========== WAV FILE HANDLING ===============================================*/

uint16_t *g_buffIO; // main IO buffer

#pragma pack(push, 1)
// WAVE file header format

typedef struct
{
	char		riff[4];				// "RIFF"
	uint32_t	overall_size;			// filesize
	char		wave[4];				// "WAVE"
	char		fmt_chunk_marker[4];	// "fmt "string with trailing null char
	uint32_t	length_of_fmt;			// length of the format data
	uint16_t	format_type;			// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	uint16_t	channels;				// channels' amount
	uint32_t	sample_rate;			// sampling rate (blocks per second)
	uint32_t	byterate;				// SampleRate * NumChannels * BitsPerSample/8
	uint16_t	block_align;			// NumChannels * BitsPerSample/8
	uint16_t	bits_per_sample;		// bits per sample, 8- 8bits, 16- 16 bits etc
	char		data_chunk_header[4];	// "DATA" or "FLLR"
	uint32_t	data_size;				// NumSamples * NumChannels * BitsPerSample/8 
										//				- size of the next chunk that will be read
}				t_wavheader;

typedef struct
{
	uint16_t	**data;
	uint8_t		channels;
	size_t		datalen;
	size_t		samplen;
}				t_wavbuffer;

typedef	struct
{
	FILE		*fs;
	t_wavheader header;
	t_wavbuffer	*buffer;
}				t_wavfile;

#pragma pack(pop)

t_wavfile	*wav_wropen(const char *path, t_wavheader *header, t_wavbuffer *buffer)
{
	t_wavfile *wavfile;

	wavfile = malloc(sizeof(t_wavfile));
	if ((wavfile->fs = fopen(path, "wb")) == NULL)
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
	assert(buffer);
	wavfile->buffer = buffer;
	return (wavfile);
}

uint16_t	*wav_getbuffIO(size_t len)
{
	if (g_buffIO == NULL)
		g_buffIO = calloc(len, sizeof(uint16_t));
	return (g_buffIO);
}

size_t		wav_rwbuffmerge(uint16_t *buffIO, t_wavbuffer *buffer)
{
	size_t		block_align = buffer->channels;
	uint16_t	*ptr;

	for (size_t j = 0; j < buffer->channels; j++)
	{
		ptr = buffIO;
		for (size_t i = 0; i < buffer->datalen; i+=buffer->samplen)
		{
			memcpy(ptr + j, &buffer->data[j][i], buffer->samplen);
			ptr += block_align;
		}
	}
	return block_align * buffer->datalen;
}

size_t		wav_write(t_wavfile *file, size_t datalen)
{
	if (file == NULL || file->fs == NULL)
		return 0;
	g_buffIO = wav_getbuffIO(file->header.block_align * file->buffer->datalen);
	memset(g_buffIO, 0, file->header.block_align * file->buffer->datalen);
	size_t length = wav_rwbuffmerge(g_buffIO, file->buffer);
	size_t len = fwrite(g_buffIO, sizeof(int16_t), length, file->fs);
	return length;
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

void		wav_close(t_wavfile **wavfile)
{
	if (!wavfile && !*wavfile)
		return;
	fclose((*wavfile)->fs);
	printf("file closing\n");
	for (size_t i = 0; i < (*wavfile)->buffer->channels; i++)
		free((*wavfile)->buffer->data[i]);
	free((*wavfile)->buffer->data);
	free(*wavfile);
	*wavfile = NULL;
}

/*========== SIGNAL GENERATORS ===============================================*/

int32_t		dsp_db2gain(double gain)
{
	double scale = pow(10, gain / 20);
	return float_to_fix(scale);
}

int16_t		signal_tone(double amplitude, double frequency, double phase, double time)
{
	t_sample	sample;

	sample.int32[1] = float_to_fix(amplitude * sin(2*M_PI * frequency * time + phase));
	return sample.int16[3];
}


int32_t		signal_linsweep(double amplitude, double freq1, double freq2, double phase,
							double duration, double time)
{
	t_sample	sample;
	double		k = (freq2 - freq1) / duration;

	sample.int32[1] = float_to_fix(amplitude *
		sin(phase + 2 * M_PI * time * (0.5 * k * time + freq1)));
	return sample.int16[3];
}

int32_t		signal_expsweep(double amplitude, double freq1, double freq2, double phase,
							double duration, double time)
{
	t_sample	sample;
	double		k = pow(freq2 / freq1, 1 / duration);

	sample.int32[1] = float_to_fix(amplitude *
			sin(phase + 2*M_PI * freq1 * ((pow(k, time) - 1) / log(k))));
	return sample.int16[3];
}

int32_t		signal_noise(double amplitude)
{
	t_sample	sample;

	sample.int32[1] = float_to_fix(amplitude) * (0.5 - rand());
	return (int16_t)sample.int16[3];
}

/*============================================================================*/

/*============  RING BUFFER  =================================================*/

typedef struct	s_ringbuff
{
	size_t		datalen;	// lenght of data in samples
	size_t		samplen;	// size of sample in bytes
	uint16_t	*buff;		// head of buffer
	uint16_t	carriage;		// carriage for moving on buffer
}				t_ringbuff;

/*============================================================================*/

/*===========  FILTER FUNCTIONS  =============================================*/

size_t		dsp_FIRimport(int32_t **buff, const char * path)
{
	size_t		datalen;
	FILE		*file;
	char		str[100];

	if ((file = fopen(path, "r")) == NULL)
	{
		printf("%s: Unable to open file for reading\n", path);
		exit(1);
	}
	printf("\nfilename: %s\n", path);
	if (buff == NULL)
		return 0;
	while (fgets(str, 90, file))
	{
		if (str[0] == '%')
			printf("%s", str);
		if (strncmp(str, "% Filter Length", 15) == 0)
			datalen = atoi(str+22);
		if (strncmp(str, "Numerator:", 10) == 0)
			break;
		memset(str, 0, 100);
	}
	*buff = malloc(datalen * sizeof(int32_t));
	double *coefs = malloc(datalen * sizeof(double));
	double norm = 0;
	memset(str, 0, 100);
	for (size_t i = 0; i < datalen; i++)
	{
		fgets(str, 90, file);
		coefs[i] = atof(str);
		norm += coefs[i];
		memset(str, 0, 100);
	}
	for (size_t i = 0; i < datalen; i++)
	{
		coefs[i] /= norm;
		(*buff)[i] = float_to_fix(coefs[i]);
	}
	fclose(file);
	return datalen;
}

int32_t		dsp_FIR(t_ringbuff *input, int32_t *coeffs, size_t filterlen)
{
	uint64_t	result = 0;
	t_sample	sample;
		
	for (size_t i = 0; i < filterlen; i++)
	{
		sample.int16[3] = input->buff[(input->carriage - i + filterlen) & (filterlen - 1)];
		fix_mac(&result, sample.int32[1], coeffs[i]);
	}
	return fix_round(result);
}

/*============================================================================*/

int main(int ac, char **av)
{
	/* flags -----------------------------------------*/
	uint8_t		type_flag = 0;		// type of generator
	uint8_t		duration_flag = 0;	// duration of signal	
	uint8_t		signal_flag = 0;	// flag of signal settings (start)
	uint8_t		end_flag = 0;		// flag of signal settings (end)

	/* signal characteristics ------------------------*/
	double	duration = 10;			//	default 10 sec
	double	amplitude = 1;			//	default 1
	double	freq1 = 100;			//	default 100 Hz
	double	freq2 = freq1;			//	default freq1 == freq2
	double	phase = 0;					//	default 0
	double	samplerate = 41300;	// more than Nyquist frequency

	/* properties of files ---------------------------*/
	char		*filepath = NULL;	// name of file with generated signal
	int32_t		*FIR = NULL;		// FIR filter structure
	size_t		filterlen = 0;

	/* parsing CLI arguments -------------------------*/
	if (ac < 2)
	{
		printf("Invalid number of args\n");
		exit(1);
	}

	int opt_index = 0;
	while ((opt_index = getopt(ac, av, "n:w:t:g:a:f:p:r:")) != -1)
	{
		switch (opt_index)
		{
		case 'n':
			filepath = optarg;
			break;
		case 'w':
			filterlen = dsp_FIRimport(&FIR, optarg);
			assert(filterlen);
			break;
		case 'g':
			if (type_flag)
				break;
			if (strcmp(optarg, "tone") == 0)
				type_flag |= TYPE_TONE;
			else if (strcmp(optarg, "linear") == 0)
				type_flag |= TYPE_LINSWEEP;
			else if (strcmp(optarg, "exp") == 0)
				type_flag |= TYPE_EXPSWEEP;
			else if (strcmp(optarg, "noise") == 0)
				type_flag |= TYPE_NOISE;
			else
			{
				printf("%s: Invalid type of generator\n", optarg);
				exit(1);
			}
			break;
		case 'a':
			if ((signal_flag & SIGNAL_AMPLITUDE) == 0)
			{
				if (amplitude > 0)
				{
					printf("signal.amplitude > 0 dB\n");
					exit(1);
				}
				amplitude = dsp_db2gain(atof(optarg));
				signal_flag |= SIGNAL_AMPLITUDE;
			}
			else
			{
				if (amplitude > 0)
				{
					printf("end.amplitude > 0 dB\n");
					exit(1);
				}
				amplitude = dsp_db2gain(atof(optarg));
				end_flag |= SIGNAL_AMPLITUDE;
			}
			break;
		case 'f':
			if ((signal_flag & SIGNAL_FREQUENCY) == 0)
			{
				freq1 = atof(optarg);
				if (freq1 < 0)
				{
					printf("freq1 < 0\n");
					exit(1);
				}
				signal_flag |= SIGNAL_FREQUENCY;
				samplerate = (samplerate < 2 * freq1) ? 2.01 * freq1 : samplerate;
			}
			else
			{
				freq2 = atof(optarg);
				if (freq2 < 0)
				{
					printf("freq2 < 0\n");
					exit(1);
				}
				end_flag |= SIGNAL_FREQUENCY;
				samplerate = (samplerate < 2 * freq2) ? 2.01 * freq2 : samplerate;
			}
			break;
		case 'p':
			if ((signal_flag & SIGNAL_PHASE) == 0)
			{
				phase = atof(optarg);
				signal_flag |= SIGNAL_PHASE;
			}
			else
			{
				phase = atof(optarg);
				end_flag|= SIGNAL_PHASE;
			}
			break;
		case 'r':
			if ((signal_flag & SIGNAL_RATE) == 0)
			{
				samplerate = (samplerate < atof(optarg)) ? atof(optarg) : samplerate;
				samplerate = (samplerate < 2 * freq1) ? 2.01 * freq1 : samplerate;
				samplerate = (samplerate < 2 * freq2) ? 2.01 * freq2 : samplerate;
				signal_flag |= SIGNAL_RATE;
			}
			break;
		case 't':
			duration = atof(optarg);
			if (duration <= 0)
			{
				printf("duration <= 0\n");
				exit(1);
			}
			break;
		case '?':
			printf("unknown option : %c", opt_index);
			exit(1);
			break;
		default:
			printf("optarg = %s\n", optarg);
			filepath = optarg;
			break;
		}
	}

	assert(FIR);

	/* WAV-file initialisation -----------------------*/
	size_t length = round(duration * samplerate);
	t_wavbuffer wavbuffer = 
	{
		.channels = STEREO,
		.samplen = sizeof(int16_t),
		.datalen = length,
		.data = NULL
	};
	wavbuffer.data = malloc(wavbuffer.channels * sizeof(int16_t*));
	wavbuffer.data[0] = calloc(length, sizeof(wavbuffer.samplen));
	wavbuffer.data[1] = calloc(length, sizeof(wavbuffer.samplen));
	memset(wavbuffer.data[0], 0, length * sizeof(wavbuffer.samplen));
	memset(wavbuffer.data[1], 0, length * sizeof(wavbuffer.samplen));

	t_wavheader	header =
	{
		.riff = {'R','I','F','F'},
		.overall_size = 36,				// TODO rewrite later after estimating filesize
		.wave = {'W','A','V','E'},
		.fmt_chunk_marker = {'f','m','t',' '},
		.length_of_fmt = 16,
		.format_type = 1,
		.channels = wavbuffer.channels,
		.sample_rate = round(samplerate), 
		.byterate =	0,				
		.block_align = 0,
		.bits_per_sample = 16,
		.data_chunk_header = {'d','a','t','a'},	
		.data_size = wavbuffer.datalen * wavbuffer.samplen,
	};
	header.block_align = header.channels * header.bits_per_sample / 8;
	header.byterate = header.sample_rate * header.block_align;
	header.overall_size += header.data_size;

	t_wavfile *wavfile = wav_wropen(filepath, &header, &wavbuffer);

	/* ring buffer initialisation --------------------*/
	t_ringbuff ring =
	{
		.datalen = wavbuffer.datalen,
		.samplen = wavbuffer.samplen,
		.buff = wavbuffer.data[0],
		.carriage = 0 
	};

	for (size_t i = 0; i < length; i++)
	{
		switch (type_flag)
		{
		case TYPE_TONE:
			ring.buff[ring.carriage] = signal_tone(amplitude, freq1, phase, i / samplerate);
			break;
		case TYPE_LINSWEEP:
			ring.buff[ring.carriage] = signal_linsweep(amplitude, freq1, freq2, phase, duration, i / samplerate);
			break;
		case TYPE_EXPSWEEP:
			ring.buff[ring.carriage] = signal_expsweep(amplitude, freq1, freq2, phase, duration, i / samplerate);
			break;
		case TYPE_NOISE:
			ring.buff[ring.carriage] = signal_noise(amplitude);
			break;
		default:
			ring.buff[ring.carriage] = signal_tone(amplitude, freq1, phase, i / samplerate);
			break;
		}
		wavbuffer.data[0][i] = ring.buff[ring.carriage];
		//wavbuffer.data[1][i] = ring.buff[ring.carriage];
		t_sample sample;
		sample.int32[1] = dsp_FIR(&ring, FIR, filterlen);
		wavbuffer.data[1][i] = sample.int16[3];
		ring.carriage = (ring.carriage + 1) & (filterlen - 1);
	}

	wav_write(wavfile, wavbuffer.datalen);

	wav_info(filepath, &header);

	wav_close(&wavfile);

	return 0;
}
