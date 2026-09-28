/*---
description: >
  getTimezoneOffset returns the local offset at the limits of the time value
  range. fxDateSplit (xsDate.c) computed the offset by converting the local
  date back to a time value, which is NaN beyond the limits, then cast NaN to
  txInteger. The result is compared with the same date 400 years (146097
  days) away, which has the same calendar and the same local time rules in XS.
flags: [onlyStrict]
---*/

var cycle = 146097 * 86400000;
var max = new Date(8.64e15);
var min = new Date(-8.64e15);
assert.sameValue(max.getTimezoneOffset(), new Date(8.64e15 - cycle).getTimezoneOffset(), "latest time value");
assert.sameValue(min.getTimezoneOffset(), new Date(-8.64e15 + cycle).getTimezoneOffset(), "earliest time value");
assert.sameValue(max.getHours(), new Date(8.64e15 - cycle).getHours(), "local hours at the latest time value");
