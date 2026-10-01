---
name: Time
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

Embedded JavaScript projects work with time in several ways, and these guides cross a range of APIs — from the standard JavaScript `Date` object to Moddable SDK modules and ECMA-419 device drivers.

- The standard [`Date`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date) object provides the current date and time of day.
- The Moddable SDK [`Time`](../../documentation/base/base.md#time) and [`Timer`](../../documentation/base/base.md#timer) modules provide system time, high-resolution timing, and short delays.
- The ECMA-419 [Real-Time Clock](https://419.ecma-international.org/#real-time-clock-class-pattern) and [NTP Client](https://419.ecma-international.org/#ntp-client) read time from hardware and the network.

## Date and Time of Day

- [Get Unix Time](./get-unix-time.md)
- [Get Time of Day](./get-time-of-day.md)
- [Get Date](./get-date.md)

## System Time

- [Get Time Since System Start](./get-time-since-system-start.md)
- [Get Microseconds](./get-microseconds.md)
- [Set System Date and Time](./set-system-date-time.md)
- [How Time and Timezone are Initialized](./how-time-initialized.md)

## Real-Time Clock

- [Get Time and Date from Real-Time Clock](./get-time-from-rtc.md)
- [Set Real-Time Clock Time](./set-rtc-time.md)

## Network Time

- [Get Time and Date from Network](./get-time-from-ntp.md)

## Sleep

- [Sleep](./sleep.md)

## Building

The standard JavaScript `Date` object and the Moddable SDK `Time` and `Timer` modules are part of every Moddable SDK build and require no manifest changes. `Date` is a global; import `Time` and `Timer` in your JavaScript source code:

```js
import Time from "time";
import Timer from "timer";
```

The remaining features require additional modules:

| Feature | Import | Manifest |
|---------|--------|----------|
| Microsecond precision (`Time.microseconds`) | none (adds `microseconds` to `Time`) | `$(MODDABLE)/modules/base/microseconds/manifest.json` |
| Network Time (`device.network.ntp.client`) | `embedded:network/ntp/client` | `$(MODDABLE)/examples/io/udp/ntp/manifest_ntp.json` |

When building with `mcconfig`, add the manifests needed to your project.

When building with `mcpack`, either add the corresponding manifest to your project or use the needed modules in `import` statements. If your project accesses Network Time through `device.network.ntp.client`, `mcpack` includes the necessary manifests automatically.


## Learn More

- Documentation
	- [Date](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date) on MDN
	- [class Time](../../documentation/base/base.md#time) in Moddable SDK
	- [class Timer](../../documentation/base/base.md#timer) in Moddable SDK
- Example
	- [NTP example](../../examples/io/udp/ntp/main.js) in Moddable SDK
- Standards
	- [Date Objects](https://tc39.es/ecma262/#sec-date-objects) in ECMAScript
	- [Real-Time Clock Class Pattern](https://419.ecma-international.org/#real-time-clock-class-pattern) in ECMA-419
	- [NTP Client](https://419.ecma-international.org/#ntp-client) in ECMA-419
- TypeScript Declarations
	- [class Time](../../typings/time.d.ts) in Moddable SDK
	- [class Timer](../../typings/timer.d.ts) in Moddable SDK
	- [Real-Time Clock](../../typings/embedded/RTC.d.ts) in Moddable SDK
	- [NTP Client](../../typings/embedded_network/ntp/client.d.ts) in Moddable SDK
