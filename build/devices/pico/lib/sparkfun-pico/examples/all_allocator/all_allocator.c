
/**
 * @file test_allocator.c
 *
 * @brief This file contains a example on how to use a memory allocator with PSRAM and SRAM on a rp2350
 *
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
#include "pico/stdlib.h"
#include "sparkfun_pico/sfe_pico.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_cpp.h"

// main()

static void memory_stats()
{
    size_t mem_size = sfe_mem_size();
    size_t mem_used = sfe_mem_used();
    printf("\tMemory pool - Total: 0x%X (%u)  Used: 0x%X (%u) - %3.2f%%\n", mem_size, mem_size, mem_used, mem_used,
           (float)mem_used / (float)mem_size * 100.0);

    size_t max_block = sfe_mem_max_free_size();
    printf("\tMax free block size: 0x%X (%u) \n", max_block, max_block);
}
int main()
{
    stdio_init_all();

    // wait a little bit - for startup
    sleep_ms(2000);
    printf("\n-----------------------------------------------------------\n");
    printf("SparkFun - Allocator test - starting\n");
    printf("-----------------------------------------------------------\n");
    sleep_ms(2000);

    sfe_pico_alloc_init();
    printf("Startup\n");

    memory_stats();

    // Allocate a Meg
    uint8_t *big_block = (uint8_t *)sfe_mem_malloc(1024 * 1024);
    if (!big_block)
    {
        printf("Big block allocation failed\n");
        return 1;
    }
    printf("\nAllocated a Meg using sfe_alloc\n");
    memory_stats();
    sfe_mem_free(big_block);

    printf("\nFreed a Meg using sfe_free\n");
    memory_stats();

// Wrapping the built in malloc/free
#if defined(SFE_PICO_ALLOC_WRAP)
    // Now with built ins -- did we override ?
    // Allocate a Meg
    big_block = (uint8_t *)malloc(1024 * 1024);
    if (!big_block)
    {
        printf("Big block built in allocation failed\n");
        return 1;
    }
    printf("\nAllocated a Meg using built in alloc\n");
    memory_stats();
    free(big_block);

    printf("\nFreed a Meg using built in free\n");
    memory_stats();

    // now c++ tests
    printf("\nAllocate a C++ object w/ 1Meg buffer\n");
    void *cpp_ptr = test_cpp_new();
    if (!cpp_ptr)
    {
        printf("C++ allocation failed\n");
        return 1;
    }

    memory_stats();

    printf("\nDelete a C++ object w/ 1Meg buffer\n");
    test_cpp_delete(cpp_ptr);
    memory_stats();

#endif
    printf("-----------------------------------------------------------\n");
    printf("<<Testing Complete>>\n");
    printf("-----------------------------------------------------------\n");
    while (1)
    {
        sleep_ms(1000);
    }
}
