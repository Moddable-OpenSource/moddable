/*
 * Copyright (c) 2021-2023  Moddable Tech, Inc.
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
 
import URL from "url";

let clients;
function Client(url) {
	clients ??= new Map;
	let origin = url.origin;
	let client = clients.get(origin);
	if (!client) {
		const protocol = url.protocol;
		const host = url.hostname;
		if (protocol == "http:") {
			const port = url.port || 80;
			client = new device.network.http.io({ 
				...device.network.http,
				host, 
				port,  
				onError() {
					clients.delete(origin);
					this.close();
				}
			});
		}
		else {
			const port = url.port || 443;
			client = new device.network.https.io({ 
				...device.network.https,
				host, 
				port,  
				onError() {
					clients.delete(origin);
					this.close();
				}
			});
		}
		clients.set(origin, client);
	}
	return client;
}

const statusTexts = {
	100: "Continue",
	101: "Switching Protocols",
	200: "OK",
	201: "Created",
	202: "Accepted",
	203: "Non-Authoritative Information",
	204: "No Content",
	205: "Reset Content",
	206: "Partial Content",
	300: "Multiple Choices",
	301: "Moved Permanently",
	302: "Found",
	303: "See Other",
	304: "Not Modified",
	305: "Use Proxy",
	307: "Temporary Redirect",
	400: "Bad Request",
	401: "Unauthorized",
	402: "Payment Required",
	403: "Forbidden",
	404: "Not Found",
	405: "Method Not Allowed",
	406: "Not Acceptable",
	407: "Proxy Authentication Required",
	408: "Request Timeout",
	409: "Conflict",
	410: "Gone",
	411: "Length Required",
	412: "Precondition Failed",
	413: "Payload Too Large",
	414: "URI Too Long",
	415: "Unsupported Media Type",
	416: "Range Not Satisfiable",
	417: "Expectation Failed",
	426: "Upgrade Required",
	500: "Internal Server Error",
	501: "Not Implemented",
	502: "Bad Gateway",
	503: "Service Unavailable",
	504: "Gateway Timeout",
	505: "HTTP Version Not Supported",
};
Object.freeze(statusTexts);

class Headers extends Map {
	delete(key) {
		return super.delete(key.toString().toLowerCase());
	}
	get(key) {
		return super.get(key.toString().toLowerCase());
	}
	has(key) {
		return super.has(key.toString().toLowerCase());
	}
	set(key, value) {
		return super.set(key.toString().toLowerCase(), value.toString());
	}
}

class Response {
	#url;
	#status;
	#headers;
	#body;
	constructor(url, status, headers, body) {
		this.#url = url;
		this.#status = status;
		this.#headers = new Headers(headers);
		this.#body = body;
	}
	get bodyUsed() {
		return this.#body ? false : true;
	}
	get headers() {
		return this.#headers;
	}
	get ok() {
		return 200 <= this.#status && this.#status <= 299;
	}
	get status() {
		return this.#status;
	}
	get statusText() {
		return statusTexts[this.#status];
	}
	get url() {
		return this.#url;
	}
	async arrayBuffer() {
		let body = this.#body;
		if (body) {
			this.#body = undefined;
			body = await body;
		}
		return body;
	}
	async json() {
		let body = this.#body;
		if (body) {
			this.#body = undefined;
			body = await body;
			body = String.fromArrayBuffer(body);
			return JSON.parse(body);
		}
		return body;
	}
	async text() {
		let body = this.#body;
		if (body) {
			this.#body = undefined;
			body = await body;
			body = String.fromArrayBuffer(body);
		}
		return body;
	}
}

function fetch(href, info = {}) {
	return new Promise((resolveResponse, rejectResponse) => {
		const url = new URL(href);
		if ((url.protocol != "http:") && (url.protocol != "https:"))
			rejectResponse(new URLError("only http or https"));
		const responseBody = new Promise((resolveBody, rejectBody) => {
			let path = url.pathname;
			let query = url.search;
			if (query)
				path += query;
			let method = info.method;
			let headers = info.headers;
			let body = info.body;
			let offset = 0;
			let length = 0;
			if ((method == "POST") || (method == "PUT")) {
				body = info.body;
				if (body == undefined) 
					rejectResponse(new URLError(method + " no body"));
				else if (!(body instanceof ArrayBuffer)) {
					body = body.toString();
					body = ArrayBuffer.fromString(body);
				}
				if (!headers)
					headers = new Headers();
				else {
					length = headers.get("content-length");
					if (length == undefined) {
						headers = new Headers(headers);
						length = body.byteLength;
						headers.set("content-length", length);
					}
				}
			}
			const client = Client(url);
			let buffer = null;
			client.request({
				method,
				path,
				headers,
				onHeaders(status, headers) {
					resolveResponse(new Response(url, status, headers, responseBody));
				},
				onReadable(count) {
					if (buffer)
						buffer = buffer.concat(this.read(count));
					else
						buffer = this.read(count);
				},
				onWritable(count) {
					if (body) {
						if (length > 0) {
							if (count > length)
								count = length;
							let view = new DataView(body, offset, count);
							this.write(view);
							offset += count;
							length -= count;
						}
						else
							this.write();
					}
				},
				onDone(error) {
					resolveBody(buffer ?? new ArrayBuffer);
				}
			});
		});
	});
}

export { fetch, Headers }
export default fetch;


