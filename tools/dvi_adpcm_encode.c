/***********************************************************
Copyright 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/*
** Intel/DVI ADPCM coder/decoder.
**
** The algorithm for this coder was taken from the IMA Compatability Project
** proceedings, Vol 2, Number 2; May 1992.
**
** Version 1.2, 18-Dec-92.
**
** Change log:
** - Fixed a stupid bug, where the delta was computed as
**   stepsize*code/4 in stead of stepsize*(code+0.5)/4.
** - There was an off-by-one error causing it to pick
**   an incorrect delta once in a blue moon.
** - The NODIVMUL define has been removed. Computations are now always done
**   using shifts, adds and subtracts. It turned out that, because the standard
**   is defined using shift/add/subtract, you needed bits of fixup code
**   (because the div/mul simulation using shift/add/sub made some rounding
**   errors that real div/mul don't make) and all together the resultant code
**   ran slower than just using the shifts all the time.
** - Changed some of the variable names to be more meaningful.
*/

// modified May 30 2018 by jph for use in Moddable SDK]

#include "stdint.h"

/* Intel ADPCM step variation table */
static const int indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

static const int stepsizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};
    
/* vat uses four bytes for state, so we do, too, to stay compatible */
struct dvi_adpcm_state {
	int16_t valpred;    /* Previous predicted value */
	uint8_t index;     /* Index into stepsize table */
	uint8_t reserved;  /* reserved */
};

static struct dvi_adpcm_state state = {0};


/*
* Initialize encoder state.
*/
void dvi_adpcm_init(void)
{
	state.valpred = 0;
	state.index = 0;
	state.reserved = 0;
} /* dvi_adpcm_init */

/*
* Insert state into output packet only if 'header_flag' is true.  Not
* inserting a header allows piecing together a packet from several
* audio chunks.  If we prefix a header, we still encode the remaining
* samples.  The DVI standard says to skip the first sample, since it
* is already known to the receiver from the header.  'True' DVI blocks
* thus always contain an odd number of samples (see the definition of
* wSamplesPerBlock and the encoding routine in the DVI ADPCM Wave
* Type, e.g., found in the Microsoft Development Library.)
*/
int dvi_adpcm_encode(void *in_buf, int in_size, void *out_buf, int *out_size)
{
  signed char *out_sbuf = out_buf;
  int val;              /* Current input sample value */
  int sign;             /* Current adpcm sign bit */
  int delta;            /* Current adpcm output value */
  int diff;             /* Difference between val and valpred */
  int step;             /* Stepsize */
  int valpred;          /* Predicted output value */
  int vpdiff;           /* Current change to valpred */
  int index;            /* Current step change index */
  int outputbuffer = 0; /* place to keep previous 4-bit value */
  int bufferstep;       /* toggle between outputbuffer/output */
  int16_t *s;           /* output buffer for linear encoding */

  in_size /= 2;
  s = (int16_t *)in_buf;

  valpred = *s++;
  in_size -= 1;		//@@

  /* insert state into output buffer */
  *out_size = in_size / 2;

  ((struct dvi_adpcm_state *)out_buf)->valpred = valpred;		//@@ convert to little endian
  ((struct dvi_adpcm_state *)out_buf)->index = state.index;
  *out_size += sizeof(struct dvi_adpcm_state);
  out_sbuf  += sizeof(struct dvi_adpcm_state);

  index = state.index;
  step  = stepsizeTable[index];
  bufferstep = 1;  /* in/out: encoder state */

  for ( ; in_size > 0; in_size--) {
    val = *s++;

    /* Step 1 - compute difference with previous value */
    diff = val - valpred;
    sign = (diff < 0) ? 8 : 0;
    if ( sign ) diff = (-diff);

    /* Step 2 - Divide and clamp */
    /* Note:
    ** This code *approximately* computes:
    **    delta = diff*4/step;
    **    vpdiff = (delta+0.5)*step/4;
    ** but in shift step bits are dropped. The net result of this is
    ** that even if you have fast mul/div hardware you cannot put it to
    ** good use since the fixup would be too expensive.
    */
    delta = 0;
    vpdiff = (step >> 3);
  
    if ( diff >= step ) {
      delta = 4;
      diff -= step;
      vpdiff += step;
    }
    step >>= 1;
    if ( diff >= step  ) {
      delta |= 2;
      diff -= step;
      vpdiff += step;
    }
    step >>= 1;
    if ( diff >= step ) {
      delta |= 1;
      vpdiff += step;
    }

    /* Step 3 - Update previous value */
    if ( sign )
      valpred -= vpdiff;
    else
      valpred += vpdiff;

    /* Step 4 - Clamp previous value to 16 bits */
    if ( valpred > 32767 )
      valpred = 32767;
    else if ( valpred < -32768 )
      valpred = -32768;

    /* Step 5 - Assemble value, update index and step values */
    delta |= sign;
  
    index += indexTable[delta];
    if ( index < 0 ) index = 0;
    if ( index > 88 ) index = 88;
    step = stepsizeTable[index];

    /* Step 6 - Output value */
    if ( bufferstep ) {
      outputbuffer = delta & 0x0F;
    } else {
      *out_sbuf++ = ((delta << 4) & 0xf0) | outputbuffer;
    }
    bufferstep = !bufferstep;
  }

  /* Output last step, if needed */
  if ( !bufferstep ) *out_sbuf++ = outputbuffer;
    
  state.valpred   = valpred;
  state.index     = index;
  return 0;
} /* dvi_adpcm_encode */

