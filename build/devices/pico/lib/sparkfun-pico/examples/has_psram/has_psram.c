

/**
 * @file has_psram.c
 *
 * @brief This file contains a example on how to detect PSRAM and run a basic test
 */

/*
The MIT License (MIT)

Copyright (c) 2024 SparkFun Electronics

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions: The
above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
"AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
// include the sparkfun_pico library - copy this into this folder to run
#include "pico/stdlib.h"
#include "sparkfun_pico/sfe_pico.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Defines for the test. Note the PSRAM location is from the datasheet
#define PSRAM_LOCATION _u(0x11000000)
#define DATA_ELEMENTS 4096
#define DATA_BLOCK_SIZE (DATA_ELEMENTS * sizeof(int32_t))

static void erase_data_block(int32_t *data_buffer)
{
    for (size_t i = 0; i < DATA_ELEMENTS; i++)
        data_buffer[i] = 0xFFFFFFFF;
}
static void write_data_block(int32_t *source_data, int32_t *data_buffer, int32_t offset)
{
    for (size_t i = 0; i < DATA_ELEMENTS; i++)
        data_buffer[i] = source_data[i] + offset;
}

static bool check_data_block(int32_t *source_data, int32_t *data_buffer, int32_t offset)
{
    for (size_t i = 0; i < DATA_ELEMENTS; i++)
    {
        if (source_data[i] + offset != data_buffer[i])
        {
            printf("ERROR : [%d] != [%d]\n", source_data[i] + offset, data_buffer[i]);
            return false;
        }
    }
    return true;
}
int main()
{
    stdio_init_all();

    // wait a little bit - for startup
    sleep_ms(2000);
    printf("\n-----------------------------------------------------------\n");
    printf("SparkFun - Basic PSRAM Test - starting\n");
    sleep_ms(2000);

    size_t psram_size = sfe_setup_psram(SFE_RP2350_XIP_CSI_PIN);

    printf("PSRAM setup complete. PSRAM size 0x%lX\n", psram_size);

    if (psram_size == 0)
    {
        printf("PSRAM not detected - done\n");
        return 1;
    }

    printf("Generating Random Data\n");
    // cook up some random data
    int32_t *random_data = (int32_t *)malloc(DATA_BLOCK_SIZE);
    if (!random_data)
    {
        printf("Random data allocation failed\n");
        return 1;
    }
    for (int i = 0; i < DATA_ELEMENTS; i++)
        random_data[i] = rand();

    // How many data blocks to write - use all of PSRAM
    size_t nDataBlocks = psram_size / DATA_BLOCK_SIZE;
    printf("PSRAM data block testing - n data blocks: %d\n", nDataBlocks);

    // Write data blocks to PSRAM - then read them back and check
    for (int i = 0; i < nDataBlocks; i++)
    {
        int32_t *data_buffer = (int32_t *)(PSRAM_LOCATION + i * DATA_BLOCK_SIZE);
        erase_data_block(data_buffer);
        write_data_block(random_data, data_buffer, i * 0x2);
    }
    printf("Data blocks written\n");
    int nPassed = 0;

    for (size_t i = 0; i < nDataBlocks; i++)
    {
        if (i % 10 == 0)
        {
            printf(".");
            stdio_flush();
        }
        int32_t *data_buffer = (int32_t *)(PSRAM_LOCATION + i * DATA_BLOCK_SIZE);
        if (!check_data_block(random_data, data_buffer, i * 0x2))
            printf("Data block %d failed\n", (int)i);
        else
            nPassed++;
    }

    free(random_data);

    printf("\n\nTest Run: %d, Passed: %d, Failed: %d\n", nDataBlocks, nPassed, nDataBlocks - nPassed);

    printf("DONE\n");
    printf("-----------------------------------------------------------\n");
    while (1)
    {
        sleep_ms(1000);
    }
}
