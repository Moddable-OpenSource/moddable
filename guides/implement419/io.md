---
name: Constructor IO
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-08-04
---

Many common ECMA-419 classes, like sensors, displays, real-time clocks, and network protocols, use hardware resources. As a rule, implementations of these classes do not import the IO resources directly. Instead, the caller passes the IO constructor and constructor options object to the class. This indirection adds a little complexity but has several benefits.

- Caller can provide precisely the IO options it wants
- IO expanders are supported automatically
- Minimizes import dependencies of implementation
- Supports testing with IO mocks
- Ensures maximum portability of the code across boards and MCUs

---

Many sensors communicate using I²C. This example shows instantiating the I²C instance from the constructor's options object, along with a GPIO for interrupts. The `sensor` and `interrupt` properties include the constructor options together with the constructor on `sensor.io` and `interrupt.io`.

The implementation amends the options for both. In the case of I²C it provides default values for `hz` and `address` which the caller can override. For the GPIO interrupt, the `target`, `edge`, and `onReadable` callback are overridden by the implementation to be the values required for correct operation. Both behaviors are valid.

```js
class Sensor {
	#i2c;
	#interrupt;

	constructor(options) {
		const {sensor, interrupt} = options;
		this.#i2c = new sensor.io({
			hz: 100_000,
			address: 0x38,
			...sensor
		});
		this.#interrupt = new interrupt.io({
			...interrupt,
			target: this,
			edge: interrupt.io.Rising,
			onReadable() {
				/* handle interrupt */
			}
		});
	}
}
```

---

While providing the IO in the constructor options object most common, there is another common approach: the [host provider instance](https://419.ecma-international.org/#host-provider-instance) constructs the class instead. This is convenient for components built into the board. It allows developers to "just use" the component without any knowledge of the hardware IO connections.

The caller instantiates the class through the `device` global and the host takes care of the rest.

Note that in this approach the actual implementation, of the temperature sensor in this case, does receive its from the IO options object, this is invisible to the developer using the class.

```js
/* caller */
const temp = new device.sensor.Temperature({});

/* implementation */
import Temperature from "embedded:sensor/Temperature/TMP117";

device.sensor.Temperature = class {
	constructor() {
		return new Temperature({
			sensor: {
				...device.i2c.default,
				io: device.io.SMBus,
				address: 0x49
			}
		});
	}
};
```
