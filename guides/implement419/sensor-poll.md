---
name: Poll Sensor
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-26
---

Polling is how samples are retrieved from sensors that do not provide notification of new samples.

Sensor classes should not implement polling to provide notification of new samples. When polling is necessary, the caller should do it, which can be more efficient and keeps sensor class implementations simple.

```js
class TemperatureSensor {
	#io;

	constructor(options) {
		/* details omitted */
	}
	sample() {
		const value = this.#io.readUint16(0x00, true);
		return {temperature: (value >> 4) * 0.0625};
	}
}
```

---

Some sensors need time to prepare a new sample, but polling may occur faster than the sensor can provide a sample. When no sample is available, `sample()` returns `undefined`. ECMA-419 requires this behavior, instead of returning the last valid sample value, because the caller doesn't receive stale data and the behavior matches the IO Class Pattern's `read()` when no data is available.

```js
class TemperatureSensor {
	#io;

	constructor(options) {
		/* details omitted */
	}
	sample() {
		const status = this.#io.readUint16(0x01, true);
		if (status & 0b0000_0001_0000_0000)
			return;	// returns undefined

		const value = this.#io.readUint16(0x00, true);
		return {temperature: (value >> 4) * 0.0625};
	}
}
```
