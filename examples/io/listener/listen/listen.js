/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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
 
import HTTPServer from "embedded:network/http/server"
import Listener from "embedded:io/socket/listener";

import URL from "./url";

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

class Request {
	#url;
	#method;
	#headers;
	#body;
	constructor(url, options) {
		this.#url = url;
		this.#method = options.method;
		this.#headers =  options.headers;
		this.#body = options.body;
	}
	get bodyUsed() {
		return this.#body ? false : true;
	}
	get headers() {
		return this.#headers;
	}
	get method() {
		return this.#method;
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
			if (body) {
				body = String.fromArrayBuffer(body);
				return JSON.parse(body);
			}
		}
		return body;
	}
	async text() {
		let body = this.#body;
		if (body) {
			this.#body = undefined;
			body = await body;
			if (body)
				body = String.fromArrayBuffer(body);
		}
		return body;
	}
}

class Response {
	#body;
	#headers;
	#status = 200;
	constructor(body, options) {
		if (!(body instanceof ArrayBuffer)) {
			body = body.toString();
			body = ArrayBuffer.fromString(body);
		}
		this.#body = body;
		
		const headers = (options && options.headers) ? options.headers : new Headers; 
		if ((headers.get("content-length") == undefined)) {
			headers.set("content-length", body.byteLength);
		}
		this.#headers = headers;
		
		this.#status = (options && options.status) ? options.status : 200; 
	}
	get body() {
		return this.#body;
	}
	get headers() {
		return this.#headers;
	}
	get status() {
		return this.#status;
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

async function* listen(options) {
	const connectionQueue = [];
	const promiseQueue = [];
	const port = options?.port ?? 80;
	const base = "http://localhost:" + port;
	const server = new HTTPServer({ 
		io: Listener,
		port,
		onConnect(_connection_) {
			let requestPromise = new Promise((resolveRequest, rejectRequest) => {
				let requestBody = null;
				let responsePromise = new Promise((resolveResponse, rejectResponse) => {
					let responseBody = null;
					let offset = 0;
					let length = 0;
					_connection_.accept({
						onRequest(_request_) {
							const { method, path, headers } = _request_;
							const request = new Request(new URL(path, base), { method, path, headers, requestPromise });
							const connection = {
								close() {
									_connection_.close();
								},
								async respondWith(response) {
									response = await response;
									responseBody = await response.arrayBuffer();
									if (responseBody)
										length = responseBody.byteLength;
									const _response_ = await responsePromise;
									_response_.status = response.status;
									_response_.headers = response.headers;
									_connection_.respond(_response_);
								}
							};
							Object.defineProperty(connection, "request", { value:request, enumerable:true }); // read only
							if (promiseQueue.length == 0) {
								connectionQueue.push(connection);
							}
							else {
								const promise = promiseQueue.shift();
								promise.resolveEvent(connection);
							}
						},
						onReadable(count) {
							if (requestBody)
								requestBody = requestBody.concat(this.read(count));
							else
								requestBody = this.read(count);
						},
						onResponse(_response_) {
							resolveRequest(requestBody);
							resolveResponse(_response_);
						},
						onWritable(count) {
							if (responseBody) {
								if (length > 0) {
									if (count > length)
										count = length;
									let view = new DataView(responseBody, offset, count);
									this.write(view);
									offset += count;
									length -= count;
								}
								else
									this.write();
							}
							else
								this.write();
						},
						onError(message) {
							rejectRequest(error);
							rejectResponse(error);
						}
					});
				});
			});
		},
		onError(message) {
			const error = new Error(message);
			if (promiseQueue.length == 0) {
				connectionQueue.push(error);
			}
			else {
				const promise = promiseQueue.shift();
				promise.rejectEvent(error);
			}
		}
	})
	while (true) {
		yield new Promise((resolveEvent, rejectEvent) => {
			if (connectionQueue.length == 0)
				promiseQueue.push({ resolveEvent, rejectEvent });
			else {
				const connection = connectionQueue.shift();
				if (connection instanceof Error)
					rejectEvent(connection);
				else
					resolveEvent(connection);
			}
		})
	}
}

export { Headers, Response }
export default listen;


