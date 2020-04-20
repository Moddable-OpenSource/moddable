import MQTT from "mqtt";
import {Socket, Listener} from "socket";

class Server extends Listener {
	#clients = [];
	#onMessage;
	#target;

	static #Client = class extends MQTT {
		subscriptions = [];

		constructor(options, server) {
			super(options);
			server.#clients.push(this);
			this.server = server;
		}
		onAccept(connect) {
			return 0;
		}
		onSubscribe(topic, qos) {
			topic = topic.split("/");
			// validate topic
			const l = topic.length;
			if (!l)
				return;
			for (let i = 0; i < l; i++) {
				const t = topic[i];
				if (t.length <= 1) {
					if ("#" === t) {
						if ((i + 1) !== l)
							return;
					}
					if (!t.length)
						return;
				}
				else if (t.includes("#") || t.includes("+"))
					return;
			}

			const subscriptions = this.subscriptions;
			// check for duplicates
		loop:
			for (let i = 0, subscriptions = this.subscriptions, length = subscriptions.length; i < length; i++) {
				for (let j = 0; j < l; j++) {
					if (topic[j] !== subscriptions[i][j])
						continue loop;
				}
				return;
			}
			// add
			subscriptions.push(topic);
			trace(`Subscribe: ${topic.join("/")}\n`);
		}
		onUnsubscribe(topic) {
			topic = topic.split("/");
			const subscriptions = this.subscriptions;
			// don't need to validate topic -- onSubscribe caught any errors
		loop:
			for (let i = 0, subscriptions = this.subscriptions, length = subscriptions.length, l = topic.length; i < length; i++) {
				for (let j = 0; j < l; j++) {
					if (topic[j] !== subscriptions[i][j])
						continue loop;
				}
				subscriptions.splice(i, 1);
				trace(`Unsubscribe: ${topic.join("/")}\n`);
				return;
			}
		}
		onMessage(topic, body) {
			this.server.#onMessage.call(this.server.#target, topic, body, this);
		}
		onClose() {
			const clients = this.server.#clients;
			clients.splice(clients.indexOf(this), 1);
		}
	}

	constructor(options) {
		super({port: 1883, ...options});
		this.#onMessage = options.onMessage ?? this.onMessage ?? function() {};
		this.#target = options.target ?? this;
	}
	close() {
		for (let i = 0, clients = this.#clients, length = clients.length; i < length; i++)
			clients[i].close();
		this.#clients = null;
		super.close();
	}
	callback() {
		const socket = new Socket({listener: this});
		new Server.#Client({socket}, this);
	}
	publish(topic, body) {
		const parts = topic.split("/");
		const l = parts.length;
		for (let i = 0, clients = this.#clients; i < clients.length; i++) {
		loop:
			for (let j = 0, subscriptions = clients[i].subscriptions, length = subscriptions.length; j < length; j++) {
				const subscription = subscriptions[j]
				let ti = 0;
				for (let k = 0; k < subscription.length; k++) {
					const part = subscription[k];
					if ("#" === part) {
						if ((k + 1) === subscription.length) {
							ti = l;
							break;
						}
						ti = parts.indexOf(subscription[k + 1], ti);
						if (ti < 0)
							continue loop;
					}
					else if ("+" === part)
						ti++;
					else if (parts[ti++] !== part)
						continue loop;
				}
				if (ti === l) {
					clients[i].publish(topic, body);
					break;
				}
			}
		}
	}
}

export default Server;

