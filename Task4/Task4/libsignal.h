#ifndef LIBSIGNAL_H
# define LIBSIGNAL_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#define _USE_MATH_DEFINES
#include <math.h>

/*========== FIXED-POINT MATH ================================================*/
#include "libfix.h"
/*============================================================================*/

/*
**	Universal struct with time-domain characteristics 
*/

typedef struct
{
	uint16_t	*buff;
	size_t		len;
}				t_sigbuff;

typedef struct
{
	double		amplitude; // [0; 1]
	double		frequency;
	double		phase;
	double		samplerate;
}				t_signal;

/*========== SIGNAL GENERATORS ===============================================*/

/*
**	Harmonic signal with stable amplitude
*/

t_sigbuff	*signal_tone(t_signal *harmonic, double duration);

/*
** Linear chirp
*/

t_sigbuff	*signal_linsweep(t_signal *start, t_signal *end, double duration);

/*
** Exponential chirp
*/

t_sigbuff	*signal_expsweep(t_signal *start, t_signal *end, double duration);

/*
**	White Gausian noise generator
**	struct t_signal
**		-	amplitude   - amplitude of signal [0;1]
**		-	frequency   - maximum frequency;
**		-	phase		- not used
**		-	samplerate  -  2 * frequency;
*/

t_sigbuff	*signal_white(t_signal *harmonic, double duration);

/*============================================================================*/


#endif
