/*
 * Copyright (c) 2019-2021  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import TCP from "embedded:io/socket/tcp";

class HTTPGet {
	#host;
	#path;
	#port;
	#socket;
	#onData;
	#onError;
	constructor(options) {
		this.#host = options.host;
		this.#path = options.path;
		this.#port = options.port ?? 80;

		this.#onData = options.onData;
		this.#onError = options.onError;

		if (!this.#onData || !this.#path)
			throw new Error("parameter error");

		System.resolve(this.#host, (host, address) => {
			if (!address) {
				this.#onError?.();
				return;
			}

			this.#socket = new TCP({
				target: this,
				address,
				port: this.#port,
				onReadable: this.readable,
				onWritable: this.writeable,
				onError: this.error,
			});
		});
	}
	readable(byteLength) {
		const target = this.target;
		const buffer = target.#socket.read(byteLength);
		target.#onData(buffer);
	}
	writeable(byteLength) {
		const target = this.target;
		if (target.#path) {
			const headers = [
				`GET ${target.#path} HTTP/1.1`,
				`Host: ${target.#host}`,
				"Connection: close",
				"",
				"",
			].join("\r\n")
			this.write(ArrayBuffer.fromString(headers));

			target.#path = undefined;
		}
	}
	error() {
		const target = this.target;
		target.#onError?.();
		target.#socket.close();
		target.#socket = undefined;
	}
}

new HTTPGet({
	host: "httpbin.org",
	port: 80,
	path: "/",
	onData(buffer) {
		trace(String.fromArrayBuffer(buffer));
	},
	onError() {
		trace("\n\n** DONE **\n\n");
	}
});
