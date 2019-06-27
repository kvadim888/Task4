#include <stdio.h>

#include "getopt.h"
#include "libfix.h"
#include "libsignal.h"
#include "libwav.h"

int main(int ac, char **av)
{
	int opt_index = 0;
	int	time = 10;
	int channels = 1;
	char *filepath = NULL;

	if (ac < 2)
	{
		fprintf(stderr, "Invalid number of args\n");
		exit(1);
	}

	t_signal	signal = 
	{
		.amplitude = 1,
		.frequency = 100,
		.phase = 0,
		.samplerate = 2.1 * signal.frequency 
	};

	while ((opt_index = getopt(ac, av, "a:f:p:r:n:t:c:")) != -1)
	{
		switch (opt_index)
		{
		case 'a':
			signal.amplitude = atof(optarg);
			if (signal.amplitude > 1)
			{
				fprintf(stderr, "signal.amplitude > 1\n");
				exit(1);
			}
			break;
		case 'f':
			signal.frequency = atof(optarg);
			if (signal.frequency < 0)
			{
				fprintf(stderr, "signal.frequency < 0\n");
				exit(1);
			}
			break;
		case 'p':
			signal.phase = atof(optarg);
			break;
		case 'r':
			signal.samplerate = atof(optarg);
			break;
		case 'n':
			filepath = optarg;
			break;
		case 't':
			time = atof(optarg);
			if (time <= 0)
			{
				fprintf(stderr, "duration <= 0\n");
				exit(1);
			}
			break;
		case 'c':
			channels = atoi(optarg);
			if (channels <= 0)
			{
				fprintf(stderr, "channels <= 0\n");
				exit(1);
			}
			break;
		case '?':
			fprintf(stderr, "unknown option : %c", opt_index);
			exit(1);
			break;
		default:
			filepath = optarg;
			break;
		}
	}

	t_sigbuff *sigbuff = signal_white(&signal, time);

	t_wavbuffer wavbuffer =
	{
		.channels = channels,
		.samplen = sizeof(uint16_t),
		.datalen = sigbuff->len,
		.data = malloc(channels * sizeof(uint8_t*))
	};

	for (int i = 0; i < channels; i++)
		wavbuffer.data[i] = sigbuff->buff;

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
		.sample_rate = signal.samplerate, 
		.byterate =	0,				
		.block_align = 0,
		.bits_per_sample = 16,
		.data_chunk_header = {'d','a','t','a'},	
		.data_size = wavbuffer.datalen * wavbuffer.samplen,
	};

	header.byterate = header.sample_rate * header.channels * header.bits_per_sample / 8;
	header.block_align = header.channels * header.bits_per_sample;
	header.overall_size += header.data_size;

	t_wavfile *file = wav_wropen(filepath, &header, &wavbuffer);

	wav_write(file, wavbuffer.datalen);

	wav_info(filepath, &header);

	wav_close(&file);

	return 0;
}