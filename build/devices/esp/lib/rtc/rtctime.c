/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

// Module for RTC time keeping

#include "rtctime_internal.h"
#include "rtctime.h"

#include "xsesp.h"

/* seconds per day */
#define SPD 24*60*60

/* days per month -- nonleap! */
static const short __spm[13] =
  { 0,
    (31),
    (31+28),
    (31+28+31),
    (31+28+31+30),
    (31+28+31+30+31),
    (31+28+31+30+31+30),
    (31+28+31+30+31+30+31),
    (31+28+31+30+31+30+31+31),
    (31+28+31+30+31+30+31+31+30),
    (31+28+31+30+31+30+31+31+30+31),
    (31+28+31+30+31+30+31+31+30+31+30),
    (31+28+31+30+31+30+31+31+30+31+30+31),
  };

static int __isleap (int year) {
  /* every fourth year is a leap year except for century years that are
   * not divisible by 400. */
  /*  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)); */
  return (!(year % 4) && ((year % 100) || !(year % 400)));
}

// ******* C API functions *************

void rtctime_early_startup (void)
{
//  Cache_Read_Enable (0, 0, 1);
  rtc_time_register_bootup ();
  rtc_time_switch_clocks ();
//  Cache_Read_Disable ();
}

void rtctime_late_startup (void)
{
  rtc_time_switch_system ();
}

void rtctime_gettimeofday (struct rtc_timeval *tv)
{
  rtc_time_gettimeofday (tv);
}

void rtctime_settimeofday (const struct rtc_timeval *tv)
{
  if (!rtc_time_check_magic ())
    rtc_time_prepare ();
  rtc_time_settimeofday (tv);
}

bool rtctime_have_time (void)
{
  return rtc_time_have_time ();
}

void rtctime_deep_sleep_us (uint32_t us)
{
  rtc_time_deep_sleep_us (us);
}

void rtctime_deep_sleep_until_aligned_us (uint32_t align_us, uint32_t min_us)
{
  rtc_time_deep_sleep_until_aligned (align_us, min_us);
}

void rtctime_gmtime (const int32 stamp, struct rtc_tm *r)
{
  int32_t i;
  int32_t work = stamp % (SPD);
  r->tm_sec = work % 60; work /= 60;
  r->tm_min = work % 60; r->tm_hour = work / 60;
  work = stamp / (SPD);
  r->tm_wday = (4 + work) % 7;
  for (i = 1970; ; ++i) {
    int32_t k = __isleap (i) ? 366 : 365;
    if (work >= k) {
      work -= k;
    } else {
      break;
    }
  }
  r->tm_year = i - 1900;
  r->tm_yday = work;

  r->tm_mday = 1;
  if (__isleap (i) && (work > 58)) {
    if (work == 59) r->tm_mday = 2; /* 29.2. */
    work -= 1;
  }

  for (i = 11; i && (__spm[i] > work); --i) ;
  r->tm_mon = i;
  r->tm_mday += work - __spm[i];
}
