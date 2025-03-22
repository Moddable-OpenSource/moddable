/**
 * @file set_qflash.c
 *
 * @brief This file is a simple program that sets the Quad Enable Bit on a flash chip for the
 * sparkfun rp2350 boards. Normally this is set during production, but this routine sets the
 * bit for pre-production boards or if somehow the bit is cleared.
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

#include "hardware/flash.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// Just a simple main routine that sets the Quad Enable Bit on the flash chip

int main()
{
    stdio_init_all();

    // wait a little bit - for startup
    sleep_ms(5000);
    printf("\n\nSparkFun - Check/Set the Flash Quad Enable Bit routine\n");
    sleep_ms(5000);

    // Get the command
    uint8_t cmdStatus[2] = {0x35, 0};
    uint8_t *pStatus = (uint8_t *)&cmdStatus;
    flash_do_cmd(pStatus, pStatus, 2);

    printf("Current Status Register: 0x%x\n", cmdStatus[1]);

    if ((cmdStatus[1] & 0x2) != 0x2)
    {
        printf("Quad enable bit not set - setting. Reset the board for it to take effect\n");
        sleep_ms(1000);

        // write enable to the flash chip
        cmdStatus[0] = 0x06;
        flash_do_cmd(pStatus, pStatus, 1);

        cmdStatus[0] = 0x31;
        cmdStatus[1] = cmdStatus[1] | 0x2; // cmdStatus[1] is untouched from enable command
        // cmdStatus[1] = cmdStatus[1] & ~0x2;  // use to flop the bit back to non-quad enabled

        // This will set the bit, but crash /hang this program
        flash_do_cmd(pStatus, pStatus, 2);
    }
    printf("Quad Enable Bit already set. Test complete.\n");

    while (1)
    {
        sleep_ms(1000);
    }
}
