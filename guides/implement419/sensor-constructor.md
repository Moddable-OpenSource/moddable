---
name: Sensor Constructor
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-26
---

The sensor's constructor is responsible for setting up communication with the sensor. This includes initializing any IO instances and probing for the sensor hardware. (See [Constructor Sets `target`](./target.md) and [Constructor Clean-up on Failure](./cleanup.md) for universal requirements of ECMA-419 constructors.)

This example initializes the SMBus interface and sends a reset command. The reset both initializes the device and confirms its presence (constructing the SMBus instance reserves the address on the bus, but does not communicate with the peripheral).

```js
class TemperatureSensor {
	#io;

	constructor(options) {
		try {
			this.#io = new options.sensor.io({
				hz: 400_000,
				address: 0x48,
				...options.sensor
			});

			this.#io.writeUint16(0x01,
				0b0110_0000_1010_0000, true);
		}
		catch (e) {
			this.close();
			throw e;
		}
	}
}
```

---

The constructor must ensure the sensor is configured to its defaults. This is necessary because should the sensor retain power between instantiations, it will retain the configuration of the previous instance. The most reliable way to restore the default state is a reset command, as above, or a reset pin, if available.

If the sensor does not have a reset operation, each individual configuration can be reset in the constructor.

```js
class TemperatureSensor {
	#io;

	constructor(options) {
		try {
			this.#io = new options.sensor.io({
				hz: 400_000,
				address: 0x48,
				...options.sensor
			});

			this.#io.writeUint16(0x03,
				0b0101_0000_0000_0000, true);
			this.#io.writeUint16(0x02,
				0b0100_1011_0000_0000, true);
		}
		catch (e) {
			this.close();
			throw e;
		}
	}
}
```

---

When resetting individual configuration options, it can be more straightforward to use the sensor's `configure()` method. This is allowed by ECMA-419.

```js
class TemperatureSensor {
	#io;

	constructor(options) {
		try {
			this.#io = new options.sensor.io({
				hz: 400_000,
				address: 0x48,
				...options.sensor
			});

			this.configure({
				lowTemperature: 75,
				highTemperature: 80
			});
		}
		catch (e) {
			this.close();
			throw e;
		}
	}
	configure(options) {
		if ("highTemperature" in options) {
			const value = (options.highTemperature / 0.0625) << 4;
			this.#io.writeUint16(0x03, value, true);
		}
		if ("lowTemperature" in options) {
			const value = (options.lowTemperature / 0.0625) << 4;
			this.#io.writeUint16(0x02, value, true);
		}
	}
}
```
