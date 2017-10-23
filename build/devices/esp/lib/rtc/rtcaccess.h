/*
 * Copyright 2015 Dius Computing Pty Ltd. All rights reserved.
 *     
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution. 
 * - Neither the name of the copyright holders nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *  
 * @author Johny Mattsson <jmattsson@dius.com.au>
 */

#ifndef RTC_ACCESS_H
#define RTC_ACCESS_H

#include <c_types.h>

#define RTC_MMIO_BASE 0x60000700
#define RTC_USER_MEM_BASE 0x60001200
#define RTC_USER_MEM_NUM_DWORDS 128
#define RTC_TARGET_ADDR 0x04
#define RTC_COUNTER_ADDR 0x1c

static inline uint32_t rtc_mem_read(uint32_t addr)
{
  return ((uint32_t*)RTC_USER_MEM_BASE)[addr];
}

static inline void rtc_mem_write(uint32_t addr, uint32_t val)
{
  ((uint32_t*)RTC_USER_MEM_BASE)[addr]=val;
}

static inline uint64_t rtc_make64(uint32_t high, uint32_t low)
{
  return (((uint64_t)high)<<32)|low;
}

static inline uint64_t rtc_mem_read64(uint32_t addr)
{
  return rtc_make64(rtc_mem_read(addr+1),rtc_mem_read(addr));
}

static inline void rtc_mem_write64(uint32_t addr, uint64_t val)
{
  rtc_mem_write(addr+1,val>>32);
  rtc_mem_write(addr,val&0xffffffff);
}

static inline void rtc_memw(void)
{
  asm volatile ("memw");
}

static inline void rtc_reg_write(uint32_t addr, uint32_t val)
{
  rtc_memw();
  addr+=RTC_MMIO_BASE;
  *((volatile uint32_t*)addr)=val;
  rtc_memw();
}

static inline uint32_t rtc_reg_read(uint32_t addr)
{
  addr+=RTC_MMIO_BASE;
  rtc_memw();
  return *((volatile uint32_t*)addr);
}
#endif
