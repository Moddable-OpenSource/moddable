////////////////////////////////////////////////////////////////////////////
//                           **** ADPCM-XQ ****                           //
//                  Xtreme Quality ADPCM Encoder/Decoder                  //
//                    Copyright (c) 2024 David Bryant.                    //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "adpcm-lib.h"

/* This module encodes and decodes ADPCM (DVI/IMA varient). It handles standard 4-bit
 * ADPCM data (where each code is a "nibble") and also the less-supported 2-bit, 3-bit,
 * and 5-bit varients. ADPCM audio is divided into independently decodable blocks that
 * can be relatively small. The most common configuration is to store 505 4-bit samples
 * into a 256 byte block, although other sizes are permitted as long as the number of
 * samples is one greater than a multiple of 8 (for 4-bit codes). When multiple
 * channels are present, they are interleaved in the data with a 4-byte interval, even
 * for code sizes that don't evenly divide into 32 bits (which seems a little weird at
 * first, but is actually kinda cool).
 *
 * Thanks to Jon Olick for the idea of limiting the ply search to only those values
 * that are reasonably likely to provide a benefit:
 *
 * https://www.jonolick.com/home/introducing-a-single-file-pcmadpcm-wav-file-writer
 */

/************************************ ADPCM encoder ***********************************/

typedef uint64_t rms_error_t;     // best if "double" or "uint64_t", "float" okay in a pinch
#define MAX_RMS_ERROR UINT64_MAX
// typedef double rms_error_t;     // best if "double" or "uint64_t", "float" okay in a pinch
// #define MAX_RMS_ERROR DBL_MAX

#define CLIP(data, min, max) \
if ((data) > (max)) data = max; \
else if ((data) < (min)) data = min;

// Given the code size in bits (e.g., 2 - 5), these macros convert from ADPCM "nibble"
// values (0 - 2^n-1) to and from the equivalent deltas (+/- 2^(n-1), no zero)
#define NIBBLE_TO_DELTA(b,n) ((n)<(1<<((b)-1))?(n)+1:(1<<((b)-1))-1-(n))
#define DELTA_TO_NIBBLE(b,d) ((d)<0?(1<<((b)-1))-1-(d):(d)-1)

#define NOISE_SHAPING_ENABLED   (NOISE_SHAPING_DYNAMIC | NOISE_SHAPING_STATIC)

/* step table */
static const uint16_t step_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

/* step index tables */
static const int index_table[] = {
    /* adpcm data size is 4 */
    -1, -1, -1, -1, 2, 4, 6, 8
};

static const int index_table_3bit[] = {
    /* adpcm data size is 3 */
    -1, -1, 1, 2
};

static const int index_table_5bit[] = {
    /* adpcm data size is 5 */
    -1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 6, 8, 10, 13, 16
};

struct adpcm_channel {
    int32_t pcmdata;                        // current PCM value
    int32_t shaping_weight, error;          // for noise shaping
    int8_t index;                           // current index into step size table
};

struct adpcm_context {
    struct adpcm_channel channels [2];
    int num_channels, sample_rate, config_flags;
    int16_t *dynamic_shaping_array, last_shaping_weight;
    int static_shaping_weight;
};

/* With the addition of 3-bit and 5-bit ADPCM formats and various alignment requirements,
 * it's become rather complicated to convert between sample counts and block sizes and
 * make sure the alignment is always correct. Therefore I have put dedicated functions
 * for this in here and removed the functionality from the command-line program.
 *
 * The first two function simply convert back and forth between sample counts and
 * block sizes (including the header). Note that these functions ignore the alignment
 * requirement that the 3-bit and 5-bit formats must exactly fill the block because
 * this requirement is really not neccessary and some programs ignore it (e.g., Adobe
 * Audition), so it's good to be able to correctly _decode_ such files (but probably
 * not a great idea to _create_ them).
 */

int adpcm_sample_count_to_block_size (int sample_count, int num_chans, int bps)
{
    return ((sample_count - 1) * bps + 31) / 32 * num_chans * 4 + (num_chans * 4);
}

int adpcm_block_size_to_sample_count (int block_size, int num_chans, int bps)
{
    return (block_size - num_chans * 4) / num_chans * 8 / bps + 1;
}

/* Convert an ADPCM block size (including header) to a (possibly) modified size that
 * is exactly bit-filled given the channel count and sample size (from 2 - 5 bits).
 * The round_up arg controls whether we round up or down to the next aligned value.
 * Rounding up ensures that the new block size will still hold at least as many
 * samples as the old block size. Even though this particular alignment requirement
 * is not really required (the spec is ambiguous) and some programs ignore it, both
 * FFmpeg (VLC) and Rockbox generate glitches when playing files that don't adhere,
 * so this function is provided to enforce it.
 */

int adpcm_align_block_size (int block_size, int num_chans, int bps, int round_up)
{
    int sample_count = adpcm_block_size_to_sample_count (block_size, num_chans, bps) - 1;
    int sample_align = (bps & 1) ? 32 : 32 / bps;

    sample_count = (sample_count + (sample_align - 1) * round_up) / sample_align * sample_align;
    return adpcm_sample_count_to_block_size (sample_count + 1, num_chans, bps);
}

