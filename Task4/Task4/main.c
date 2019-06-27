#include <stdio.h>

#include "getopt.h"
#include "libfix.h"
#include "libsignal.h"
#include "libwav.h"

#define	MONO	1
#define STEREO	2

#pragma pack(push, 1)

typedef struct
{
	uint8_t		amplitude:1;
	uint8_t		frequency:1;
	uint8_t		phase:1;
	uint8_t		samplerate:1;
}				t_sigflag;

#pragma pack(pop)

int main(int ac, char **av)
{
/* flags =====================================================================*/
	uint8_t		type_flag = 0;
	uint8_t		duration_flag = 0;
	t_sigflag	signal_flag;
	memset(&signal_flag, 0, sizeof(t_sigflag));
	t_sigflag	end_flag;
	memset(&end_flag, 0, sizeof(t_sigflag));
/*============================================================================*/

/* signal characteristics ====================================================*/
	double	duration = 10; // default 10 sec
	t_signal	signal = 
	{
		.amplitude = 1,
		.frequency = 100,
		.phase = 0,
		.samplerate = 2.1 * signal.frequency 
	};
	t_signal	end;
	memcpy(&end, &signal, sizeof(t_signal));
/*============================================================================*/

	if (ac < 2)
	{
		printf("Invalid number of args\n");
		exit(1);
	}


	char *filepath = NULL;
	int opt_index = 0;
	int channels = 1;
	t_sigbuff *sigbuff = NULL;

	static struct option long_option[] =
	{
		{"filename", required_argument, 0, 'n'},
		{"duration", required_argument, 0, 't'},
		/* num of chanels */
		//{"channel", required_argument, 0, 'c'},
		/* each signal characteristics */
		{"type", required_argument, 0, 'g'},
		{"amplitude", required_argument, 0, 'a'},
		{"frequency", required_argument, 0, 'f'},
		{"phase", required_argument, 0, 'p'},
		{"rate", required_argument, 0, 'r'},
		{0, 0, 0, 0}
	};

	printf("getopt start\n");
	while ((opt_index = getopt_long(ac, av, "n:t:g:a:f:p:r:", long_option, NULL)) != -1)
	{
		switch (opt_index)
		{
		case 'n':
			filepath = optarg;
			break;
		case 'g':
			if (type_flag)
				break;
			type_flag = 1;
			char *type = optarg;
			if (strcmp(type, "sinus") == 0)
			{
				sigbuff = signal_tone(&signal, duration);
				break;
			}
			if (strcmp(type, "linear") == 0)
			{
				sigbuff = signal_linsweep(&signal, &end, duration);
				break;
			}
			if (strcmp(type, "exp") == 0)
			{
				sigbuff = signal_expsweep(&signal, &end, duration);
				break;
			}
			if (strcmp(type, "noise") == 0)
			{
				sigbuff = signal_white(&signal, duration);
				break;
			}
			printf("%s: Invalid type of generator\n", type);
			exit(1);
			break;
		case 'a':
			if (signal_flag.amplitude == 0)
			{
				signal.amplitude = atof(optarg);
				signal_flag.amplitude = 1;
			}
			else
			{
				end.amplitude = atof(optarg);
				end_flag.amplitude == 1;
			}
			if (signal.amplitude > 1)
			{
				printf("signal.amplitude > 1\n");
				exit(1);
			}
			if (end.amplitude > 1)
			{
				printf("end.amplitude > 1\n");
				exit(1);
			}
			break;
		case 'f':
			if (signal_flag.frequency == 0)
			{
				signal.frequency = atof(optarg);
				signal_flag.frequency = 1;
				signal.samplerate = 2.1 * signal.frequency;
			}
			else
			{
				end.frequency = atof(optarg);
				end_flag.frequency = 1;
				end.samplerate = 2.1 * end.frequency;
			}
			if (signal.frequency < 0)
			{
				printf("signal.frequency < 0\n");
				exit(1);
			}
			if (end.frequency < 0)
			{
				printf("end.frequency < 0\n");
				exit(1);
			}
			break;
		case 'p':
			if (signal_flag.phase == 0)
			{
				signal.phase = atof(optarg);
				signal_flag.phase = 1;
			}
			else
			{
				end.phase = atof(optarg);
				end_flag.phase = 1;
			}
			break;
		case 'r':
			if (signal_flag.samplerate == 0)
			{
				signal.samplerate = atof(optarg);
				if (signal.samplerate < 2 * signal.frequency)
					signal.samplerate = 2.1 * signal.frequency;
				signal_flag.samplerate = 1;
			}
			else
			{
				end.samplerate = atof(optarg);
				if (end.samplerate < 2 * end.frequency)
					end.samplerate = 2.1 * end.frequency;
				end_flag.samplerate = 1;
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
			filepath = optarg;
			break;
		}
	}
	printf("getopt end\n");

	sigbuff = (sigbuff == NULL) ? signal_tone(&signal, duration) : sigbuff;

/* WAV-file struct ===========================================================*/
	t_wavbuffer wavbuffer =
	{
		.channels = channels,
		.samplen = sizeof(uint16_t),
		.datalen = sigbuff->len,
		.data = malloc(channels * sizeof(uint8_t*))
	};
	printf("kek\n");

	for (int i = 0; i < channels; i++)
		wavbuffer.data[i] = (uint8_t *)sigbuff->buff;

	printf("kek1\n");
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

	header.block_align = header.channels * header.bits_per_sample / 8;
	header.byterate = header.sample_rate * header.block_align;
	header.overall_size += header.data_size;
/*============================================================================*/

	printf("wav_wropen\n");
	t_wavfile *file = wav_wropen(filepath, &header, &wavbuffer);

	wav_write(file, wavbuffer.datalen);

	wav_info(filepath, &header);

	wav_close(&file);

	return 0;
}