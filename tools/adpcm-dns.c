////////////////////////////////////////////////////////////////////////////
//                           **** ADPCM-XQ ****                           //
//                  Xtreme Quality ADPCM Encoder/Decoder                  //
//                    Copyright (c) 2024 David Bryant.                    //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// adpcm-dns.c

// This module handles the implementation of "dynamic noise shaping" which is
// designed to move the spectrum of the quantization noise introduced by lossy
// compression up or down in frequency so that it is more likely to be masked
// by the source material.

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "adpcm-lib.h"

#define FILTER_LENGTH 15
#define WINDOW_LENGTH 101
#define MIN_BLOCK_SAMPLES 16

static void win_average_buffer (float *samples, int sample_count, int half_width);

// Generate the shaping values for the specified buffer of stereo or mono samples,
// one shaping value output for each sample (or stereo pair of samples). This is
// calculated by filtering the audio at fs/6 (7350 Hz at 44.1 kHz) and comparing
// the averaged levels above and below that frequency. The output shaping values
// are nominally in the range of +/-1024, with 1024 indicating first-order HF boost
// shaping and -1024 for similar LF boost. However, since -1024 would result in
// infinite DC boost (not useful) a "min_value" is passed in. An output value of
// zero represents no noise shaping. For stereo input data the channels are summed
// for the calculation and the output is still just mono. Note that at the ends of
// the buffer the values diverge from true because not all the required source
// samples are visible. Use this formula to calculate the number of samples
// required for this process to "settle":
//
//  int settle_distance = (WINDOW_LENGTH >> 1) + (FILTER_LENGTH >> 1) + 1;
//
// We also pass in a "last_value" so that we can smoothly interpolate from that
// to the first calculated value during the initial "unknown" samples. This
// reduces discontinuities.