/* Create ADPCM encoder context with given number of channels.
 * The returned pointer is used for subsequent calls. Note that
 * even though an ADPCM encoder could be set up to encode frames
 * independently, we use a context so that we can use previous
 * data to improve quality, mostly with respect to noise-shaping
 * but also for the step table index at low search depths.
 */

void *adpcm_create_context (int num_channels, int sample_rate, int lookahead, int noise_shaping)
{
    struct adpcm_context *pcnxt = malloc (sizeof (struct adpcm_context));
    int ch;

    memset (pcnxt, 0, sizeof (struct adpcm_context));
    pcnxt->config_flags = noise_shaping | lookahead;
    pcnxt->static_shaping_weight = 1024;
    pcnxt->num_channels = num_channels;
    pcnxt->sample_rate = sample_rate;

    // we set the indicies to invalid values so that we always recalculate them
    // on at least the first frame (and every frame if the depth is sufficient)

    for (ch = 0; ch < num_channels; ++ch)
        pcnxt->channels [ch].index = -1;

    return pcnxt;
}

/* Set the shaping weight in range: -1.0 > weight >= 1.0.
 * Note that previously this was fixed to pure first-order (i.e., 1.0).
 * Also, values very close to -1.0 are not recommended because
 * of the high DC gain.
 */

void adpcm_set_shaping_weight (void *p, double shaping_weight)
{
    struct adpcm_context *pcnxt = (struct adpcm_context *) p;

    pcnxt->static_shaping_weight = (int) floor (shaping_weight * 1024.0 + 0.5);

    if (pcnxt->static_shaping_weight > 1024) pcnxt->static_shaping_weight = 1024;
    if (pcnxt->static_shaping_weight < -1023) pcnxt->static_shaping_weight = -1023;
}

/* Free the ADPCM encoder context.
 */

void adpcm_free_context (void *p)
{
    struct adpcm_context *pcnxt = (struct adpcm_context *) p;

    free (pcnxt);
}

/* Apply noise-shaping to the supplied sample value using the shaping_weight
 * and accumulated error term stored in the adpcm_channel structure. Note that
 * the error term in the structure is updated, but won't be "correct" until the
 * final re-quantized sample value is added to it (and of course we don't know
 * that value yet).
 */

static inline int32_t noise_shape (struct adpcm_channel *pchan, int32_t sample)
{
    int32_t temp = -((pchan->shaping_weight * pchan->error + 512) >> 10);

    if (pchan->shaping_weight < 0 && temp) {
        if (temp == pchan->error)
            temp = (temp < 0) ? temp + 1 : temp - 1;

        pchan->error = -sample;
        sample += temp;
    }
    else
        pchan->error = -(sample += temp);

    return sample;
}

/* These recursive functions are the core of the "lookahead" feature of the library.
 * They determine the best ADPCM code for the given audio (optionally returned in
 * *best_nibble) and also return the minimum RMS error that that code will generate
 * for the specified depth of the future audio. For speed, there are separate
 * versions for each code size (e.g., 2bit to 5bit).
 *
 * Parameters:
 *  pchan           pointer to the encoding status for the channel to encode
 *  nch             number of channels (just used to correctly stride sample array)
 *  csample         current sample to encode (may be modified by noise shaping)
 *  psample         pointer to samples for lookahead (enough for lookahead depth)
 *  flags           depth of search (in lower bits) plus some other control bits       
 *  best_nibble     optional pointer for return of best nibble for current sample
 *  max_error       maximum allowed error (used to eliminate pointless branches)
 *
 * Returns RMS total error for the specified depth
 */

