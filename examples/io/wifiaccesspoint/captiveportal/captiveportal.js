/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

import WiFiAccessPoint from "embedded:network/interface/wifi/accesspoint";
import WiFi from "embedded:network/interface/wifi";

import UDP from "embedded:io/socket/udp";
import DNSServer from "embedded:network/dns/server/udp";

import StaticRoute from "embedded:network/http/server/route/static";
import WebSocketHandshake from "embedded:network/http/server/route/ws/handshake";
import WebSocketClient from "embedded:network/websocket/client";

const ALPHABET = "abcdefghijkmnpqrstuvwxyz23456789";
function randomText(length) {
	let out = "";
	for (let i = 0; i < length; i++)
		out += ALPHABET[Math.irandom(ALPHABET.length)];
	return out;
}

function clearestChannel(scan) {
	const load = {1: 0, 6: 0, 11: 0};
	for (const ap of scan.values()) {
		let nearest = 1, distance = Infinity;
		for (const c of [1, 6, 11]) {
			if (ap.channel) {
				const d = Math.abs(c - ap.channel);
				if (d < distance) {
					distance = d;
					nearest = c;
				}
			}
		}
		load[nearest] += 1;
	}
	let choice = 1, min = Infinity;
	for (const c of [1, 6, 11]) {
		if (load[c] < min) {
			min = load[c];
			choice = c;
		}
	}
	return choice;
}

class CaptivePortal {
	#ap;
	#wifi;
	#httpServer;
	#dnsServer;
	#ws;
	#channel;
	#onPage;
	#onClose;
	#onError;
	#onStatus;
	#onInfo;
	#phase = "";
	#credentials;
	#ssid;
	#password;
	#scan;

