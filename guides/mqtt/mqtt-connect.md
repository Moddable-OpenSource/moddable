---
name: Connect to MQTT Server using MQTT()
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

To connect to an MQTT server, pass the "mqtt:" URL to the `connect()` method.

```js
import * as mqtt from "mqtt/js";

const mc = mqtt.connect("mqtt://broker.hivemq.com");
```

---

Specify the username and password using the options object argument.

```js
import * as mqtt from "mqtt/js";

const mc = mqtt.connect("mqtt://broker.hivemq.com", {
	username: "mqtt user",
	password: "secret"
});
```

---

Configure the keep-alive interval (in seconds) using the options object. You can also configure the interval to wait (in milliseconds) before attempting to reconnect a dropped connection and whether to automatically resubscribe upon reconnection.

```js
import * as mqtt from "mqtt/js";

const mc = mqtt.connect("mqtt://broker.hivemq.com", {
	keepalive: 60,
	reconnectPeriod: 5000,
	resubscribe: true
});
```

---

Specify a [will](https://www.hivemq.com/blog/mqtt-essentials-part-9-last-will-and-testament/) to send when the server detects unexpected connection termination.

```js
import * as mqtt from "mqtt/js";

const mc = mqtt.connect("mqtt://broker.hivemq.com", {
	will: {
		topic: "service/status",
		payload: ArrayBuffer.fromString("gone!"),
		qos: 0,
		retain: false
	}
});
```

---

Connecting to an MQTT server is asynchronous. To know when the connection is established, listen for the `connect` event.

```js
import * as mqtt from "mqtt/js";

const mc = mqtt.connect("mqtt://broker.hivemq.com");
mc.addEventListener("connect", () => {
	trace(`connected\n`);
});
```

---

If the MQTT connection drops with auto-reconnect disabled, use `reconnect()` to reestablish the connection. One use of this is to defer reconnection until a Wi-Fi connection is available.

```js
import * as mqtt from "mqtt/js";
import Timer from "timer";

const mc = mqtt.connect("mqtt://broker.hivemq.com", {
	reconnectPeriod: 0
});
mc.addEventListener("error", () => {
	Timer.set(() => {
		mc.reconnect();
	}, 10_000);
});
```
