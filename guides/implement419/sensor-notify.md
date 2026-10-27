---
name: Sensor Sample Notifications
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-26
---

Implementing a sensor that provides a notification when a new sample is ready is more complex than a simple [polled sensor](./sensor-poll.md). The sensor's constructor must accept an `onSample()` callback, [invoke the callback](./callbacks.md) when a new sample is available, and manage the most recent sample in `sample()`. Even if a sensor class implementation supports notifications, the caller may use polling. Therefore, the sensor must behave correctly for both.

---

The constructor should store the `onSample()` callback only if it can provide notifications. Often this requires checking for the presence of the notification interrupt's IO.

This example binds `onSample()` to the interrupt's `onReadable()` callback to deliver the notification. The `onSample()` callback may read the value using the sensor's `sample()` method.

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
}
```

---

Another way to implement the notification is to have the sensor class always read the latest sample when it is available, and then its `sample()` method returns the cached value. The cached value is cleared after being read so that no stale values are returned.

Notice that this implementation works whether or not `onSample()` and `interrupt` are provided.

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
		this.#sample = {temperature: (value >> 4) * 0.0625};
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

Sensors that support `onSample` should implement the `interrupt` property of `configuration` to allow callers to detect that notifications are supported. See the [Report Sensor Configuration](./sensor-configuration.md) guide for details.
