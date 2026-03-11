/*
 * Copyright (c) 2025-2026  Moddable Tech, Inc.
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

console.log("@moddable/proxy launched");

const PROXY_VERSION = 1;
const HTTP_BASE = 15000;
const READY_MESSAGE = 15025;
const LOCATION_MESSAGE = 15026;
const WS_BASE = 15050;

const gHTTPRequests = new Map;
const gWSRequests = new Map;

function appMessageReceived(e) {
	if (state.log) {
		console.log("moddable appmessage received");

		for (let key in e.payload) {
			console.log(key);
			let value = e.payload[key];
			if (Array.isArray(value)) { 
				let tt = Uint8Array.from(value);
				tt = String.fromCharCode(...tt);
				console.log("binary: " + tt);
			}
			else
				console.log((typeof value) + ": " + value);
		}
	}

	let id = e.payload[HTTP_BASE + 1];
	if (undefined !== id) {
		try {
			httpMessage(id, e);
		}
		catch (error) {
			console.log("moddable proxy http exception" + error);
		}
		return true;
	}

	id = e.payload[WS_BASE + 1];
	if (undefined !== id) {
		try {
			wsMessage(id, e);
		}
		catch (error) {
			console.log("moddable proxy ws exception" + error);
		}
		return true;
	}

	id = e.payload[LOCATION_MESSAGE];
	if (undefined !== id) {
		try {
			locationMessage(id, e);
		}
		catch (error) {
			console.log("moddable proxy location exception" + error);
		}
		return true;
	}

	id = e.payload[READY_MESSAGE];
	if (undefined !== id) {
		try {
			readyReceived();
		}
		catch (error) {
			console.log("moddable proxy ready exception" + error);
		}
		return true;
	}

	return false;
};

function readyReceived(e) {
	if (state.log)
		console.log("readyReceived")
	sendAppMessage({
		[READY_MESSAGE]: PROXY_VERSION
	});
}

function httpMessage(id, e) {
	if (state.log)
		console.log("   connection: " + id);

	if (!gHTTPRequests.has(id))
		gHTTPRequests.set(id, {id, state: "configure", kind: "http"});
	const request = gHTTPRequests.get(id);
	
	switch (request.state) {
		case "configure": {
			const [protocol, method, host, port, path, bufferSize, headersMask] = arrayToString(e.payload[HTTP_BASE + 2]).split(":");
			request.bufferSize = parseInt(bufferSize);
			if ("/" === path)
				request.path = "";
			else
				request.path = path || "";
			request.port = port;
			request.host = host;
			request.protocol = protocol;
			request.method = method;
			request.headersMask = headersMask ? headersMask.split(",") : "*";

			request.state = "recieveHeaders";  
			request.headers = "";
			} break;

		case "recieveHeaders":
			if (e.payload[HTTP_BASE + 3]) {
				request.headers += arrayToString(e.payload[HTTP_BASE + 3]);
				break;
			}
			request.requestBody = new Uint8Array(0);
			request.state = "receiveBody";  
			// deliberate fall through

		case "receiveBody":
			if (e.payload[HTTP_BASE + 4]) {
				const fragment = arrayToUint8Array(e.payload[HTTP_BASE + 4]);
				const requestBody = new Uint8Array(fragment.length + request.requestBody.length);
				requestBody.set(request.requestBody);
				requestBody.set(fragment, request.requestBody.length);;
				request.requestBody = requestBody; 
				break;
			}
			request.state = "makeRequest";  
			// deliberate fall through

		case "makeRequest": {
			if (!e.payload[HTTP_BASE + 5])
				throw new Error("expected property missing");

			if (state.log) {
				console.log("make the request")
				console.log(`  method: ${request.method}`);  
				console.log(`  protocol: ${request.protocol}`);  
				console.log(`  host: ${request.host}`);  
				console.log(`  port: ${request.port}`);  
				console.log(`  path: ${request.path}`);  
				console.log(`  bufferSize: ${request.bufferSize}`);
				console.log(`  headersMask: ${request.headersMask}`);
				console.log(`  requestBody: ${request.requestBody.length} bytes`);
				request.headers.split("\n").forEach(line => console.log("  " + line));
			}

			request.xhr = new XMLHttpRequest;
			const url = `${request.protocol}://${request.host}${request.port ? ":" + request.port : ""}/${request.path}`;
			if (state.log)
				console.log(`  url: ${url}`);
			request.xhr.open(request.method, url, true);
			request.xhr.responseType = 'arraybuffer';

			request.headers.split("\n").forEach(line => {
				const [key, value] = line.split(":");
				request.xhr.setRequestHeader(key, value);
			});

			request.xhr.onload = function () {
				request.messages = [];

				request.messages.push({
					[HTTP_BASE + 1]: request.id,
					[HTTP_BASE + 6]: request.xhr.status,
					[HTTP_BASE + 11]: request.xhr.statusText
				});

				const headers = request.xhr.getAllResponseHeaders().split("\r\n").filter(header => {
					if ("*" === request.headersMask)
						return true;			// no mask, return all
					header = header.split(":");
					return request.headersMask.includes(header[0].trim().toLowerCase());
				}).map(header => {
					header = header.split(":");
					header[0] = header[0].trim().toLowerCase();
					header[1] = header[1].trim();
					return header.join(":");
				}).join("\n");
				for (let position = 0, fragmentSize = request.bufferSize - 32 /* @@ */; position < headers.length; position += fragmentSize) {
					const fragment = headers.slice(position, position + fragmentSize);
					request.messages.push({
						[HTTP_BASE + 1]: request.id,
						[HTTP_BASE + 7]: fragment
					});
				}

				for (let position = 0, response = new Uint8Array(request.xhr.response), fragmentSize = request.bufferSize - 32 /* @@ */; position < response.byteLength; position += fragmentSize) {
					const fragment = response.slice(position, position + fragmentSize);
					request.messages.push({
						[HTTP_BASE + 1]: request.id,
						[HTTP_BASE + 8]: Array.from(fragment)		// sendAppMessage won't accept ArrayBuffer or Uint8Array. only Array.
					});
				}
				request.messages.push({
					[HTTP_BASE + 1]: request.id,
					[HTTP_BASE + 9]: 0						// done. success.
				});
				request.state = "sendMessages";
				
				sendRequestMessage(request);
			}
			request.xhr.onerror = function () {
				console.log("ON error!!!!")
				sendAppMessage({
					[HTTP_BASE + 1]: request.id,
					[HTTP_BASE + 9]: -1						// done. failure.
				});
			}
			if (request.requestBody.length)
				request.xhr.send(request.requestBody.buffer);
			else
				request.xhr.send();
			request.state = "waitResponse";  
			} break;

		default:
			console.log("unexpected state " + request.state + "\n");
			break;
	}
}

