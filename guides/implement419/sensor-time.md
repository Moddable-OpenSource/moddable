---
name: Sensor Sample Time
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-26
---

The time a sample was captured can be essential information when analyzing the sample, for example to recognize user gestures from accelerometers or touch panels. Because the sample may be captured before `sample()` is called, there is no way the caller can apply an accurate timestamp. Therefore, the [Provenance Sensor Class Pattern](https://419.ecma-international.org/#provenance-sensor-class-pattern-identification-property-properties-of-sample-object) defines the `time` property on a sample to indicate when the sample was captured.

The following example is modified from [Sensor Sample Notifications](./sensor-notify.md) to record the `time` the sample is collected in `#getSample()`. This works for both notification and polling.

```js
class TemperatureSensor {
	#io;
	#interrupt;
	#onSample;
	#sample;

	constructor(options) {
		try {
			/* initialize #io */

			const {interrupt, onSample} = options;
			if (interrupt) {
				this.#interrupt = new interrupt.io({
					...interrupt,
					mode: interrupt.io.Input,
					edge: interrupt.io.Rising,
					target: this,
					onReadable() {
						this.target.#getSample();
						this.target.#onSample?.();
					}
				});
				this.#onSample = onSample;
			}
		}
		catch (e) {
			this.close();
			throw e;
		}
	}
	#getSample() {
		const value = this.#io.readUint16(0x00, true);
		this.#sample = {
			temperature: (value >> 4) * 0.0625,
			time: Date.now()
		};
	}
	sample() {
		if (!this.#interrupt)
			this.#getSample();
		const result = this.#sample;
		this.#sample = undefined;
		return result;
	}
}
```

---

Some sensor components report timing data that allows an implementation to determine the timestamp with greater accuracy. When this timestamp is available, it is preferred over a timestamp from the host.

Some applications require timestamp resolution higher than milliseconds, the default in JavaScript. While timestamps are in units of milliseconds, they are stored in JavaScript Numbers, which are double precision floating point. JavaScript Numbers easily accommodate fractional values, which allow for higher resolution timing information. (Note that fractional time values passed to `new Date` are treated as integers.)
