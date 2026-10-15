---
name: MQTT
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-14
---

The high-level `MQTT()` API, modeled on the popular [MQTT.js](https://github.com/mqttjs/MQTT.js) library, and the low-level ECMA-419 embedded standard [MQTT Client](https://419.ecma-international.org/#mqtt-client-class-pattern) APIs are both available to Embedded JavaScript developers using the Moddable SDK. In fact, `MQTT()` is implemented using MQTT Client.

## Using MQTT()

- [Connect to Server](./mqtt-connect.md)
- [Connect Securely to Server](./mqtt-connect-securely.md)
- [Close Connection](./mqtt-close.md)
- [Publish Message](./mqtt-publish.md)
- [Subscribe to Topic](./mqtt-subscribe.md)
- [Receive Message](./mqtt-receive.md)
- [Connection Information](./mqtt-connection-info.md)

## Using ECMA-419 MQTT Client

- [Connect to Server](./mqclient-connect.md)
- [Connect Securely to Server](./mqclient-connect-securely.md)
- [Close Connection](./mqclient-close.md)
- [Publish Message](./mqclient-publish.md)
- [Subscribe to Topic](./mqclient-subscribe.md)
- [Receive Message](./mqclient-receive.md)

## How to Choose

If `MQTT()` works for your project, you should use it. You can always start with `MQTT()` and switch to MQTT Client later. With `MQTT()` you'll usually write less code. Because `MQTT()` follows the widely-used MQTT.js API, many developers are already familiar with it. It also provides conveniences such as automatic reconnection, message queuing before the connection is established, and automatic resubscription.

Situations where `MQTT()` might not be the best choice include when resources like RAM and ROM are very constrained, when you have large messages, and when you need precise control over the buffering of messages.

Using MQTT Client requires more code and a deeper understanding of the MQTT protocol. But, it offers real advantages:

- Lower runtime overhead
- Send and receive messages of unlimited size
- Smaller firmware size

## Building with mcconfig

Include the `MQTT()` manifest in your project's `manifest.json`:

	$(MODDABLE)/modules/web/mqtt

Then, import the module in your JavaScript source code:

```js
import * as mqtt from "mqtt/js";
```

To use the MQTT Client instead, include its manifest. Including the manifest initializes the host provider `device.network.mqtt`:

	$(MODDABLE)/modules/io/mqtt/client

For secure connections over MQTTS, include the secure MQTT Client manifest instead. It initializes the host provider `device.network.mqtts`:

	$(MODDABLE)/modules/io/mqtts/client

## Building with mcpack

Use `mqtt/js` in an import statement and its manifest is automatically included.

If your project accesses the MQTT Client through `device.network.mqtt` or `device.network.mqtts`, `mcpack` includes the necessary manifests automatically.

## Learn More

- Documentation
	- [MQTT.js](https://github.com/mqttjs/MQTT.js) on GitHub
- Standards
	- [MQTT Client](https://419.ecma-international.org/#mqtt-client-class-pattern) in ECMA-419
	- [MQTT protocol specification Version 3.1.1](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html)
- Examples
	- [MQTT() example](../../examples/io/tcp/mqtt/main.js) in Moddable SDK
	- [MQTT Client example](../../examples/io/tcp/mqttclient/main.js) in Moddable SDK
- Implementations
	- [MQTT()](../../modules/web/mqtt/mqtt.js) in Moddable SDK
	- [MQTT Client](../../modules/io/mqtt/client/mqttclient.js) in Moddable SDK
- TypeScript Declarations
	- [MQTT()](../../typings/mqtt/js.d.ts) in Moddable SDK
	- [MQTT Client](../../typings/embedded_network/mqtt/client.d.ts) in Moddable SDK
