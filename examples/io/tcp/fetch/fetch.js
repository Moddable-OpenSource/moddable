/*
 * Copyright (c) 2021-2025  Moddable Tech, Inc.
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

class Response {
	#url;
	#status;
	#statusText;
	#headers;
	#body;
	#redirected;
	constructor(url, status, headers, body, redirected, statusText) {
		this.#url = url;
		this.#status = status;
		this.#headers = new Headers(headers);
		this.#body = body;
		this.#redirected = redirected;
		this.#statusText = statusText;
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
		return this.#statusText;
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
			rejectResponse(new URIError("only http or https"));
		const promiseBody = new Promise((resolveBody /*, rejectBody */) => {
			let method = info.method;
			let headers = info.headers;
			let body = info.body;
			let length = 0;
			if ((method == "POST") || (method == "PUT")) {
				body = info.body;
				if (body == undefined) 
					rejectResponse(new URIError(method + " no body"));
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
				}
				length = headers.get("content-length");
				if (length == undefined) {
					length = body.byteLength;
					const transferEncoding = headers.get("transfer-encoding");
					if (transferEncoding?.toLowerCase() != "chunked")
						headers.set("content-length", length);
				}
			}
			
			let redirected = false;
			let offset = 0;
			let buffer = null;
			const options = {
				method,
				headers,
				onHeaders(status, headers, statusText) {
					if ((301 === status) || (308 === status) || (302 === status) || (303 === status) || (307 === status)) {
						url = new URL(headers.get("location"));
						redirected = this.redirected = true;
						offset = 0;
						return;
					}
					resolveResponse(new Response(url, status, headers, promiseBody, redirected, statusText));
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
				onDone(/* error */) {
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