function wsMessage(id, e) {
	if (!gWSRequests.has(id))
		gWSRequests.set(id, {id, state: "configure", kind: "ws"});
	const request = gWSRequests.get(id);

	switch (request.state) {
		case "configure": {
			const [protocol, subprotocol, host, port, path, bufferSize] = arrayToString(e.payload[WS_BASE + 2]).split(":");
			request.bufferSize = parseInt(bufferSize);
			request.path = path || "/";
			request.port = port;
			request.host = host;
			request.protocol = protocol;
			request.subprotocol = subprotocol ? subprotocol.split(",") : [];
			
			request.state = "waitHandshake";  
			request.headers = "";		//@@ to do
			}

			// deliberate fall through

		case "connecting": {
			if (state.log) {
				console.log("websocket connect")
				console.log(`  protocol: ${request.protocol}`);  
				console.log(`  host: ${request.host}`);  
				console.log(`  port: ${request.port}`);  
				console.log(`  path: ${request.path}`);  
				console.log(`  subprotocol: ${request.subprotocol}`);  
				console.log(`  bufferSize: ${request.bufferSize}`);
			}

			const url = `${request.protocol}://${request.host}${request.port ? ":" + request.port : ""}${request.path}`;
			if (state.log)
				console.log(`   url: ${url}`);

//@@ use of subprotocol gives exception in pypkjs	request.ws = request.subprotocol ? new WebSocket(url, request.subprotocol) : new WebSocket(url);
			request.ws = new WebSocket(url);
			request.ws.binaryType = "arraybuffer";

			request.ws.onopen = ( )=> {
				if (state.log)
					console.log("websocket connected to host");
				request.state = "connected";
				request.messages = [];
				request.messages.sending = false;
				sendAppMessage({
					[WS_BASE + 1]: request.id,
					[WS_BASE + 2]: 0						// connected. success.
				});
			};
			request.ws.onerror = () => {
				if (state.log)
					console.log("websocket connection failed");
				request.state = "error";
				sendAppMessage({
					[WS_BASE + 1]: request.id,
					[WS_BASE + 3]: -1						// disconnected error.
				});
			};
			request.ws.onclose = event => {
				if (state.log)
					console.log("websocket connection closed");
				request.state = "closed";
				let reason = event.reason ? stringToArray(event.reason) : [];
				let bytes = new Uint8Array(2 + reason.length);
				let code = event.code ? event.code : 0;
				bytes[0] = event.code >> 8;
				bytes[1] = event.code;
				if (state.log)
					console.log(`close code ${code} reason ${arrayToString(reason)}`);
				if (reason.byteLength)
					bytes.set(arrayToUint8Array(reason.slice(2)), 2);
				request.messages.push({
					[WS_BASE + 1]: request.id,
					[WS_BASE + 3]: 0,						// disconnected clean.
					[WS_BASE + 10]: Array.from(bytes)		// sendAppMessage wants an Array
				});
				if (!request.messages.sending)
					sendRequestMessage(request);
			};
			request.ws.onmessage = event => {
				let data = event.data;		// either ArrayBuffer or String
				if (data instanceof ArrayBuffer)
					data = new Uint8Array(data);
				const binary = "string" !== typeof data;
				if  (binary)
					data = Array.from(data);	// sendAppMessage wants an Array
				else
					data = stringToArray(data);		// sendAppMessage wants an Array

				for (let position = 0, fragmentSize = request.bufferSize - 64 /* @@ */; position < data.length; position += fragmentSize) {
					const fragment = data.slice(position, position + fragmentSize);
					const more = (position + fragment.length) < data.length;
					const part = (binary ? 4 : 6) + (more ? 1 : 0);

					request.messages.push({
						[WS_BASE + 1]: request.id,
						[WS_BASE + part]: fragment
					});
				}
				
				if (!request.messages.sending)
					sendRequestMessage(request);
			};
			} break;

		case "connected": {
			let binary;
			if (!request.pendingWrite)
				request.pendingWrite = [];
	
			if (e.payload[WS_BASE + 4]) {	// binary no more
				request.pendingWrite.push(e.payload[WS_BASE + 4]);
				binary = true;
			}
			else if (e.payload[WS_BASE + 5])	// binary more
				request.pendingWrite.push(e.payload[WS_BASE + 5]);
			else if (e.payload[WS_BASE + 6]) {	// text no more
				request.pendingWrite.push(e.payload[WS_BASE + 6]);
				binary = false;
			}
			else if (e.payload[WS_BASE + 7])	// text more
				request.pendingWrite.push(e.payload[WS_BASE + 7]);
			else if (e.payload[WS_BASE + 8]) {	// close
				request.state = "closing";
				const bytes = arrayToUint8Array(e.payload[WS_BASE + 8]);
				let code, reason;
				if (bytes.byteLength >= 2) {
					code = (new DataView(bytes.buffer)).getInt16(0, false);
					if (state.log)
						console.log(`code ${code}`);
					if (bytes.byteLength > 2) {
						reason = arrayToString(e.payload[WS_BASE + 8].slice(2));
						if (state.log)
							console.log(`reason ${reason}`);
					}
				}
//				if (undefined === code)
					request.ws.close();
//				else if (undefined === reason)
//					request.ws.close(code);
//				else
//					request.ws.close(code, reason);
				return;
			}
			else {
				console.log("no payload found!");
				throw new Error("surrender");
			}

			if (undefined !== binary) {
				let total = 0;
				request.pendingWrite.forEach(fragment => total += fragment.length);
				let msg = new Uint8Array(total);
				for (let i = 0, offset = 0; i < request.pendingWrite.length; offset += request.pendingWrite[i++].length)
					msg.set(arrayToUint8Array(request.pendingWrite[i]), offset);

				try {
					if (binary)
						request.ws.send(msg);
					else
						request.ws.send(arrayToString(msg));
				}
				catch (e) {
					console.log("ws.send failed: " + e);		// sometiems Broken Pipe.
					request.state = "closed";
					sendAppMessage({
						[WS_BASE + 1]: request.id,
						[WS_BASE + 3]: -1						// done. failure.
					});
				}
				delete request.pendingWrite;
			}
			} break;

		case "closed":
			// client write after close - caused by latency delivering close notification
			break;

		default:
			console.log("unexpected state " + request.state + "\n");
			break;
	}
}

