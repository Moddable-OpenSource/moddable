/**
 * @file sfe_pico_alloc.h
 *
 * @brief Header file for the pico allocator - enables an allocator to use with PSRAM
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

#ifndef _SFE_PICO_ALLOC_H_
#define _SFE_PICO_ALLOC_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif
    void *sfe_mem_malloc(size_t size);
    void sfe_mem_free(void *ptr);
    void *sfe_mem_realloc(void *ptr, size_t size);
    void *sfe_mem_calloc(size_t num, size_t size);
    size_t sfe_mem_max_free_size(void);
    size_t sfe_mem_size(void);
    size_t sfe_mem_used(void);
    bool sfe_pico_alloc_init();

#if defined(SFE_PICO_ALLOC_WRAP)
    // c allocator wrappers - define for use as a wrapper if specified
    void *__wrap_malloc(size_t size);
    void __wrap_free(void *ptr);
    void *__wrap_realloc(void *ptr, size_t size);
    void *__wrap_calloc(size_t num, size_t size);
#endif
#ifdef __cplusplus
}
#endif

#endif