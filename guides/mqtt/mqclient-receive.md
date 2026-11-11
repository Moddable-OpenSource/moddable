---
name: Receive Messages using MQTT Client
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-11-11
---

To receive MQTT messages, first [subscribe](./mqclient-subscribe.md) to one or more topics. Then `onReadable()` is called when messages are received. A single message may be split across several fragments, which results in multiple calls to `onReadable()` for a single message. This example reassembles fragments into a `Uint8Array`.

```js
let payload = null;
let offset = 0;

const MQTTClient = device.network.mqtt.io;
new MQTTClient({
	...device.network.mqtt,
	host: "test.mosquitto.org",
	onWritable(count) {
		if (this.once) return;
		this.once = true;

		this.write(null, {
			operation: MQTTClient.SUBSCRIBE,
			items: [
				{topic: "test/419"}
			]
		});
	},
	onReadable(count, options) {
		if (options.topic) {
			trace(`receiving message on ${options.topic}\n`);
			trace(` byteLength ${options.byteLength}\n`);
			trace(` QoS ${options.QoS}\n`);
			trace(` retain ${options.retain}\n`);
		}

		if ((null === payload) && (false === options.more))
			payload = new Uint8Array(this.read());
		else {
			payload ??= new Uint8Array(options.byteLength)
			const fragment = new Uint8Array(this.read());
			payload.set(fragment, offset);
			offset += fragment.byteLength;
			if (options.more)
				return;
		}
		trace(`payload: ${payload.toHex()}\n`);

		payload = null;
		offset = 0;
	}
});
```