let locationHandler = undefined;

function locationMessage(message /*, e */) {
	message = message.split(",");	// enable,enableHighAccuracy,timeout,maximumAge
	if (parseInt(message[0])) {			// enable
		if (undefined !== locationHandler)
			return;
	
		const options = {};
		if ((undefined !== message[1]) && message[1])
			options.enableHighAccuracy = Boolean(message[1]);
		if ((undefined !== message[2]) && message[2])
			options.timeout = Number(message[2]);
		if ((undefined !== message[3]) && message[3])
			options.maximumAge = Number(message[3]);

		function filter(value) {
			return (undefined === value) ? "" : value;
		}
		locationHandler = navigator.geolocation.watchPosition(position => {
			const c = position.coords;
			sendAppMessage({
				[LOCATION_MESSAGE]: `1,${c.latitude},${c.longitude},${filter(c.altitude)},${filter(c.accuracy)},${filter(c.altitudeAccuracy )},${filter(c.heading)},${filter(c.speed)},${position.timestamp}`
			});
		}, () => {
			navigator.geolocation.clearWatch(locationHandler);
			locationHandler = undefined;

			sendAppMessage({
				[LOCATION_MESSAGE]: "0"
			});
		},
		options);
	}
	else {						// disable
		if (undefined === locationHandler)
			return;

		navigator.geolocation.clearWatch(locationHandler);
		locationHandler = undefined;
	}
}