static rms_error_t min_error_4bit (const struct adpcm_channel *pchan, int nch, int32_t csample, const int16_t *psample, int flags, int *best_nibble, rms_error_t max_error)
{
    int32_t delta = csample - pchan->pcmdata, csample2;
    struct adpcm_channel chan = *pchan;
    uint16_t step = step_table[chan.index];
    uint16_t trial_delta = (step >> 3);
    int nibble, testnbl;
    rms_error_t min_error;

    // this odd-looking code always generates the nibble value with the least error,
    // regardless of step size (which was not true previously)

    if (delta < 0) {
        int mag = ((-delta << 2) + (step & 3) + ((step & 1) << 1)) / step;
        nibble = 0x8 | (mag > 7 ? 7 : mag);
    }
    else {
        int mag = ((delta << 2) + (step & 3) + ((step & 1) << 1)) / step;
        nibble = mag > 7 ? 7 : mag;
    }

    if (nibble & 1) trial_delta += (step >> 2);
    if (nibble & 2) trial_delta += (step >> 1);
    if (nibble & 4) trial_delta += step;

    if (nibble & 8)
        chan.pcmdata -= trial_delta;
    else
        chan.pcmdata += trial_delta;

    CLIP(chan.pcmdata, -32768, 32767);
    if (best_nibble) *best_nibble = nibble;
    min_error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);

    // if we're at a leaf, or we're not at a leaf but have already exceeded the error limit, return
    if (!(flags & LOOKAHEAD_DEPTH) || min_error >= max_error)
        return min_error;

    // otherwise we execute that naively closest nibble and search deeper for improvement

    chan.index += index_table[nibble & 0x07];
    CLIP(chan.index, 0, 88);

    if (flags & NOISE_SHAPING_ENABLED) {
        chan.error += chan.pcmdata;
        csample2 = noise_shape (&chan, psample [nch]);
    }
    else
        csample2 = psample [nch];

    min_error += min_error_4bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, max_error - min_error);

    // min_error is the error (from here to the leaf) for the naively closest nibble.
    // Unless we've been told not to try, we may be able to improve on that by choosing
    // an alternative (not closest) nibble.

    if (flags & LOOKAHEAD_NO_BRANCHING)
        return min_error;

    for (testnbl = 0; testnbl <= 0xF; ++testnbl) {
        rms_error_t error, threshold;

        if (testnbl == nibble)  // don't do the same value again
            continue;

        // we execute this branch if:
        // 1. we're doing an exhaustive search, or
        // 2. the test value is one of the maximum values (i.e., 0x7 or 0xf), or
        // 3. the test value's delta is within three of the initial estimate's delta

        if (flags & LOOKAHEAD_EXHAUSTIVE || !(~testnbl & 0x7) || abs (NIBBLE_TO_DELTA (4,nibble) - NIBBLE_TO_DELTA (4,testnbl)) <= 3) {
            trial_delta = (step >> 3);
            chan = *pchan;

            if (testnbl & 1) trial_delta += (step >> 2);
            if (testnbl & 2) trial_delta += (step >> 1);
            if (testnbl & 4) trial_delta += step;

            if (testnbl & 8)
                chan.pcmdata -= trial_delta;
            else
                chan.pcmdata += trial_delta;

            CLIP(chan.pcmdata, -32768, 32767);

            error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);
            threshold = max_error < min_error ? max_error : min_error;

            if (error < threshold) {
                chan.index += index_table[testnbl & 0x07];
                CLIP(chan.index, 0, 88);

                if (flags & NOISE_SHAPING_ENABLED) {
                    chan.error += chan.pcmdata;
                    csample2 = noise_shape (&chan, psample [nch]);
                }
                else
                    csample2 = psample [nch];

                error += min_error_4bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, threshold - error);

                if (error < min_error) {
                    if (best_nibble) *best_nibble = testnbl;
                    min_error = error;
                }
            }
        }
    }

    return min_error;
}

static rms_error_t min_error_2bit (const struct adpcm_channel *pchan, int nch, int32_t csample, const int16_t *psample, int flags, int *best_nibble, rms_error_t max_error)
{
    int32_t delta = csample - pchan->pcmdata, csample2;
    struct adpcm_channel chan = *pchan;
    uint16_t step = step_table[chan.index];
    int nibble, testnbl;
    rms_error_t min_error;

    if (delta < 0) {
        if (-delta >= step) {
            chan.pcmdata -= step + (step >> 1);
            nibble = 3;
        }
        else {
            chan.pcmdata -= step >> 1;
            nibble = 2;
        }
    }
    else
        chan.pcmdata += step * ((nibble = delta >= step)) + (step >> 1);

    CLIP(chan.pcmdata, -32768, 32767);
    if (best_nibble) *best_nibble = nibble;
    min_error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);

    // if we're at a leaf, or we're not at a leaf but have already exceeded the error limit, return
    if (!(flags & LOOKAHEAD_DEPTH) || min_error >= max_error)
        return min_error;

    // otherwise we execute that naively closest nibble and search deeper for improvement

    chan.index += (nibble & 1) * 3 - 1;
    CLIP(chan.index, 0, 88);

    if (flags & NOISE_SHAPING_ENABLED) {
        chan.error += chan.pcmdata;
        csample2 = noise_shape (&chan, psample [nch]);
    }
    else
        csample2 = psample [nch];

    min_error += min_error_2bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, max_error - min_error);

    // min_error is the error (from here to the leaf) for the naively closest nibble.
    // Unless we've been told not to try, we may be able to improve on that by choosing
    // an alternative (not closest) nibble.

    if (flags & LOOKAHEAD_NO_BRANCHING)
        return min_error;

    for (testnbl = 0; testnbl <= 0x3; ++testnbl) {
        rms_error_t error, threshold;

        if (testnbl == nibble)  // don't do the same value again
            continue;

        chan = *pchan;

        if (testnbl & 2)
            chan.pcmdata -= step * (testnbl & 1) + (step >> 1);
        else
            chan.pcmdata += step * (testnbl & 1) + (step >> 1);

        CLIP(chan.pcmdata, -32768, 32767);

        error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);
        threshold = max_error < min_error ? max_error : min_error;

        if (error < threshold) {
            chan.index += (testnbl & 1) * 3 - 1;
            CLIP(chan.index, 0, 88);

            if (flags & NOISE_SHAPING_ENABLED) {
                chan.error += chan.pcmdata;
                csample2 = noise_shape (&chan, psample [nch]);
            }
            else
                csample2 = psample [nch];

            error += min_error_2bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, threshold - error);

            if (error < min_error) {
                if (best_nibble) *best_nibble = testnbl;
                min_error = error;
            }
        }
    }

    return min_error;
}

