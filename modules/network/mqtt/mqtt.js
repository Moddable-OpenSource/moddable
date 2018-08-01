/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import {Client as WSClient} from "websocket";

/*
 * Implements a basic MQTT client. Upon creation of a client instance, provides methods for
 * subscribing and unsubscribing to topics, publishing messages to topics, and pinging if you really
 * want to. Currently provides callbacks only for connection-ready, messages received on subscribed
 * channels, and closing.
 *
 * Currently supports only MQTT over websockets; native/direct connections are not currently
 * supported.
 *
 *     import {Client} from "mqtt";
 *     let client = new Client({host: hostname, port: port, path: "/", id: "myclient"});
 *     client.onReady = () => { trace('connection up\n'); client.subscribe("/foo"); };
 *     client.onMessage = (t, b) => { trace(`received message on ${t} with body ${String.fromArrayBuffer(b)}\n`); };
 *     client.onClose = () => { trace(`server closed connection\n`); };
 *     client.publish("/foo", "bar");
 *
 * See mqtt.c for details on exactly which MQTT features are supported. In general, this
 * implementation is QoS 0 only.
 */

export default class Client {
	constructor(params) {
		if (!params.id)
			throw new Error("parameter id is required");

		if (!params.host)
			throw new Error("parameter host is required");

		this.connect = {id: params.id};
		if (params.user)
			this.connect.user = params.user;
		if (params.password)
			this.connect.password = params.password;

		this.host = params.host;

		// set default callbacks to be overridden by caller
		this.onReady = function() {};
		this.onMessage = function() {};
		this.onClose = function() {};

		this.port = params.port ? params.port : 80;
		this.path = params.path ? params.path : null; // includes query string

		this.ws_state = 0;

		this.packet_id = 1;

		if (this.path) {
			// presence of this.path triggers WebSockets mode, as MQTT has no native concept of path
			if (params.Socket)
				this.ws = new WSClient({host: this.host, port: this.port, path: this.path, protocol: "mqtt", Socket: params.Socket, secure: params.secure});
			else
				this.ws = new WSClient({host: this.host, port: this.port, path: this.path, protocol: "mqtt"});
			this.ws.callback = ws_callback.bind(this);
		} else {
			throw new Error("native MQTT not yet implemented; use websockets");
			// TODO: implement MQTT native mode
			/* something like...
			this.socket = new Socket({...});
			this.socket.callback = socket_callback.bind(this);
			*/
			// also need the usual read/write loops for socket, etc.
		}
	}

	publish(topic, msg) {
		if (this.ws_state < 2) {
			throw new Error("cannot publish to closed connection");
		}
		msg = MQTTHelper.to_publish_msg(topic, msg, this.packet_id++);
		this.ws.write(msg);
	}

	subscribe(topic) {
		if (this.ws_state < 2) {
			throw new Error("cannot subscribe to closed connection");
		}
		let msg = MQTTHelper.to_subscribe_msg(topic, this.packet_id++);
		this.ws.write(msg);
	}

	unsubscribe(topic) {
		if (this.ws_state < 2) {
			throw new Error("cannot subscribe to closed connection");
		}
		let msg = MQTTHelper.to_unsubscribe_msg(topic, this.packet_id++);
		this.ws.write(msg);
	}

	ping() {
		if (this.ws_state < 2) {
			throw new Error("cannot ping on closed connection");
		}

		let msg = MQTTHelper.new_ping_message
		this.ws.write(msg);
		// currently we ignore pongs; can add an onPingResponse() callback w/ code in ws_callback if useful
	}

	close() {
		if (this.ws) {
			let msg = MQTTHelper.new_close_msg();
			this.ws.write(msg); // just shoot it out there, don't worry about ACKs
			this.ws.close();
			delete this.ws;
		}

		this.ws_state = 0;
	}
}

function ws_callback(state, message) {
	const CONNACK = 0x20;
	const PUBLISH = 0x30;

	switch (state) {
		case 1: // socket connected
			// we don't care about this, we only care when websocket handshake is done
			break;

		case 2: // websocket handshake complete
			// at this point we need to begin the MQTT protocol handshake
			this.ws_state = 1;
			let bytes = MQTTHelper.to_connect_msg(this.connect);
			this.ws.write(bytes);
			break;

		case 3: // message received
			let msg = MQTTHelper.decode_msg(message);
			if (this.ws_state == 1) {
				if (msg.code != CONNACK) {
					trace(`WARNING: received message type '${flag}' when expecting CONNACK\n`);
					break;
				}

				if (msg.returnCode) {
					throw new Error("server rejected request with code " + msg.returnCode);
				}

				this.ws_state = 2;
				this.onReady();
				break;
			}

			switch (msg.code) {
				case PUBLISH:
					this.onMessage(msg.topic, msg.data);
					break;
				default:
					if (msg.code)
						trace(`received unhandled or no-op message type '${msg.code}'\n`);
					break;
			}
		  
			break;

		case 4: // websocket closed
			this.onClose();
			delete this.ws;
			break;

		default:
			trace(`ERROR: unrecognized websocket state %{state}\n`);
			break;
	}
}

class MQTTHelper {
	// byte-formatting helper functions
	static to_connect_msg(client_id) @ "mqtt_connect_msg";
	static to_publish_msg(topic, payload) @ "mqtt_publish_msg";
	static to_subscribe_msg(topic) @ "mqtt_subscribe_msg";
	static to_unsubscribe_msg(topic) @ "mqtt_unsubscribe_msg";
	static new_ping_msg() @ "mqtt_ping_msg";
	static new_close_msg() @ "mqtt_close_msg";

	// byte-decoding helper functions
	static decode_msg() @ "mqtt_decode_msg";
}
