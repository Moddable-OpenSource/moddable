import Timer from "timer";

let urlRegExp = null;
let authorityRegExp = null;
function URLParts(url) {
	if (!urlRegExp)
		urlRegExp = new RegExp("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?");
	const urlParts = url.match(urlRegExp);
	if (!authorityRegExp)
		authorityRegExp = new RegExp("^([^:]+)(:(.*))?");
	const authorityParts = urlParts[4].match(authorityRegExp);
	return {
    	scheme:urlParts[2],
		host:authorityParts[1],
		port:authorityParts[3] ?? 1883,
		path:urlParts[5],
    	query:urlParts[7],
		fragment:urlParts[9],
	}
}

const TypedArray = Object.getPrototypeOf(Int8Array);

const CLOSED = 0;
const CONNECTING = 1;
const RECONNECTING = 2;
const CONNECTED = 3;
const ENDING = 4;
const CLOSING = 5;

export class Client {
	#acks = [];
	#buffers = [];
	#client = null;
	#connack = null;
	#endCallback = null;
	#eventListeners = {
		close:[],
		connect:[],
// 		disconnect:[], // MQTT 5.0
		end:[],
		error:[],
		message:[],
		offline:[],
// 		packetreceive:[], // not supported
// 		packetsend:[], // not supported
		reconnect:[],
	};
	#id = 0;
	#moreMessage = null;
	#moreTopic = null;
	#options = null;
	#reconnectPeriod = 1000;
	#reconnectTimer = null;
	#resubscribe = true;
	#state = CLOSED;
	#subscriptions = [];
	#wait = false;
	#writable = 0;
	
	constructor(url, options) {
		const parts = URLParts(url);
		if (parts.scheme != "mqtt")
			throw new URIError("mqtt only");
		let keepalive = 60_000;
		let id = "mqttxs_"  + Math.random().toString(16).substr(2, 8);
		let user = undefined;
		let password = undefined;
		let will = null;
		if (options) {
			if ("clientId" in options)
				id = options.clientId
			if ("keepalive" in options)
				keepalive = 1000 * options.keepalive
			if ("password" in options)
				password = options.password
			if ("username" in options)
				user = options.username
			if ("will" in options) {
				will = {
					topic: options.will.topic,
					message: options.will.payload,
					QoS: options.will.qos ?? 0,
					retain: options.will.retain ?? false,
				};
			}
			if ("reconnectPeriod" in options)
				this.#reconnectPeriod = options.reconnectPeriod
			if ("resubscribe" in options)
				this.#resubscribe = options.resubscribe
		}
		this.#options = {
			...device.network.mqtt,
			host: parts.host, port: parts.port,
			keepalive, id, user, password, will,
			onControl: (msg) => {
				if (msg.operation == device.network.mqtt.io.CONNACK) {
					this.#state = CONNECTED;
					this.#wait = true;
					this.#acks.forEach(ack => {
						ack.wait = true;
					});
					this.#buffers.forEach(buffer => {
						buffer.wait = true;
					});
				}
				else {
					let acks = this.#acks;
					this.#acks = [];
					acks = acks.filter(ack => {
						if ((ack.operation == msg.operation) && (ack.id == msg.id)) {
							if (ack.operation == device.network.mqtt.io.PUBREC) {
								ack.operation = device.network.mqtt.io.PUBCOMP;
								delete ack.data;
								delete ack.options;
								delete ack.optionsLength;
								// device.network.mqtt.io already sent PUBREL
								return true;
							}
							if (ack.callback) {
								if (ack.operation == device.network.mqtt.io.SUBACK) {
									const items = ack.items.map((item, index) => {
										return {
											qos: msg.payload[index],
											topic: item.key,
										};
									});
									ack.callback.call(null, null, items);
								}
								else
									ack.callback.call(null, null);
							}
							return false;
						}
						return true;
					});
					this.#acks = acks.concat(this.#acks);
				}
				this.#checkConnect();
				this.#checkEnd();
			},
			onReadable: (count, options) => {
// 				trace(`onReadable ${count} more ${options.more}\n`);
				if (!count)
					return;
				let data = this.#client.read(count);
				let message = this.#moreMessage;
				let topic = this.#moreTopic;
				if (message)
					message = message.concat(data);
				else {
					message = data;
					topic = options.topic;
				}
				if (options.more) {
					this.#moreMessage = message;
					this.#moreTopic = topic;
				}
				else {
					this.#moreMessage = null;
					this.#moreTopic = null;
					this.#eventListeners.message.forEach(listener => listener.call(null, topic, message));
				}
			},
			onWritable: (count) => {
// 				trace(`onWritable ${count}\n`);
				if (!count)
					return;
				this.#writable = count;
				let buffers = this.#buffers;
				while (buffers.length) {
					let buffer = buffers[0];
					let { callback, data, options, optionsLength } = buffer;
					let dataLength = data ? data.byteLength : 0;
					let writable = this.#writable;
					if (optionsLength > 0)
						writable -= optionsLength;
					else
						options = null;
					if (dataLength <= writable) {
						this.#writable = this.#client.write(data, options);
						buffer.options.duplicate = true;
						if (callback) callback();
						buffers.shift();
					}
					else if ((0 < dataLength) && (0 < writable)) {
						let dataBuffer, dataOffset;
						if (ArrayBuffer.isView(data)) {
							dataBuffer = data.buffer;
							dataOffset = data.byteOffset;
						}
						else {
							dataBuffer = data;
							dataOffset = 0;
						}
						this.#writable = this.#client.write(new DataView(dataBuffer, dataOffset, writable), options);
						buffer.data = new DataView(dataBuffer, dataOffset + writable);
						buffer.optionsLength = 0;
// 						trace("...\n");
// 						Timer.delay(1000);
						break;
					}
					else
						break;
				}
				this.#checkConnect();
				this.#checkEnd();
			},
			onError: (reason) => {
 				const state = this.#state;
				this.#client.close();
				this.#client = null;
				this.#state = CLOSED;
				this.#wait = false;
    			this.#writable = 0;
				switch (state) {
				case CLOSED:
					break;
				case CONNECTING:
				case RECONNECTING:
   					this.#eventListeners.error.forEach(listener => listener.call(null, reason));
					break;
				case CONNECTED:
 					this.#restore(true);
   					this.#eventListeners.offline.forEach(listener => listener.call(null));
					break;
				case ENDING:
				case CLOSING:
					// ??
					break;
				}
				this.#eventListeners.close.forEach(listener => listener.call(null));
 				if (this.#reconnectPeriod > 0) {
					this.#reconnectTimer = Timer.set(() => { 
						this.reconnect();
					}, this.#reconnectPeriod);
				}
			}
		};
		this.#state = CONNECTING;
		this.#client = new device.network.mqtt.io(this.#options);
	}
	get connected() {
		return this.#state == CONNECTED;
	}
	get reconnecting() {
		return this.#state == RECONNECTING;
	}
	addEventListener(event, listener) {
		let listeners = this.#eventListeners[event];
		if (!listeners)
			throw new Error("no such event");
		listeners.push(listener);
	}
	end(force, options, callback) {
		if (typeof force !== 'boolean') {
			callback = options;
			options = force;
			force = false;
		}
		if (typeof options !== 'object') {
			callback = options;
			options = undefined;
		}
		if (typeof callback !== 'function') {
			callback = undefined;
		}
		if (this.#reconnectTimer) {
			Timer.clear(this.#reconnectTimer);
			this.#reconnectTimer = null;
		}
		switch (this.#state) {
		case CLOSED:
			if (callback) callback();
			break;
		case CONNECTING:
		case RECONNECTING:
			throw new Error("MQTT client connecting");
			break;
		case CONNECTED:
			this.#endCallback = callback;
			this.#state = ENDING;
// 			this.#write(null, { operation: device.network.mqtt.io.DISCONNECT }, 2);
			if (force)
				this.#close();
			else
				this.#checkEnd();
			break;
		case ENDING:
		case CLOSING:
			if (callback) callback();
			break;
		}
		return this;
	}
	getLastMessageId() {
		return this.#id;
	}
	handleMessage(packet, callback) {
		throw new Error("no such method"); // not supported
	}
	publish(topic, message, options, callback) {
		if (typeof options !== 'object') {
			callback = options;
			options = undefined;
		}
		if (typeof callback !== 'function') {
			callback = undefined;
		}
		if (this.#checkConnection(callback))
			return this;
		const QoS = options?.qos ?? 0;
		const retain = options?.retain ?? false;
		const duplicate = options?.dup ?? false;
		const id = ++this.#id;
		let data, ack;
		if (message instanceof ArrayBuffer)
			data = message;
		else if ((message instanceof DataView) || (message instanceof TypedArray))
			data = message.buffer.slice(message.byteOffset, message.byteOffset + message.byteLength);
		else
			data = ArrayBuffer.fromString(message);
		topic = ArrayBuffer.fromString(topic);
		options =  { operation: device.network.mqtt.io.PUBLISH, topic, id, QoS, retain, duplicate, byteLength:data.byteLength };
		let optionsLength = 5 + 2 + topic.byteLength;
		if (QoS == 1) {
			optionsLength += 2;
			this.#acks.push({ operation:device.network.mqtt.io.PUBACK, id, callback, data, options, optionsLength });
			callback = undefined;
		}
		else if (QoS == 2) {
			optionsLength += 2;
			this.#acks.push({ operation:device.network.mqtt.io.PUBREC, id, callback, data, options, optionsLength });
			callback = undefined;
		}
		this.#write(data, options, optionsLength, callback);
		return this;
	}
	reconnect() {
		if (this.#reconnectTimer) {
			Timer.clear(this.#reconnectTimer);
			this.#reconnectTimer = null;
		}
		switch (this.#state) {
		case CLOSED:
 			this.#state = RECONNECTING;
   			this.#eventListeners.reconnect.forEach(listener => listener.call(null));
			this.#client = new device.network.mqtt.io(this.#options);
			break;
		case CONNECTING:
 			this.#state = RECONNECTING;
    		this.#eventListeners.reconnect.forEach(listener => listener.call(null));
			break;
		case RECONNECTING:
			break;
		case CONNECTED:
			this.end(() => {
				this.reconnect();
			});
			break;
		case ENDING:
		case CLOSING:
			throw new Error("MQTT client disconnecting");
			break;
		}
  		return this;
	}
	removeEventListener(event, listener) {
		let listeners = this.#eventListeners[event];
		if (!listeners)
			throw new Error("no such event");
		let index = listeners.find(item => item === listener);
		if (index >= 0)
			listeners.splice(index, 1);
	}
	removeOutgoingMessage() {
		throw new Error("no such method"); // not supported
	}
	subscribe(topics, options, callback) {
		if (typeof options !== 'object') {
			callback = options;
			options = undefined;
		}
		if (typeof callback !== 'function') {
			callback = undefined;
		}
		if (this.#checkConnection(callback))
			return this;
		const QoS = options?.qos ?? 0;
		let items;
		if (Array.isArray(topics)) {
			items = topics.map(key => {
				const topic = ArrayBuffer.fromString(key);
				return { key, topic, QoS };
			});
		}
		else if (typeof(topics) == "object") {
			items = [];
			for (let key in topics) {
				const topic = ArrayBuffer.fromString(key);
				items.push({ key, topic, QoS:topics[key] });
			}
		}
		else {
			const key = topics;
			const topic = ArrayBuffer.fromString(key);
			items = [{ key, topic, QoS  }];
		}
		if (this.#resubscribe) {
			items = items.filter(item => {
				return this.#subscriptions.find(subscription => subscription.key == item.key) == undefined;
			});
			if (items.length)
				this.#subscriptions = this.#subscriptions.concat(items);
		}
		if (items.length) {
			let optionsLength = 5 + 2;
			items.forEach(item => {
				optionsLength += 2 + item.topic.byteLength + 1;
			});
			const id = ++this.#id;
			this.#acks.push({ operation:device.network.mqtt.io.SUBACK, id, callback, items });
			this.#write(null, { operation: device.network.mqtt.io.SUBSCRIBE, items, id }, optionsLength);
		}
		else if (callback)
			callback(null, items);
		return this;
	}
	unsubscribe(topics, options, callback) {
		if ((callback === undefined) && (typeof(options) == "function")) {
			callback = options;
			options = undefined;
		}
		if (this.#checkConnection(callback))
			return this;
		const id = ++this.#id;
		let items, optionsLength = 5 + 2;
		if (Array.isArray(topics)) {
			items = topics.map(key => {
				const topic = ArrayBuffer.fromString(key);
				optionsLength += 2 + topic.byteLength;
				return { key, topic };
			});
		}
		else {
			const key = topics;
			const topic = ArrayBuffer.fromString(key);
			items = [{ key, topic  }];
			optionsLength += 2 + topic.byteLength;
		}
		if (this.#resubscribe) {
			this.#subscriptions = this.#subscriptions.filter(subscription => {
				return items.find(item => item.key == subscription.key) == undefined;
			});
		}
		this.#acks.push({ operation:device.network.mqtt.io.UNSUBACK, id, callback });
		this.#write(null, { operation: device.network.mqtt.io.UNSUBSCRIBE, items, id }, optionsLength);
		return this;
	}
	#checkConnect() {
		if (this.#wait) {
			if (!this.#acks.find(ack => ack.wait) && !this.#buffers.find(buffer => buffer.wait)) {
				this.#wait = false;
     			this.#eventListeners.connect.forEach(listener => listener.call(null, this.#connack));
			}
		}
	}
	#checkConnection(callback) {
		if ((this.#state == ENDING) || (this.#state == CLOSING)) {
			if (callback)
				callback(new Error("MQTT client disconnecting"));
			return true;
		}
	}
	#checkEnd() {
		if ((this.#state == ENDING) && (this.#acks.length == 0) && (this.#buffers.length == 0)) {
			this.#close();
		}
	}
	#close() {
		this.#state = CLOSING;
		Timer.set(() => { 
			this.#client.close();
			this.#client = null;
			this.#state = CLOSED;
			this.#wait = false;
    		this.#writable = 0;
			this.#restore(false);
			this.#eventListeners.close.forEach(listener => listener.call(null));
			this.#eventListeners.end.forEach(listener => listener.call(null));
			const callback = this.#endCallback;
			if (callback) {
				this.#endCallback = null;
				callback.call(null);
			}
		});
	}
	#restore(republish) {
		const acks = this.#acks;
		const buffers = this.#buffers;
		this.#acks = [];
		this.#buffers = [];
		const items = this.#subscriptions;
		if (items.length) {
			let optionsLength = 5 + 2;
			items.forEach(item => {
				optionsLength += 2 + item.topic.byteLength + 1;
			});
			const id = ++this.#id;
			const callback = () => {
				// ??  
			};
			this.#acks.push({ operation:device.network.mqtt.io.SUBACK, id, callback, items });
			this.#write(null, { operation: device.network.mqtt.io.SUBSCRIBE, items, id }, optionsLength);
		}
		acks.forEach(ack => {
			if ((ack.operation == device.network.mqtt.io.SUBACK) || (ack.operation == device.network.mqtt.io.UNSUBACK)) {
				if (ack.callback) {
					ack.callback(new Error("MQTT client disconnected"));
				}
			}
			if (republish) {
				if ((ack.operation == device.network.mqtt.io.PUBACK) || (ack.operation == device.network.mqtt.io.PUBREC)) {
					const { data, options, optionsLength } = ack;
					this.#acks.push(ack);
					this.#write(data, options, optionsLength);
				}
				else if (ack.operation == device.network.mqtt.io.PUBCOMP) {
					const { id } = ack;
					this.#acks.push(ack);
					this.#write(null, { operation: device.network.mqtt.io.PUBREL, id }, 5 + 2);
				}
			}
		});
	}
	#write(data, options, optionsLength, callback) {
		let buffers = this.#buffers;
		if ((this.#state < CONNECTED) || buffers.length)
			buffers.push({ data, options, optionsLength, callback });
		else {
			let dataLength = 0;
			if (data)
				dataLength = options.dataLength = data.byteLength;
			const writable = this.#writable - optionsLength;
			if (dataLength <= writable) {
				this.#writable = this.#client.write(data, options);
				options.duplicate = true;
				if (callback) callback();
			}
			else if ((0 < dataLength) && (0 < writable)) {
				this.#writable = this.#client.write(new DataView(data, 0, writable), options);
				buffers.push({ data:new DataView(data, writable), options, optionsLength:0, callback });
			}
			else {
				buffers.push({ data, options, optionsLength, callback });
			}
		}
	}
}
Client.prototype.off = Client.prototype.removeEventListener;
Client.prototype.on = Client.prototype.addEventListener;

export function connect(url, options) {
	return new Client(url, options);
}
