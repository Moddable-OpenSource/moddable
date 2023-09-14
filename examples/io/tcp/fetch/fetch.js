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
 
import Headers from "headers";
import URL from "url";

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

class Response {
	#url;
	#status;
	#headers;
	#body;
	#redirected;
	constructor(url, status, headers, body, redirected) {
		this.#url = url;
		this.#status = status;
		this.#headers = new Headers(headers);
		this.#body = body;
		this.#redirected = redirected;
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
	get redirected() {
		return this.#redirected;
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

let clients;
function fetchClientRequest(url, options) {
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
	let path = url.pathname;
	let query = url.search;
	if (query)
		path += query;
	options.path = path;
	client.request(options);
}

function fetch(href, info = {}) {
	return new Promise((resolveResponse, rejectResponse) => {
		let url = new URL(href);
		if ((url.protocol != "http:") && (url.protocol != "https:"))
			rejectResponse(new URLError("only http or https"));
		const promiseBody = new Promise((resolveBody, rejectBody) => {
			let method = info.method;
			let headers = info.headers;
			let body = info.body;
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
					if (!(headers instanceof Headers)) {
						const h = new Headers();
						for (const name in headers)
							h.set(name.toLowerCase(), headers[name]);
						headers = h;
					}
					length = headers.get("content-length");
					if (length == undefined) {
						headers = new Headers(headers);
						length = body.byteLength;
						headers.set("content-length", length);
					}
				}
			}
			
			let redirected = false;
			let offset = 0;
			let buffer = null;
			const options = {
				method,
				headers,
				onHeaders(status, headers) {
					if ((301 === status) || (308 === status) || (302 === status) || (303 === status) || (307 === status)) {
						url = new URL(headers.get("location"));
						redirected = this.redirected = true;
						offset = 0;
						return;
					}
					resolveResponse(new Response(url, status, headers, promiseBody, redirected));
				},
				onWritable(count) {
					if (body) {
						let remain = length - offset;
						if (remain > 0) {
							if (count > remain)
								count = remain;
							let view = new DataView(body, offset, count);
							this.write(view);
							offset += count;
						}
						else
							this.write();
					}
				},
				onReadable(count) {
					if (count == 0)
						return;
					if (this.redirected)
						this.read();
					else if (buffer)
						buffer = buffer.concat(this.read(count));
					else
						buffer = this.read(count);
				},
				onDone(error) {
					if (this.redirected) {
						fetchClientRequest(url, options);
						return;
					}
					resolveBody(buffer ?? new ArrayBuffer);
				}
			};
			fetchClientRequest(url, options);
		});
	});
}

export { fetch, Headers }
export default fetch;