static rms_error_t min_error_3bit (const struct adpcm_channel *pchan, int nch, int32_t csample, const int16_t *psample, int flags, int *best_nibble, rms_error_t max_error)
{
    int32_t delta = csample - pchan->pcmdata, csample2;
    struct adpcm_channel chan = *pchan;
    uint16_t step = step_table[chan.index];
    uint16_t trial_delta = (step >> 2);
    int nibble, testnbl;
    rms_error_t min_error;

    if (delta < 0) {
        int mag = ((-delta << 1) + (step & 1)) / step;
        nibble = 0x4 | (mag > 3 ? 3 : mag);
    }
    else {
        int mag = ((delta << 1) + (step & 1)) / step;
        nibble = mag > 3 ? 3 : mag;
    }

    if (nibble & 1) trial_delta += (step >> 1);
    if (nibble & 2) trial_delta += step;

    if (nibble & 4)
        chan.pcmdata -= trial_delta;
    else
        chan.pcmdata += trial_delta;

    CLIP(chan.pcmdata, -32768, 32767);
    if (best_nibble) *best_nibble = nibble;
    min_error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);

    // if we're at a leaf, or we're not at a leaf but have already exceeded the error limit, return
    if (!(flags & LOOKAHEAD_DEPTH) || min_error >= max_error)
        return min_error;

    // otherwise we execute that naively closest nibble and search deeper for improvement

    chan.index += index_table_3bit[nibble & 0x03];
    CLIP(chan.index, 0, 88);

    if (flags & NOISE_SHAPING_ENABLED) {
        chan.error += chan.pcmdata;
        csample2 = noise_shape (&chan, psample [nch]);
    }
    else
        csample2 = psample [nch];

    min_error += min_error_3bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, max_error - min_error);

    // min_error is the error (from here to the leaf) for the naively closest nibble.
    // Unless we've been told not to try, we may be able to improve on that by choosing
    // an alternative (not closest) nibble.

    if (flags & LOOKAHEAD_NO_BRANCHING)
        return min_error;

    for (testnbl = 0; testnbl <= 0x7; ++testnbl) {
        rms_error_t error, threshold;

        if (testnbl == nibble)  // don't do the same value again
            continue;

        // we execute this branch if:
        // 1. we're doing an exhaustive search, or
        // 2. the test value is one of the maximum values (i.e., 0x3 or 0x7), or
        // 3. the test value's delta is within two of the initial estimate's delta

        if (flags & LOOKAHEAD_EXHAUSTIVE || !(~testnbl & 0x3) || abs (NIBBLE_TO_DELTA (3,nibble) - NIBBLE_TO_DELTA (3,testnbl)) <= 2) {
            trial_delta = (step >> 2);
            chan = *pchan;

            if (testnbl & 1) trial_delta += (step >> 1);
            if (testnbl & 2) trial_delta += step;

            if (testnbl & 4)
                chan.pcmdata -= trial_delta;
            else
                chan.pcmdata += trial_delta;

            CLIP(chan.pcmdata, -32768, 32767);
            error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);
            threshold = max_error < min_error ? max_error : min_error;

            if (error < threshold) {
                chan.index += index_table_3bit[testnbl & 0x03];
                CLIP(chan.index, 0, 88);

                if (flags & NOISE_SHAPING_ENABLED) {
                    chan.error += chan.pcmdata;
                    csample2 = noise_shape (&chan, psample [nch]);
                }
                else
                    csample2 = psample [nch];

                error += min_error_3bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, threshold - error);

                if (error < min_error) {
                    if (best_nibble) *best_nibble = testnbl;
                    min_error = error;
                }
            }
        }
    }

    return min_error;
}

