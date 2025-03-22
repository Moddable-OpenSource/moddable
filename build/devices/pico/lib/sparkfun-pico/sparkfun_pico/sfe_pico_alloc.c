/**
 * @file sfe_pico_alloc.c
 *
 * @brief Implementation the pico allocator - enables an allocator to use with PSRAM
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
// #include "hardware/flash.h"
#include "hardware/address_mapped.h"
#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/regs/addressmap.h"
#include "hardware/spi.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip_ctrl.h"
#include "hardware/sync.h"
#include "pico/binary_info.h"
#include "pico/flash.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// the psram setup routine
#include "sfe_pico_boards.h"
#include "sfe_psram.h"

// our allocator - tlsf
// #include "tlsf/tlsf.h"
#include "tlsf.h"

#include "sfe_pico_alloc.h"

static tlsf_t _mem_heap = NULL;
static pool_t _mem_sram_pool = NULL;
static pool_t _mem_psram_pool = NULL;

static size_t _psram_size = 0;

static bool _bInitalized = false;

#if defined(SFE_PICO_ALLOC_WRAP)
static bool _bUseHeapPool = true;
#else
static bool _bUseHeapPool = false;
#endif

//-------------------------------------------------------------------------------
// Allocator Routines
//-------------------------------------------------------------------------------

// Location /address where PSRAM starts on the RP2350
// TODO - is there a define in the pico sdk for this?
#define PSRAM_LOCATION _u(0x11000000)

// Internal function that init's the allocator

// TODO: Make this configurable - heap or not via a parameter. Default will have this key off the #define

bool sfe_pico_alloc_init(void)
{
    if (_bInitalized)
        return true;

    _mem_heap = NULL;
    _mem_sram_pool = NULL;
    _mem_psram_pool = NULL;

#ifndef SFE_RP2350_XIP_CSI_PIN
    printf("PSRAM CS pin not defined - check board file or specify board on build. unable to use PSRAM\n");
    _psram_size = 0;
#else
    // Setup PSRAM if we have it.
    _psram_size = sfe_setup_psram(SFE_RP2350_XIP_CSI_PIN);
#endif
    // printf("PSRAM size: %u\n", _psram_size);
    if (!_bUseHeapPool)
    {
        if (_psram_size > 0)
        {
            _mem_heap = tlsf_create_with_pool((void *)PSRAM_LOCATION, _psram_size, 64 * 1024 * 1024);
            _mem_psram_pool = tlsf_get_pool(_mem_heap);
        }
    }
    else
    {
        // First, our sram pool. External heap symbols from rpi pico-sdk
        extern uint32_t __heap_start;
        extern uint32_t __heap_end;
        // size
        size_t sram_size = (size_t)(&__heap_end - &__heap_start) * sizeof(uint32_t);
        // printf("point 2 start: %x, end %x, size %X %u\n", &__heap_start, &__heap_end, sram_size, sram_size);

        _mem_heap = tlsf_create_with_pool((void *)&__heap_start, sram_size, 64 * 1024 * 1024);
        _mem_sram_pool = tlsf_get_pool(_mem_heap);

        if (_psram_size > 0)
            _mem_psram_pool = tlsf_add_pool(_mem_heap, (void *)PSRAM_LOCATION, _psram_size);
    }
    _bInitalized = true;
    return true;
}

// Our allocator interface -- same signature as the stdlib malloc/free/realloc/calloc

void *sfe_mem_malloc(size_t size)
{
    if (!sfe_pico_alloc_init() || !_mem_heap)
        return NULL;
    return tlsf_malloc(_mem_heap, size);
}

void sfe_mem_free(void *ptr)
{
    if (!sfe_pico_alloc_init() || !_mem_heap)
        return;
    tlsf_free(_mem_heap, ptr);
}

void *sfe_mem_realloc(void *ptr, size_t size)
{
    if (!sfe_pico_alloc_init() || !_mem_heap)
        return NULL;
    return tlsf_realloc(_mem_heap, ptr, size);
}

void *sfe_mem_calloc(size_t num, size_t size)
{
    if (!sfe_pico_alloc_init() || !_mem_heap)
        return NULL;
    void *ptr = tlsf_malloc(_mem_heap, num * size);
    if (ptr)
        memset(ptr, 0, num * size);
    return ptr;
}

static bool max_free_walker(void *ptr, size_t size, int used, void *user)
{
    size_t *max_size = (size_t *)user;
    if (!used && *max_size < size)
    {
        *max_size = size;
    }
    return true;
}
size_t sfe_mem_max_free_size(void)
{
    if (!sfe_pico_alloc_init() || !_mem_heap)
        return 0;
    size_t max_free = 0;

    // walk our pools
    if (_mem_sram_pool)
        tlsf_walk_pool(_mem_sram_pool, max_free_walker, &max_free);

    if (_mem_psram_pool)
        tlsf_walk_pool(_mem_psram_pool, max_free_walker, &max_free);

    return max_free;
}

static bool memory_size_walker(void *ptr, size_t size, int used, void *user)
{
    *((size_t *)user) += size;
    return true;
}

size_t sfe_mem_size(void)
{
    if (!sfe_pico_alloc_init() || !_mem_heap)
        return 0;
    size_t total_size = 0;

    // walk our pools
    if (_mem_sram_pool)
        tlsf_walk_pool(_mem_sram_pool, memory_size_walker, &total_size);

    if (_mem_psram_pool)
        tlsf_walk_pool(_mem_psram_pool, memory_size_walker, &total_size);

    return total_size;
}

static bool memory_used_walker(void *ptr, size_t size, int used, void *user)
{
    if (used)
        *((size_t *)user) += size;
    return true;
}

size_t sfe_mem_used(void)
{
    if (!sfe_pico_alloc_init() || !_mem_heap)
        return 0;
    size_t total_size = 0;

    // walk our pools
    if (_mem_sram_pool)
        tlsf_walk_pool(_mem_sram_pool, memory_used_walker, &total_size);

    if (_mem_psram_pool)
        tlsf_walk_pool(_mem_psram_pool, memory_used_walker, &total_size);

    return total_size;
}

// Wrappers for the standard malloc/free/realloc/calloc routines - set the wrapper functions
// in the cmake file ...
#if defined(SFE_PICO_ALLOC_WRAP)
void *__wrap_malloc(size_t size)
{
    return sfe_mem_malloc(size);
}
void __wrap_free(void *ptr)
{
    sfe_mem_free(ptr);
}
void *__wrap_realloc(void *ptr, size_t size)
{
    return sfe_mem_realloc(ptr, size);
}
void *__wrap_calloc(size_t num, size_t size)
{
    return sfe_mem_calloc(num, size);
}
#endif
