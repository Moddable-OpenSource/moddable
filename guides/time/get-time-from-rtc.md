---
name: Get Time and Date from Real-Time Clock
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-02
---

On devices with Real-Time Clock hardware, you can get the current time and date from the Real-Time Clock. The value returned is Unix Time in milliseconds. If the time is unavailable, for example because the Real-Time Clock has not been set, the value is `undefined`.

```js
const rtc = new device.rtc.io(device.rtc);
trace(`rtc.time ${rtc.time}\n`);
// => "rtc.time 1760390224000"
rtc.close();
```
---

You can use the time value returned by the Real-Time Clock to initialize a standard JavaScript `Date` object.

```js
const rtc = new device.rtc.io(device.rtc);
const time = rtc.time;
if (undefined !== time) {
	const date = new Date(time);
	trace(date, "\n");
	// => "Fri Nov 07 2025 14:58:11 GMT-0800"
}
rtc.close();
```

---

You can use the Real-Time Clock to set the time and date of the device. This is useful to initialize the time and date at start-up.

Note that the Real-Time Clock `time` value is milliseconds but `Time.set` expects seconds.

```js
import Time from "time";

const rtc = new device.rtc.io(device.rtc);
const time = rtc.time;
if (undefined !== time)
	Time.set(time / 1000);
rtc.close();
```