static rms_error_t min_error_5bit (const struct adpcm_channel *pchan, int nch, int32_t csample, const int16_t *psample, int flags, int *best_nibble, rms_error_t max_error)
{
    static char comp_table [16] = { 0, 0, 0, 5, 0, 6, 4, 10, 0, 7, 6, 10, 4, 11, 11, 13 };
    int32_t delta = csample - pchan->pcmdata, csample2;
    struct adpcm_channel chan = *pchan;
    uint16_t step = step_table[chan.index];
    uint16_t trial_delta = (step >> 4);
    int nibble, testnbl;
    rms_error_t min_error;

    if (delta < 0) {
        int mag = ((-delta << 3) + comp_table [step & 0xf]) / step;
        nibble = 0x10 | (mag > 0xf ? 0xf : mag);
    }
    else {
        int mag = ((delta << 3) + comp_table [step & 0xf]) / step;
        nibble = mag > 0xf ? 0xf : mag;
    }

    if (nibble & 1) trial_delta += (step >> 3);
    if (nibble & 2) trial_delta += (step >> 2);
    if (nibble & 4) trial_delta += (step >> 1);
    if (nibble & 8) trial_delta += step;

    if (nibble & 0x10)
        chan.pcmdata -= trial_delta;
    else
        chan.pcmdata += trial_delta;

    CLIP(chan.pcmdata, -32768, 32767);
    if (best_nibble) *best_nibble = nibble;
    min_error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);

    // if we're at a leaf, or we're not at a leaf but have already exceeded the error limit, return
    if (!(flags & LOOKAHEAD_DEPTH) || min_error >= max_error)
        return min_error;

    // otherwise we execute that naively closest nibble and search deeper for improvement

    chan.index += index_table_5bit[nibble & 0x0f];
    CLIP(chan.index, 0, 88);

    if (flags & NOISE_SHAPING_ENABLED) {
        chan.error += chan.pcmdata;
        csample2 = noise_shape (&chan, psample [nch]);
    }
    else
        csample2 = psample [nch];

    min_error += min_error_5bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, max_error - min_error);

    // min_error is the error (from here to the leaf) for the naively closest nibble.
    // Unless we've been told not to try, we may be able to improve on that by choosing
    // an alternative (not closest) nibble.

    if (flags & LOOKAHEAD_NO_BRANCHING)
        return min_error;

    for (testnbl = 0; testnbl <= 0x1F; ++testnbl) {
        rms_error_t error, threshold;

        if (testnbl == nibble)  // don't do the same value again
            continue;

        // we execute this trial if:
        // 1. we're doing an exhaustive search, or
        // 2. the trial value is one of the four maximum values for the sign, or
        // 3. the test value's delta is within three of the initial estimate's delta

        if (flags & LOOKAHEAD_EXHAUSTIVE || (testnbl | 3) == (nibble | 0xf) || abs (NIBBLE_TO_DELTA (5,nibble) - NIBBLE_TO_DELTA (5,testnbl)) <= 3) {
            trial_delta = (step >> 4);
            chan = *pchan;

            if (testnbl & 1) trial_delta += (step >> 3);
            if (testnbl & 2) trial_delta += (step >> 2);
            if (testnbl & 4) trial_delta += (step >> 1);
            if (testnbl & 8) trial_delta += step;

            if (testnbl & 0x10)
                chan.pcmdata -= trial_delta;
            else
                chan.pcmdata += trial_delta;

            CLIP(chan.pcmdata, -32768, 32767);

            error = (rms_error_t) (chan.pcmdata - csample) * (chan.pcmdata - csample);
            threshold = max_error < min_error ? max_error : min_error;

            if (error < threshold) {
                chan.index += index_table_5bit [testnbl & 0x0f];
                CLIP(chan.index, 0, 88);

                if (flags & NOISE_SHAPING_ENABLED) {
                    chan.error += chan.pcmdata;
                    csample2 = noise_shape (&chan, psample [nch]);
                }
                else
                    csample2 = psample [nch];

                error += min_error_5bit (&chan, nch, csample2, psample + nch, flags - 1, NULL, threshold - error);

                if (error < min_error) {
                    if (best_nibble) *best_nibble = testnbl;
                    min_error = error;
                }
            }
        }
    }

    return min_error;
}

static uint8_t encode_sample (struct adpcm_context *pcnxt, int ch, int bps, const int16_t *psample, int num_samples)
{
    struct adpcm_channel *pchan = pcnxt->channels + ch;
    uint16_t step = step_table[pchan->index];
    int flags = pcnxt->config_flags, nibble;
    int32_t csample = *psample;
    uint16_t trial_delta;

    if (flags & NOISE_SHAPING_ENABLED)
        csample = noise_shape (pchan, csample); 

    if ((flags & LOOKAHEAD_DEPTH) > num_samples - 1)
        flags = (flags & ~LOOKAHEAD_DEPTH) + num_samples - 1;

    if (bps == 2) {
        min_error_2bit (pchan, pcnxt->num_channels, csample, psample, flags, &nibble, MAX_RMS_ERROR);

        if (nibble & 2)
            pchan->pcmdata -= step * (nibble & 1) + (step >> 1);
        else
            pchan->pcmdata += step * (nibble & 1) + (step >> 1);

        pchan->index += (nibble & 1) * 3 - 1;
    }
    else if (bps == 3) {
        min_error_3bit (pchan, pcnxt->num_channels, csample, psample, flags, &nibble, MAX_RMS_ERROR);
        trial_delta = (step >> 2);
        if (nibble & 1) trial_delta += (step >> 1);
        if (nibble & 2) trial_delta += step;

        if (nibble & 4)
            pchan->pcmdata -= trial_delta;
        else
            pchan->pcmdata += trial_delta;

        pchan->index += index_table_3bit[nibble & 0x03];
    }
    else if (bps == 4) {
        min_error_4bit (pchan, pcnxt->num_channels, csample, psample, flags, &nibble, MAX_RMS_ERROR);
        trial_delta = (step >> 3);
        if (nibble & 1) trial_delta += (step >> 2);
        if (nibble & 2) trial_delta += (step >> 1);
        if (nibble & 4) trial_delta += step;

        if (nibble & 8)
            pchan->pcmdata -= trial_delta;
        else
            pchan->pcmdata += trial_delta;

        pchan->index += index_table[nibble & 0x07];
    }
    else {  // bps == 5
        min_error_5bit (pchan, pcnxt->num_channels, csample, psample, flags, &nibble, MAX_RMS_ERROR);
        trial_delta = (step >> 4);
        if (nibble & 1) trial_delta += (step >> 3);
        if (nibble & 2) trial_delta += (step >> 2);
        if (nibble & 4) trial_delta += (step >> 1);
        if (nibble & 8) trial_delta += step;

        if (nibble & 0x10)
            pchan->pcmdata -= trial_delta;
        else
            pchan->pcmdata += trial_delta;

        pchan->index += index_table_5bit[nibble & 0x0f];
    }

    CLIP(pchan->index, 0, 88);
    CLIP(pchan->pcmdata, -32768, 32767);

    if (flags & NOISE_SHAPING_ENABLED)
        pchan->error += pchan->pcmdata;

    return nibble;
}