const sendQueue = [];
let sending = false;

function sendAppMessageSuccess() {
	// console.log("sendAppMessageSuccess");
	sending = false;
	const sent = sendQueue.shift();
	if (sent.success)
		sent.success();
	if (!sending && sendQueue.length) {
	// console.log("Send from success");
		sending = true;
		Pebble.sendAppMessage(sendQueue[0].message, sendAppMessageSuccess, sendAppMessageFailure);
	}
}

function sendAppMessageFailure() {
	// console.log("sendAppMessageFailure");
	sending = false;
	const sent = sendQueue.shift();
	if (sent.failure)
		sent.failure();
	if (!sending && sendQueue.length) {
	// console.log("Send from failure");
		sending = true;
		Pebble.sendAppMessage(sendQueue[0].message, sendAppMessageSuccess, sendAppMessageFailure);
	}
}

function sendAppMessage(message, success, failure) {
	// console.log("sendAppMessage");
	sendQueue.push({message, success, failure});
	if (sending)
		return;

	// console.log("Send initial");
	sending = true;
	Pebble.sendAppMessage(sendQueue[0].message, sendAppMessageSuccess, sendAppMessageFailure);
}


function sendRequestMessage(request) {
	if ("http" === request.kind)
		sendRequestMessageHTTP(request);
	else if ("ws" === request.kind)
		sendRequestMessageWS(request);
	else
		throw new Error("unexpected request kind: " + request.kind);
}

function sendRequestMessageHTTP(request)
{
	sendAppMessage(
		request.messages.shift(),
		function () {
			if (request.messages.length)
				sendRequestMessage(request);
			else
				request.state = "done";
		},
		function () {
			console.log("http message send FAILED");

			sendAppMessage({
				[HTTP_BASE + 1]: request.id,
				[HTTP_BASE + 9]: -1						// done. failure.
			});
		}
	);
}

function sendRequestMessageWS(request) {
	sendAppMessage(
		request.messages.shift(),
		function () {
			if (request.messages.length)
				sendRequestMessage(request);
			else
				request.messages.sending = false;
		},
		function (e) {
			console.log("ws message send FAILED " + JSON.stringify(e));

			sendAppMessage({
				[WS_BASE + 1]: request.id,
				[WS_BASE + 3]: -1						// done. failure.
			});
		}
	);
	request.messages.sending = true;
}

function arrayToString(a) {
	return String.fromCharCode(...a);
}

function arrayToUint8Array(a) {
	return Uint8Array.from(a);
}

function stringToArray(str) {
	const result = [];

	for (let i = 0; i < str.length; i++) {
		const charCode = str.charCodeAt(i);

		if (charCode < 0x80)
			result.push(charCode);
		else if (charCode < 0x800)
			result.push(	0xc0 | (charCode >> 6),
							0x80 | (charCode & 0x3f));
		else if (charCode < 0xd800 || charCode >= 0xe000)
			result.push(	0xe0 | (charCode >> 12),
							0x80 | ((charCode >> 6) & 0x3f),
							0x80 | (charCode & 0x3f));
		else {
			i++;
			if (i >= str.length)
				throw new Error('Unmatched surrogate pair');

			const surrogate1 = charCode, surrogate2 = str.charCodeAt(i);
			const codePoint = 0x10000 + ((surrogate1 - 0xd800) << 10) + (surrogate2 - 0xdc00);

			result.push(	0xf0 | (codePoint >> 18),
							0x80 | ((codePoint >> 12) & 0x3f),
							0x80 | ((codePoint >> 6) & 0x3f),
							0x80 | (codePoint & 0x3f));
		}
	}

	return result;
}

const state = {
	appMessageReceived,
	readyReceived,
	sendAppMessage,
	log: false
};

module.exports = state;
