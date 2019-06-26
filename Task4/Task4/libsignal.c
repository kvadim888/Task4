#include "libsignal.h"

#include <assert.h> // for debugging

t_sigbuff	*signal_tone(t_signal *harmonic, double duration)
{
	assert(harmonic);
	t_sample	sample;
	double		dt = 1 / harmonic->samplerate;
	t_sigbuff	*sigbuff = malloc(sizeof(t_sigbuff));
	sigbuff->len = harmonic->samplerate * duration;
	sigbuff->buff = calloc(sigbuff->len, sizeof(uint16_t));

	for (int i = 0; i < sigbuff->len; i++)
	{
		sample.int32[1] = float_to_fix(harmonic->amplitude * sin(2 * M_PI * harmonic->frequency * i * dt));
		sigbuff->buff[i] = sample.int16[3];
	}
	return sigbuff;
}

t_sigbuff	*signal_linsweep(t_signal *start, t_signal *end, double duration)
{
	assert(start && end);
	t_sample	sample;
	double		rate = __max(start->samplerate, end->samplerate);
	double		dt = 1 / rate;
	double		k = (end->frequency - start->frequency) / duration;
	double		time = 0;
	t_sigbuff	*sigbuff = malloc(sizeof(t_sigbuff));
	sigbuff->len = rate * duration;
	sigbuff->buff = calloc(sigbuff->len, sizeof(uint16_t));

	for (int i = 0; i < sigbuff->len; i++)
	{
		sample.int32[1] = float_to_fix(start->amplitude *
			sin(start->phase + 2 * M_PI * time * (0.5 * k * time + start->frequency)));
		time += dt;
		sigbuff->buff[i] = sample.int16[3];
	}
	return sigbuff;
}

t_sigbuff	*signal_expsweep(t_signal *start, t_signal *end, double duration)
{
	assert(start && end);
	t_sample	sample;
	double		rate = __max(start->samplerate, end->samplerate);
	double		dt = 1 / rate;
	double		k = pow(end->frequency / start->frequency, 1 / duration);
	double		time = 0;
	t_sigbuff	*sigbuff = malloc(sizeof(t_sigbuff));
	sigbuff->len = rate * duration;
	sigbuff->buff = calloc(sigbuff->len, sizeof(uint16_t));

	for (int i = 0; i < sigbuff->len; i++)
	{
		sample.int32[1] = float_to_fix(start->amplitude *
			sin(start->phase + 2 * M_PI * start->frequency * ((pow(k, time) - 1) / log(k))));
		time += dt;
		sigbuff->buff[i] = sample.int16[3];
	}
	return sigbuff;
}

t_sigbuff	*signal_white(t_signal *harmonic, double duration)
{
	assert(harmonic);
	t_sample	sample;
	t_sigbuff	*sigbuff = malloc(sizeof(t_sigbuff));
	sigbuff->len = harmonic->samplerate * duration;
	sigbuff->buff = calloc(sigbuff->len, sizeof(uint16_t));

	for (int i = 0; i < sigbuff->len; i++)
	{
		sample.int32[1] = float_to_fix(harmonic->amplitude) * rand();
		sigbuff->buff[i] = sample.int16[3];
		sigbuff->buff[i] *= rand();
	}
	return sigbuff;
}