static void encode_chunks (struct adpcm_context *pcnxt, uint8_t *outbuf, size_t *outbufsize, const int16_t *inbuf, int inbufcount, int bps)
{
    const int16_t *pcmbuf;
    int ch;

    for (ch = 0; ch < pcnxt->num_channels; ++ch) {
        int shiftbits = 0, numbits = 0, i, j;

        if (pcnxt->config_flags & NOISE_SHAPING_STATIC)
            pcnxt->channels [ch].shaping_weight = pcnxt->static_shaping_weight;

        pcmbuf = inbuf + ch;

        for (j = i = 0; i < inbufcount; ++i) {
            if (pcnxt->config_flags & NOISE_SHAPING_DYNAMIC)
                pcnxt->channels [ch].shaping_weight = pcnxt->dynamic_shaping_array [i];

            shiftbits |= encode_sample (pcnxt, ch, bps, pcmbuf, inbufcount - i) << numbits;
            pcmbuf += pcnxt->num_channels;

            if ((numbits += bps) >= 8) {
                outbuf [(j & ~3) * pcnxt->num_channels + (ch * 4) + (j & 3)] = shiftbits;
                shiftbits >>= 8;
                numbits -= 8;
                j++;
            }
        }

        if (numbits)
            outbuf [(j & ~3) * pcnxt->num_channels + (ch * 4) + (j & 3)] = shiftbits;
    }

    *outbufsize += (inbufcount * bps + 31) / 32 * pcnxt->num_channels * 4;
}

/* Encode a block of 16-bit PCM data into N-bit ADPCM.
 *
 * Parameters:
 *  p               the context returned by adpcm_begin()
 *  outbuf          destination buffer
 *  outbufsize      pointer to variable where the number of bytes written
 *                   will be stored
 *  inbuf           source PCM samples
 *  inbufcount      number of composite PCM samples provided (note: this is
 *                   the total number of 16-bit samples divided by the number
 *                   of channels)
 *  bps             bits per ADPCM sample (2-5)
 *
 * Returns 1 for success or 0 for error (which is only invalid bit count)
 */

int adpcm_encode_block_ex (void *p, uint8_t *outbuf, size_t *outbufsize, const int16_t *inbuf, int inbufcount, int bps)
{
    struct adpcm_context *pcnxt = (struct adpcm_context *) p;
    int ch;

    *outbufsize = 0;

    if (bps < 2 || bps > 5)
        return 0;

    if (!inbufcount)
        return 1;

    // The first PCM sample is encoded verbatim. In theory, we should apply the noise shaping,
    // but we'll actually just apply the error term on the next sample.

    for (ch = 0; ch < pcnxt->num_channels; ch++)
        pcnxt->channels[ch].pcmdata = *inbuf++;

    inbufcount--;

    // Use min_error_nbit() to find the optimum initial index if this is the first frame or
    // the lookahead depth is at least 3. Below that just using the value leftover from
    // the previous frame is better, and of course faster.

    if (inbufcount && (pcnxt->channels [0].index < 0 || (pcnxt->config_flags & LOOKAHEAD_DEPTH) >= 3)) {
        int flags = 16 | LOOKAHEAD_NO_BRANCHING;

        if ((flags & LOOKAHEAD_DEPTH) > inbufcount - 1)
            flags = (flags & ~LOOKAHEAD_DEPTH) + inbufcount - 1;

        for (ch = 0; ch < pcnxt->num_channels; ch++) {
            rms_error_t min_error = MAX_RMS_ERROR;
            rms_error_t error_per_index [89];
            int best_index;

            for (int tindex = 0; tindex <= 88; tindex++) {
                struct adpcm_channel chan = pcnxt->channels [ch];

                chan.index = tindex;
                chan.shaping_weight = 0;

                if (bps == 2)
                    error_per_index [tindex] = min_error_2bit (&chan, pcnxt->num_channels, inbuf [ch], inbuf + ch, flags, NULL, MAX_RMS_ERROR);
                else if (bps == 3)
                    error_per_index [tindex] = min_error_3bit (&chan, pcnxt->num_channels, inbuf [ch], inbuf + ch, flags, NULL, MAX_RMS_ERROR);
                else if (bps == 5)
                    error_per_index [tindex] = min_error_5bit (&chan, pcnxt->num_channels, inbuf [ch], inbuf + ch, flags, NULL, MAX_RMS_ERROR);
                else
                    error_per_index [tindex] = min_error_4bit (&chan, pcnxt->num_channels, inbuf [ch], inbuf + ch, flags, NULL, MAX_RMS_ERROR);
            }

            // we use a 3-wide average window because the min_error_nbit() results can be noisy

            for (int tindex = 0; tindex <= 87; tindex++) {
                rms_error_t terror = error_per_index [tindex];

                if (tindex)
                    terror = (error_per_index [tindex - 1] + terror + error_per_index [tindex + 1]) / 3;

                if (terror < min_error) {
                    best_index = tindex;
                    min_error = terror;
                }
            }

            pcnxt->channels [ch].index = best_index;
        }
    }

    // write the block header, which includes the first PCM sample verbatim

    for (ch = 0; ch < pcnxt->num_channels; ch++) {
        outbuf[0] = pcnxt->channels[ch].pcmdata;
        outbuf[1] = pcnxt->channels[ch].pcmdata >> 8;
        outbuf[2] = pcnxt->channels[ch].index;
        outbuf[3] = 0;

        outbuf += 4;
        *outbufsize += 4;
    }

    if (inbufcount && (pcnxt->config_flags & NOISE_SHAPING_DYNAMIC)) {
        pcnxt->dynamic_shaping_array = malloc (inbufcount * sizeof (int16_t));
        generate_dns_values (inbuf, inbufcount, pcnxt->num_channels, pcnxt->sample_rate, pcnxt->dynamic_shaping_array, -512, pcnxt->last_shaping_weight);
        pcnxt->last_shaping_weight = pcnxt->dynamic_shaping_array [inbufcount - 1];
    }

    // encode the rest of the PCM samples, if any, into 32-bit, possibly interleaved, chunks

    if (inbufcount)
        encode_chunks (pcnxt, outbuf, outbufsize, inbuf, inbufcount, bps);

    if (pcnxt->dynamic_shaping_array && (pcnxt->config_flags & NOISE_SHAPING_DYNAMIC)) {
        free (pcnxt->dynamic_shaping_array);
        pcnxt->dynamic_shaping_array = NULL;
    }

    return 1;
}

