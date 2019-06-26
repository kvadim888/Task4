#include <stdio.h>

#include "libfix.h"
#include "libsignal.h"
#include "libwav.h"

int main(int ac, char **av)
{
	if (ac != 2)
	{
		printf("Invalid number of args\n");
		exit(1);
	}

	t_signal	sinus = 
	{
		.amplitude = 0.5,
		.frequency = 2000,
		.phase = 0.5,
		.samplerate = 2.1 * sinus.frequency 
	};


	t_sigbuff *sigbuff = signal_white(&sinus, 10);

	t_wavbuffer wavbuffer =
	{
		.channels = 1,
		.samplen = sizeof(uint16_t),
		.datalen = sigbuff->len,
		.data = malloc(sizeof(uint8_t*))
	};

	wavbuffer.data[0] = sigbuff->buff;

	t_wavheader header;
	memset(&header, 0, sizeof(t_wavheader));
	header = (t_wavheader){
		.riff = {'R','I','F','F'},
		.overall_size = 36,				// TODO rewrite later after estimating filesize
		.wave = {'W','A','V','E'},
		.fmt_chunk_marker = {'f','m','t',' '},
		.length_of_fmt = 16,
		.format_type = 1,
		.channels = wavbuffer.channels,
		.sample_rate = sinus.samplerate, 
		.byterate =	0,				
		.block_align = 0,
		.bits_per_sample = 16,
		.data_chunk_header = {'d','a','t','a'},	
		.data_size = wavbuffer.datalen * wavbuffer.samplen,
	};

	header.byterate = header.sample_rate * header.channels * header.bits_per_sample / 8;
	header.block_align = header.channels * header.bits_per_sample;
	header.overall_size += header.data_size;

	t_wavfile *file = wav_wropen(av[1], &header, &wavbuffer);

	wav_write(file, wavbuffer.datalen);

	wav_info(av[1], &header);

	wav_close(&file);

	return 0;
}