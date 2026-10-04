---
name: Close Connection using MQTT Client
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

To cleanly disconnect from the MQTT server, write a DISCONNECT opcode and then close the instance.

```js
const MQTTClient = device.network.mqtt.io;
const mc = new MQTTClient({
	...device.network.mqtt,
	host: "test.mosquitto.org"
});
mc.write(null, {operation: MQTTClient.DISCONNECT});
mc.close();
```

---

To immediately disconnect from the MQTT server, without a clean termination of the session, call `close()` without sending a DISCONNECT opcode.

```js
const mc = new device.network.mqtt.io({
	...device.network.mqtt,
	host: "test.mosquitto.org"
});
mc.close();
```

---

If the connection terminates, due to an error or by the remote endpoint closing it, `onError()` is invoked.

```js
const mc = new device.network.mqtt.io({
	...device.network.mqtt,
	host: "test.mosquitto.org",
	onError(e) {
		trace(`connection failed: ${e}\n`);
	}
});
```