/* Encode a block of 16-bit PCM data into 4-bit ADPCM.
 *
 * Parameters:
 *  p               the context returned by adpcm_begin()
 *  outbuf          destination buffer
 *  outbufsize      pointer to variable where the number of bytes written
 *                   will be stored
 *  inbuf           source PCM samples
 *  inbufcount      number of composite PCM samples provided (note: this is
 *                   the total number of 16-bit samples divided by the number
 *                   of channels)
 *
 * Returns 1 (for success as there is no error checking)
 */

int adpcm_encode_block (void *p, uint8_t *outbuf, size_t *outbufsize, const int16_t *inbuf, int inbufcount)
{
    return adpcm_encode_block_ex (p, outbuf, outbufsize, inbuf, inbufcount, 4);
}

/************************************ ADPCM decoder ***********************************/

/* Decode the block of 4-bit ADPCM data into PCM. This requires no context because ADPCM
 * blocks are independently decodable. This assumes that a single entire block is always
 * decoded; it must be called multiple times for multiple blocks and cannot resume in the
 * middle of a block. Note that for all other bit depths, use adpcm_decode_block_ex().
 *
 * Parameters:
 *  outbuf          destination for interleaved PCM samples
 *  inbuf           source ADPCM block
 *  inbufsize       size of source ADPCM block
 *  channels        number of channels in block (must be determined from other context)
 *
 * Returns number of converted composite samples (total samples divided by number of channels)
 */ 

int adpcm_decode_block (int16_t *outbuf, const uint8_t *inbuf, size_t inbufsize, int channels)
{
    int ch, samples = 1, chunks;
    int32_t pcmdata[2];
    int8_t index[2];

    if (inbufsize < (uint32_t) channels * 4)
        return 0;

    for (ch = 0; ch < channels; ch++) {
        *outbuf++ = pcmdata[ch] = (int16_t) (inbuf [0] | (inbuf [1] << 8));
        index[ch] = inbuf [2];

        if (index [ch] < 0 || index [ch] > 88 || inbuf [3])     // sanitize the input a little...
            return 0;

        inbufsize -= 4;
        inbuf += 4;
    }

    chunks = inbufsize / (channels * 4);
    samples += chunks * 8;

    while (chunks--) {
        int ch, i;

        for (ch = 0; ch < channels; ++ch) {

            for (i = 0; i < 4; ++i) {
                uint16_t step = step_table [index [ch]], delta = step >> 3;

                if (*inbuf & 1) delta += (step >> 2);
                if (*inbuf & 2) delta += (step >> 1);
                if (*inbuf & 4) delta += step;

                if (*inbuf & 8)
                    pcmdata[ch] -= delta;
                else
                    pcmdata[ch] += delta;

                index[ch] += index_table [*inbuf & 0x7];
                CLIP(index[ch], 0, 88);
                CLIP(pcmdata[ch], -32768, 32767);
                outbuf [i * 2 * channels] = pcmdata[ch];

                step = step_table [index [ch]]; delta = step >> 3;

                if (*inbuf & 0x10) delta += (step >> 2);
                if (*inbuf & 0x20) delta += (step >> 1);
                if (*inbuf & 0x40) delta += step;

                if (*inbuf & 0x80)
                    pcmdata[ch] -= delta;
                else
                    pcmdata[ch] += delta;
                
                index[ch] += index_table [(*inbuf >> 4) & 0x7];
                CLIP(index[ch], 0, 88);
                CLIP(pcmdata[ch], -32768, 32767);
                outbuf [(i * 2 + 1) * channels] = pcmdata[ch];

                inbuf++;
            }

            outbuf++;
        }

        outbuf += channels * 7;
    }

    return samples;
}

