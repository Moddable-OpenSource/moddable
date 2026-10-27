---
name: Sensor Configuration Report
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-26
---

The `configuration` property of a sensor returns an object that describes the current settings of the instance. The `configuration` property is the getter for the `configure()` method, though it may report additional properties (see `interrupt` below).

```js
class TemperatureSensor {
	#io;
	#highTemperature, #lowTemperature;

	configure(options) {
		if ("highTemperature" in options) {
			const value = (options.highTemperature /
				0.0625) << 4;
			this.#io.writeUint16(0x03, value, true);
			this.#highTemperature = options.highTemperature;
		}
		if ("lowTemperature" in options) {
			const value = (options.lowTemperature /
				0.0625) << 4;
			this.#io.writeUint16(0x02, value, true);
			this.#lowTemperature = options.lowTemperature;
		}
	}
	get configuration() {
		return {
			lowTemperature: this.#lowTemperature,
			highTemperature: this.#highTemperature,
		};
	}
}
```

---

A sensor's `configuration` can report whether or not the `onSample` callback passed to the constructor is supported by setting the `interrupt` property to `true`. If the caller passed `onSample` and finds that `!sensor.configuration?.interrupt` is `true`, it must fall back to polling.

```js
class TemperatureSensor {
	#io;
	#interrupt;

	constructor(options) {
		try {
			/* initialize #io */

			const {interrupt, onSample} = options;
			if (interrupt && onSample) {
				this.#interrupt = new interrupt.io({
					...interrupt,
					mode: interrupt.io.Input,
					edge: interrupt.io.Rising,
					onReadable: onSample.bind(this)
				});
			}
		}
		catch (e) {
			this.close();
			throw e;
		}
	}
	get configuration() {
		return {
			interrupt: !!this.#interrupt
		};
	}
}
```

---

A sensor may report information about itself, such as its manufacturer and model, using the `identification` property, also defined by the [Provenance Sensor Class Pattern](https://419.ecma-international.org/#provenance-sensor-class-pattern).

```js
class TemperatureSensor {

	get identification() {
		return {
			model: "TI TMP102",
			classification: "Temperature"
		};
	}
}
```
