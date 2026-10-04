/*
 * Copyright (c) 2021-2026  Moddable Tech, Inc.
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
import { ReadableStream } from "web/streams";
import DecompressionStream from "web/decompressionstream";

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
	get body() {
		let body = this.#body;
		this.#body = undefined;
		return body;
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
		let body = this.body;
		if (body) {
			const reader = body.getReader();
			let buffer = null;
			while (true) {
				const { done, value } = await reader.read();
				if (value) {
					const slice = value.buffer.slice(value.byteOffset, value.byteOffset + value.byteLength);
					if (buffer)
						buffer = buffer.concat(slice);
					else
						buffer = slice;
				}
				if (done) {
					return buffer;
				}
			}
		}
	}
	async json() {
		const text = await this.text();
		if (text) {
			return JSON.parse(text);
		}
	}
	async text() {
		const buffer = await this.arrayBuffer();
		if (buffer) {
			return String.fromArrayBuffer(buffer);
		}
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
			client = new device.network.http.client.io({ 
				...device.network.http.client,
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
			client = new device.network.https.client.io({ 
				...device.network.https.client,
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
	return client;
}

function toBytes(body) {
	if (ArrayBuffer.isView(body))
		return new Uint8Array(body.buffer, body.byteOffset, body.byteLength);
	if ((body instanceof ArrayBuffer) || (body instanceof SharedArrayBuffer))
		return new Uint8Array(body);
	return new Uint8Array(ArrayBuffer.fromString(body.toString()));
}

function fetch(href, info = {}) {
	return new Promise((resolveResponse, rejectResponse) => {
		let url = new URL(href);
		if ((url.protocol != "http:") && (url.protocol != "https:"))
			rejectResponse(new URIError("only http or https"));
		let method = info.method;
		let reader = null;
		let body = info.body;
		let length = 0;

		let headers;
		const h = info.headers;
		if (h instanceof Headers)
			headers = new Headers(h);
		else {
			headers = new Headers;
			if (h) {
				for (const name in h)
					headers.set(name.toLowerCase(), h[name]);
			}
		}

		if ((method == "POST") || (method == "PUT") || (method == "PATCH")) {
			if (body == undefined)
				rejectResponse(new URIError(method + " no body"));
			else if (body instanceof ReadableStream) {
				reader = body.getReader();
				body = null;
				headers.set("transfer-encoding", "chunked");
			}
			else {
				body = toBytes(body);
				length = Number(headers.get("content-length") ?? body.byteLength);
				headers.set("content-length", length);
			}
		}

		if (!headers.has("accept-encoding"))
			headers.set("accept-encoding", "deflate, gzip;q=0.9");
		let readableController = null;
		let redirected = false;
		let offset = 0;
		let buffer = null;
		if (info.signal) {
			info.signal.addEventListener("abort", event => {
				client.close();
				clients.delete(url.origin);
				if (readableController) {
					if (event.signal.reason === null)
						readableController.close();
					else
						readableController.error(event.signal.reason);
					readableController = null;
				}
				else
					rejectResponse(event.signal.reason);
			});
		}
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
				const readableStream = new ReadableStream({
					start(controller) {
						readableController = controller;
						readableController.available = 0;
					},
					pull: (controller) => {
						const count = readableController.available;
						if (count > 0) {
							readableController.available = 0;
							readableController.enqueue(new Uint8Array(this.read(count)));
						}
						else {
							readableController.promiseRecord = Promise.withResolvers();
							return readableController.promiseRecord.promise;
						}
					},
					cancel: () => {
						client.close();
						clients.delete(url.origin);
					}
				});

				let body = readableStream;
				const encoding = headers.get("content-encoding");
				if (("gzip" === encoding) || ("deflate" === encoding)) {
					body = readableStream.pipeThrough(new DecompressionStream(encoding));
					headers.delete("content-encoding");
				}
				resolveResponse(new Response(url.href, status, headers, body, redirected, statusText));
			},
			onWritable(count) {
				const writeBody = () => {
					let remain = length - offset;
					if (remain > 0) {
						if (count > remain)
							count = remain;
						this.write(body.subarray(offset, offset + count));
						offset += count;
					}
					else if (!reader)
						this.write();
				}
				if (body) {
					writeBody()
				}
				if (offset == length) {
					if (reader) {
						body = null;
						length = 0;
						offset = 0;
						reader.read().then(result => {
							if (result.done) {
								this.write();
								reader.releaseLock();
								return;
							}
							body = toBytes(result.value);
							length = body.byteLength;
							writeBody();
						});
					}
				}
			},
			onReadable(count) {
				if (count == 0)
					return;
				if (this.redirected) {
					this.read();
					return;
				}
				if (readableController) {
					const promiseRecord = readableController.promiseRecord;
					if (promiseRecord) {
						readableController.promiseRecord = null;
						readableController.enqueue(new Uint8Array(this.read(count)));
						promiseRecord.resolve();
					}
					else
						readableController.available = count;
				}
			},
			onDone(/* error */) {
				if (this.redirected) {
					client = fetchClientRequest(url, options);
					return;
				}
				if (readableController) {
					readableController.close();
					readableController = null;
				}
			}
		};
		let client = fetchClientRequest(url, options);
		if (headers?.get("accept") == "text/event-stream") {
			clients.delete(url.origin);
		}	
	});
}

export { fetch, Headers }
export default fetch;
