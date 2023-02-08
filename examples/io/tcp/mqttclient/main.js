/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Timer from "timer";

const MQTTClient = device.network.mqtt.io;
const mqtt = new MQTTClient({
	...device.network.mqtt,
	host: "test.mosquitto.org",
	port: 1883,
	id: "ecma-419 mqtt test",
	keepalive: 60_000,		// keepalive is optional for MQTT, but a non-zero value is required by test.mosquitto.org!
	will: {
		topic: "foo/bar/will",
		message: ArrayBuffer.fromString("born @ " + Date()),
		QoS: 0,
		retain: 0
	},
	onControl(msg) {
		if (MQTTClient.CONNACK === msg.operation) {
			this.write(null, {
				operation: MQTTClient.SUBSCRIBE,
				items: [
					{topic: "foo/bar/+", QoS: 2},
					{topic: "bar/foo"}
				]
			});
			this.write(null, {
				operation: MQTTClient.UNSUBSCRIBE,
				items: [
					{topic: "frogs"},
					{topic: "bar/foo"}
				]
			});
			this.write(ArrayBuffer.fromString("SM"), {
				topic: "foo/bar/small",
				byteLength: 6,
				QoS: 2
			});
			this.write(ArrayBuffer.fromString("ALL\n"));
			this.write(null, {operation: MQTTClient.PINGREQ});

			this.remain = 16 * 1024;
			this.counter = 0;
			this.write(ArrayBuffer.fromString("XXL"), {
				topic: "foo/bar/xxl",
				byteLength: 3 + this.remain,
				QoS: 1,
				id: 1234
			});
		}
		if (MQTTClient.PUBACK === msg.operation) {
			trace(`PUBACK - ${msg.id}\n`);
			if (1234 == msg.id) {
//				this.write(null, {operation: MQTTClient.DISCONNECT});
//				this.close();
			}
		}
		if (MQTTClient.PINGRESP === msg.operation)
			trace("Ping response\n");
		if (MQTTClient.PUBREC === msg.operation)
			trace(`PUBREC - ${msg.id}\n`);
		if (MQTTClient.PUBREL === msg.operation)
			trace(`PUBREL - ${msg.id}\n`);
		if (MQTTClient.PUBCOMP === msg.operation)
			trace(`PUBCOMP - ${msg.id}\n`);
		if (MQTTClient.SUBACK === msg.operation)
			trace(`SUBACK - ${msg.id}\n`);
		if (MQTTClient.UNSUBACK === msg.operation)
			trace(`UNSUBACK - ${msg.id}\n`);
	},
	onReadable(count, options) {
		trace(String.fromArrayBuffer(this.read(count)) + "\n");
	},
	onWritable(count) {
		if (!this.remain)
			return;

		let repeat = (count < this.remain) ? count : this.remain;
		this.remain -= repeat;

		this.counter += 1; 
		this.write(ArrayBuffer.fromString(String.fromCharCode(64 + this.counter).repeat(repeat)));
	},
	onError() {
		trace("ERROR\n");
	}
});