/* Decode the block of ADPCM data, with from 2 to 5 bits per sample, into 16-bit PCM.
 * This requires no context because ADPCM blocks are independently decodable. This assumes
 * that a single entire block is always decoded; it must be called multiple times for
 * multiple blocks and cannot resume in the middle of a block.
 *
 * Parameters:
 *  outbuf          destination for interleaved PCM samples
 *  inbuf           source ADPCM block
 *  inbufsize       size of source ADPCM block
 *  channels        number of channels in block (must be determined from other context)
 *  bps             bits per ADPCM sample (2-5, must be determined from other context)
 *
 * Returns number of converted composite samples (total samples divided by number of channels)
 */ 

int adpcm_decode_block_ex (int16_t *outbuf, const uint8_t *inbuf, size_t inbufsize, int channels, int bps)
{
    int samples = 1, ch;
    int32_t pcmdata[2];
    int8_t index[2];

    if (bps == 4)
        return adpcm_decode_block (outbuf, inbuf, inbufsize, channels);

    if (bps < 2 || bps > 5 || inbufsize < (uint32_t) channels * 4)
        return 0;

    for (ch = 0; ch < channels; ch++) {
        *outbuf++ = pcmdata[ch] = (int16_t) (inbuf [0] | (inbuf [1] << 8));
        index[ch] = inbuf [2];

        if (index [ch] < 0 || index [ch] > 88 || inbuf [3])     // sanitize the input a little...
            return 0;

        inbufsize -= 4;
        inbuf += 4;
    }

    if (!inbufsize || (inbufsize % (channels * 4)))             // extra clean
        return samples;

    samples += inbufsize / channels * 8 / bps;

    switch (bps) {
        case 2:
            for (ch = 0; ch < channels; ++ch) {
                int shiftbits = 0, numbits = 0, i, j;

                for (j = i = 0; i < samples - 1; ++i) {
                    uint16_t step = step_table [index [ch]];

                    if (numbits < bps) {
                        shiftbits |= inbuf [(j & ~3) * channels + (ch * 4) + (j & 3)] << numbits;
                        numbits += 8;
                        j++;
                    }

                    if (shiftbits & 2)
                        pcmdata[ch] -= step * (shiftbits & 1) + (step >> 1);
                    else
                        pcmdata[ch] += step * (shiftbits & 1) + (step >> 1);

                    index[ch] += (shiftbits & 1) * 3 - 1;
                    shiftbits >>= bps;
                    numbits -= bps;

                    CLIP(index[ch], 0, 88);
                    CLIP(pcmdata[ch], -32768, 32767);
                    outbuf [i * channels + ch] = pcmdata[ch];
                }
            }

            break;

        case 3:
            for (ch = 0; ch < channels; ++ch) {
                int shiftbits = 0, numbits = 0, i, j;

                for (j = i = 0; i < samples - 1; ++i) {
                    uint16_t step = step_table [index [ch]], delta = step >> 2;

                    if (numbits < bps) {
                        shiftbits |= inbuf [(j & ~3) * channels + (ch * 4) + (j & 3)] << numbits;
                        numbits += 8;
                        j++;
                    }

                    if (shiftbits & 1) delta += (step >> 1);
                    if (shiftbits & 2) delta += step;

                    if (shiftbits & 4)
                        pcmdata[ch] -= delta;
                    else
                        pcmdata[ch] += delta;

                    index[ch] += index_table_3bit [shiftbits & 0x3];
                    shiftbits >>= bps;
                    numbits -= bps;

                    CLIP(index[ch], 0, 88);
                    CLIP(pcmdata[ch], -32768, 32767);
                    outbuf [i * channels + ch] = pcmdata[ch];
                }
            }

            break;

        case 5:
            for (ch = 0; ch < channels; ++ch) {
                int shiftbits = 0, numbits = 0, i, j;

                for (j = i = 0; i < samples - 1; ++i) {
                    uint16_t step = step_table [index [ch]], delta = step >> 4;

                    if (numbits < bps) {
                        shiftbits |= inbuf [(j & ~3) * channels + (ch * 4) + (j & 3)] << numbits;
                        numbits += 8;
                        j++;
                    }

                    if (shiftbits & 1) delta += (step >> 3);
                    if (shiftbits & 2) delta += (step >> 2);
                    if (shiftbits & 4) delta += (step >> 1);
                    if (shiftbits & 8) delta += step;

                    if (shiftbits & 0x10)
                        pcmdata[ch] -= delta;
                    else
                        pcmdata[ch] += delta;

                    index[ch] += index_table_5bit [shiftbits & 0xf];
                    shiftbits >>= bps;
                    numbits -= bps;

                    CLIP(index[ch], 0, 88);
                    CLIP(pcmdata[ch], -32768, 32767);
                    outbuf [i * channels + ch] = pcmdata[ch];
                }
            }

            break;

        default:
            return 0;
    }

    return samples;
}