void generate_dns_values (const int16_t *samples, int sample_count, int num_chans, int sample_rate,
    int16_t *values, int16_t min_value, int16_t last_value)
{
    float dB_offset = 7.3, dB_scaler = 64.0, max_dB, min_dB, max_ratio, min_ratio;
    int filtered_count = sample_count - FILTER_LENGTH + 1, i;
    float *low_freq, *high_freq;

    memset (values, 0, sample_count * sizeof (values [0]));

    if (filtered_count <= 0)
        return;

    low_freq = malloc (filtered_count * sizeof (float));
    high_freq = malloc (filtered_count * sizeof (float));

    // First, directly calculate the lowpassed audio using the 15-tap filter. This is
    // a basic sinc with Hann windowing (for a fast transition) and because the filter
    // is set to exactly fs/6, some terms are zero (which we can skip). Also, because
    // it's linear-phase and has an odd number of terms, we can just subtract the LF
    // result from the original to get the HF values.

    if (num_chans == 1)
        for (i = 0; i < filtered_count; ++i, ++samples) {
            float filter_sum =
                ((int32_t) samples [0] + samples [14]) *  0.00150031 +
                ((int32_t) samples [2] + samples [12]) * -0.01703392 +
                ((int32_t) samples [3] + samples [11]) * -0.03449186 +
                ((int32_t) samples [5] + samples [ 9]) *  0.11776258 +
                ((int32_t) samples [6] + samples [ 8]) *  0.26543272 +
                         (int32_t) samples [7]         *  0.33366033;

            high_freq [i] = samples [FILTER_LENGTH >> 1] - filter_sum;
            low_freq [i] = filter_sum;
        }
    else
        for (i = 0; i < filtered_count; ++i, samples += 2) {
            float filter_sum =
                ((int32_t) samples [ 0] + samples [ 1] + samples [28] + samples [29]) *  0.00150031 +
                ((int32_t) samples [ 4] + samples [ 5] + samples [24] + samples [25]) * -0.01703392 +
                ((int32_t) samples [ 6] + samples [ 7] + samples [22] + samples [23]) * -0.03449186 +
                ((int32_t) samples [10] + samples [11] + samples [18] + samples [19]) *  0.11776258 +
                ((int32_t) samples [12] + samples [13] + samples [16] + samples [17]) *  0.26543272 +
                               ((int32_t) samples [14] + samples [15])                *  0.33366033;

            high_freq [i] = samples [FILTER_LENGTH & ~1] + samples [FILTER_LENGTH] - filter_sum;
            low_freq [i] = filter_sum;
        }

    // Apply a simple first-order "delta" filter to the lowpass because frequencies below fs/6
    // become progressively less important for our purposes as the decorrelation filters make
    // those frequencies less and less relevant. Note that after all this filtering, the
    // magnitude level of the high frequency array will be 8.7 dB greater than the low frequency
    // array when the filters are presented with pure white noise (determined empirically).

    for (i = filtered_count - 1; i; --i)
        low_freq [i] -= low_freq [i - 1];

    low_freq [0] = low_freq [1];    // simply duplicate for the "unknown" sample

    // Next we determine the averaged (absolute) levels for each sample using a box filter.

    win_average_buffer (low_freq, filtered_count, WINDOW_LENGTH >> 1);
    win_average_buffer (high_freq, filtered_count, WINDOW_LENGTH >> 1);

    // calculate the minimum and maximum ratios that won't be clipped so that we only
    // have to compute the logarithm when needed

    max_dB = 1024 / dB_scaler - dB_offset;
    min_dB = min_value / dB_scaler - dB_offset;
    max_ratio = pow (10.0, max_dB / 20.0);
    min_ratio = pow (10.0, min_dB / 20.0);

    for (i = 0; i < filtered_count; ++i)
        if (high_freq [i] > 1.0 && low_freq [i] > 1.0) {
            float ratio = high_freq [i] / low_freq [i];
            int shaping_value;

            if (ratio >= max_ratio)
                shaping_value = 1024;
            else if (ratio <= min_ratio)
                shaping_value = min_value;
            else
                shaping_value = (int) floor ((log10 (ratio) * 20.0 + dB_offset) * dB_scaler + 0.5);

            values [i + (FILTER_LENGTH >> 1)] = shaping_value;
        }

    // interpolate the first 7 values from the supplied "last_value" to the first new value

    for (i = 0; i < FILTER_LENGTH >> 1; ++i)
        values [i] =
            (
                (int32_t) values [FILTER_LENGTH >> 1] * (i + 1)     +
                (int32_t) last_value * ((FILTER_LENGTH >> 1) - i)   +
                (FILTER_LENGTH >> 2)
            ) / ((FILTER_LENGTH >> 1) + 1);

    // finally, copy the value at the end into the 7 final positions because unfortunately
    // we have no "next_value" to interpolate with

    for (i = filtered_count + (FILTER_LENGTH >> 1); i < sample_count; ++i)
        values [i] = values [(FILTER_LENGTH >> 1) + filtered_count - 1];

    free (low_freq);
    free (high_freq);
}

// Given a buffer of floating values, apply a simple box filter of specified half width
// (total filter width is always odd) to determine the averaged magnitude at each point.
// For the ends, we use only the visible samples.

static void win_average_buffer (float *samples, int sample_count, int half_width)
{
    float *output = malloc (sample_count * sizeof (float));
    double sum = 0.0;
    int m = 0, n = 0;
    int i, j, k;

    for (i = 0; i < sample_count; ++i) {
        k = i + half_width + 1;
        j = i - half_width;

        if (k > sample_count) k = sample_count;
        if (j < 0) j = 0;

        while (m < j) {
            if ((sum -= samples [m] * samples [m]) < 0.0) sum = 0.0;
            m++;
        }

        while (n < k) {
            sum += samples [n] * samples [n];
            n++;
        }

        output [i] = sqrt (sum / (n - m));
    }

    memcpy (samples, output, sample_count * sizeof (float));
    free (output);
}
