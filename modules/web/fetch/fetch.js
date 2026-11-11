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
function fetchClientRequest(url, options, embedded) {
	clients ??= new Map;
	const origin = url.origin;
	const protocol = url.protocol;
	const tls = ("https:" === protocol) ? embedded?.tls : undefined;
	let client = clients.get(origin);
	if (client && (client.tls !== tls))
		client = undefined;		// TLS options differ (by identity): don't reuse
	if (!client) {
		const host = url.hostname;
		function onError() {
			if (clients.get(origin) === this)
				clients.delete(origin);
			this.close();
		}
		if (protocol == "http:") {
			const port = url.port || 80;
			client = new device.network.http.client.io({
				...device.network.http.client,
				host,
				port,
				onError
			});
		}
		else {
			const port = url.port || 443;
			const https = device.network.https.client;
			client = new https.io({
				...https,
				...(tls && {socket: {...https.socket, tls: {...https.socket.tls, ...tls}}}),
				host,
				port,
				onError
			});
		}
		client.tls = tls;
		clients.set(origin, client);
	}
	let path = url.pathname;
	let query = url.search;
	if (query)
		path += query;
	options.path = path;
	options.client = client;
	options.origin = origin;
	client.request(options);
	if (("close" === options.headers?.get?.("connection")?.toLowerCase()) && (clients.get(origin) === client))
		clients.delete(origin);
}

function fetch(href, info = {}) {
	return new Promise((resolveResponse, rejectResponse) => {
		let url = new URL(href);
		if ((url.protocol != "http:") && (url.protocol != "https:"))
			return void rejectResponse(new URIError("only http or https"));
		let responded = false;
		const promiseBody = new Promise((resolveBody, rejectBody) => {
			let method = info.method;
			let headers = info.headers;
			let body;
			let length = 0, chunked = false;
			if ((method !== undefined) && (method !== "GET") && (method !== "HEAD")) {
				body = info.body;
				if (body == undefined)
					body = new Uint8Array(0);		// no body: send an empty one, as the Fetch standard
				else if (ArrayBuffer.isView(body))
					body = new Uint8Array(body.buffer, body.byteOffset, body.byteLength);
				else if ((body instanceof ArrayBuffer) || (body instanceof SharedArrayBuffer))
					body = new Uint8Array(body);
				else
					body = new Uint8Array(ArrayBuffer.fromString(body));
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
				if (undefined === length) {
					length = body.byteLength;
					const transferEncoding = headers.get("transfer-encoding");
					chunked = transferEncoding?.toLowerCase() == "chunked";
					if (!chunked)
						headers.set("content-length", length);
				}
			}
			
			let redirected = false, redirects = 0, failed = false;
			let offset = 0;
			let buffer = null;
			const options = {
				method,
				headers,
				onHeaders(status, headers, statusText) {
					options.close = ("close" === headers.get("connection")?.toLowerCase()) || ("close" === options.headers?.get?.("connection")?.toLowerCase());
					const location = headers.get("location");
					if (location && ((301 === status) || (308 === status) || (302 === status) || (303 === status) || (307 === status))) {		// without a location, the redirect is the response
						this.redirected = true;
						try {
							if (++redirects > 20)
								throw new TypeError("too many redirects");
							url = new URL(location, url.href);		// relative to the current URL, as the Fetch standard
						}
						catch (e) {		// called from the HTTP client: report, never throw
							failed = true;
							return void rejectResponse(e);
						}
						redirected = true;
						offset = 0;
						return;
					}
					responded = true;
					resolveResponse(new Response(url.href, status, headers, promiseBody, redirected, statusText));
				},
				onWritable(count) {
					if (body) {
						let remain = length - offset;
						if (remain > 0) {
							if (count > remain)
								count = remain;
							this.write(body.subarray(offset, offset + count));
							offset += count;
						}
						else if (chunked && (offset !== Infinity)) {
							offset = Infinity;
							this.write();
						}
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
					if (error) {
						if (responded)
							rejectBody(error);
						else
							rejectResponse(error)
						return;
					}
					if (options.close && (clients.get(options.origin) === options.client))
						clients.delete(options.origin);
					if (failed)
						return;
					if (this.redirected) {
						fetchClientRequest(url, options, info.embedded);
						return;
					}
					resolveBody(buffer ?? new ArrayBuffer);
				}
			};
			fetchClientRequest(url, options, info.embedded);
		});
		promiseBody.catch(() => {});
	});
}

export { fetch, Headers }
export default fetch;