	constructor(options) {
		const {onPage, target} = options;
		if (!onPage)
			throw new Error("onPage required");

		this.#onPage = onPage;
		this.#onClose = options.onClose;
		this.#onError = options.onError;
		this.#onStatus = options.onStatus;
		this.#onInfo = options.onInfo;
		this.#channel = options.channel;
		this.#ssid = options.SSID ?? "Moddable";
		this.#password = options.password ?? randomText(10);

		if (undefined !==target)
			this.target = target;

		this.#wifi = new WiFi({
			onChanged: property => this.#onWiFiChanged(property, this.#wifi[property])
		});

		this.#setPhase("initializing");
		this.#wifi.scan({
			onFound: ap => {
				if (!ap.SSID) return;
				this.#wifi.found ??= new Map;
				const prev = this.#wifi.found.get(ap.SSID);
				if (!prev || (prev.RSSI < ap.RSSI)) {
					const {BSSID, ...entry} = ap;		// remove BSSID
					this.#wifi.found.set(ap.SSID, entry);
				}
			},
			onComplete: () => {
				const found = this.#wifi.found;
				delete this.#wifi.found;
				if (found)
					this.#scan = Array.from(found.values());

				let SSID = this.#ssid;
				while (found?.has(SSID))
					SSID = `${this.#ssid}-${randomText(4)}`;

				const options = {
					SSID,
					max: 1,
					onChanged: name => this.#onAPChanged(name),
					onConnect: () => this.#setPhase("connected"),
					onDisconnect: () => {
						this.#ws?.close();
						this.#ws = undefined;
						this.#setPhase("waiting");
					}
				};
				if (this.#channel)
					options.channel = this.#channel;
				else if (found?.size)
					options.channel = clearestChannel(found);
				if (this.#password) options.password = this.#password;
				this.#ap = new WiFiAccessPoint(options);
			}
		});
	}

	get SSID() {
		return this.#ap?.SSID;
	}

	close() {
		this.#ws?.close();
		this.#httpServer?.close();
		this.#dnsServer?.close();
		this.#wifi?.close();
		this.#ap?.close();
		this.#ws = this.#httpServer = this.#dnsServer = this.#wifi = this.#ap = undefined;
	}

	#onAPChanged(name) {
		if ("connection" !== name) return;
		if (this.#ap.connection >= 400) {
			if (!this.#dnsServer) {
				this.#ap.configure({
					portal: `http://${this.#ap.address}`
				});
				this.#dnsServer = this.#initializeDNSServer();
			}
			this.#httpServer ??= this.#initializeHTTPServer();
			this.#setPhase("ready", {SSID: this.#ap.SSID, password: this.#password});
		}
	}

	#onWiFiChanged(property, value) {
		if ("connection" !== property) return;

		if (value >= 500) {
			this.#setPhase("provisioned", this.#credentials);
			this.#onInfo?.({event: "credentials", ...this.#credentials})
		}
		else if (value >= 300)
			this.#setPhase("connecting", this.#credentials);
		else if ((value <= 200) && ("connecting" === this.#phase))
			this.#setPhase("failed");
	}

	#setPhase(phase, detail) {
		if (this.#phase === phase) return;
		this.#phase = phase;
		this.#onStatus?.(phase, detail);
		this.#sendWS({event: "status", phase});
	}

	#fail(err) {
		this.#onError?.(err);
	}

	#sendWS(msg) {
		if (!this.#ws)
			return;

		try {
			this.#ws.target.pending.push(new Uint8Array(ArrayBuffer.fromString(JSON.stringify(msg))));
			this.#flushWS();
		}
		catch (e) {
			this.#fail(e);
		}
	}

	#initializeDNSServer() {
		return new DNSServer({
			socket: {io: UDP},
			onResolve: () => this.#ap.address
		});
	}

	#initializeHTTPServer() {
		const portal = this;
		return new device.network.http.server.io({
			...device.network.http.server,
			port: 80,
			onRoute: request => {
				if ("GET" !== request.method)
					return;

				if ("/ws" === request.path) {
					return {
						...WebSocketHandshake,
						onDone() {
							portal.#attachWebSocket(this.detach());
						}
					};
				}

				const page = portal.#onPage(request.path);
				if (page) {
					return {
						...StaticRoute,
						data: page.content,
						headers: new Map([
							["content-type", page.mimeType],
							["cache-control", "no-store"]
						])
					};
				}

				const dest = `http://${portal.#ap.address}`;
				return {
					...StaticRoute,
					data: `<HTML><HEAD><META http-equiv="refresh" content="0; URL=${dest}"></HEAD></HTML>`,
					status: 200,
					headers: new Map([
						["location", dest],
						["content-type", "text/html"],
						["cache-control", "no-store"]
					])
				};
			}
		});
	}

	#attachWebSocket(socket) {
		const portal = this;
		let message;

		this.#ws = new WebSocketClient({
			attach: socket,
			target: {open: false, writable: 0, pending: []},
			onReadable(count, options) {
				if (count) {
					const data = this.read(count);
					message = message ? message.concat(data) : data;
				}
				if (options.more)
					return;

				const complete = message;
				message = undefined;
				if (options.binary || !complete)
					return;

				try {
					portal.#handleWSMessage(JSON.parse(String.fromArrayBuffer(complete)));
				}
				catch (e) {
					portal.#fail(e);
				}
			},
			onWritable(count) {
				const state = this.target;
				state.writable = count;
				if (!state.open) {
					state.open = true;
					portal.#sendWS({event: "ws initialize"});
				}
				portal.#flushWS();
			},
			onControl(opcode) {
				if (WebSocketClient.close === opcode)
					portal.#ws = null;
			},
			onError() {
				portal.#ws = null;
				portal.#fail(new Error("WebSocket error"));
			}
		});
	}

	#flushWS() {
		const ws = this.#ws;
		if (!ws)
			return;

		const state = ws.target;
		while (state.pending.length && state.writable) {
			const pending = state.pending[0];
			const more = pending.byteLength > state.writable;
			const data = more ? pending.subarray(0, state.writable) : pending;

			state.writable = ws.write(data, {binary: false, more});

			if (more)
				state.pending[0] = pending.subarray(data.byteLength);
			else
				state.pending.shift();
		}
	}

	#handleWSMessage(msg) {
		switch (msg.event) {
			case "scan":
				this.#handleScan();
				break;
			case "connect":
				this.#handleConnect(msg);
				break;
			case "disconnect":
				this.#wifi.disconnect();
				break;
			case "terminate":
				this.#handleTerminate();
				break;
			default:
				this.#onInfo?.(msg);
				break;
		}
	}

	#handleScan() {
		if (this.#scan) {
			this.#sendWS({event: "scan", scan: this.#scan});
			this.#scan = undefined;
			return;
		}

		const scan = new Map;
		try {
			this.#wifi.scan({
				onFound: ap => {
					if (ap.SSID === this.#ap.SSID) return;
					const prev = scan.get(ap.SSID);
					if (!prev || (prev.RSSI < ap.RSSI)) {
						const {BSSID, ...entry} = ap;		// remove BSSID
						scan.set(ap.SSID, entry);
					}
				},
				onComplete: () => {
					this.#sendWS({
						event: "scan",
						scan: Array.from(scan.values())
					});
				}
			});
		}
		catch (e) {
			this.#fail(e);
		}
	}

	#handleConnect(msg) {
		const options = {SSID: msg.SSID};
		if (msg.password) options.password = msg.password;
		if (msg.channel) options.channel = msg.channel;
		if (msg.secure) options.secure = msg.secure;
		this.#credentials = options;
		try {
			this.#wifi.disconnect();		// ensure connect() is a fresh attempt (not currently connected to the same access point)
			this.#wifi.connect(options);
		   this.#setPhase("connecting", options);
		}
		catch (e) {
			this.#fail(e);
		}
	}

	#handleTerminate() {
		this.close();
		this.#onClose?.();
	}
}

export default CaptivePortal;
