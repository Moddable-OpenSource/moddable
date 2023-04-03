/*
 * Copyright (c) 2021-2023  Moddable Tech, Inc.
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

import Session from "ssl/session";
import Timer from "timer";

class TLSSocket {
	#socket;
	#ready = false;
	#session;
	#callbacks;
	#data;
	#format = true;		// true == buffer, false = number
	#doRead;

/*
	host (TLS server name indication extension)
	ALPNProtocols
	enableTrace
	minVersion - 'TLSv1.3', 'TLSv1.2', 'TLSv1.1', or 'TLSv1'
	macVersion - same (IGNORE?)
	ca: [PEM!!]
	certificate: [DER!],
	clientKey
	clientCertificates
 */

	constructor(options) {		
		this.#callbacks = {
			onReadable: options.onReadable,
			onWritable: options.onWritable,
			onError: options.onError
		}; 

		this.#session = new Session({
			tls_server_name: options.host,
			...options.secure,
			protocolVersion: 0x303,
			trace: false
		});
		this.#session.initiateHandshake();

		this.#socket = new options.TCP.io({
			...options,
			onReadable: count => this.#onReadable(count),
			onWritable: count => this.#onWritable(count),
			onError: () => this.#onError()
		});
		this.#socket.readable =
		this.#socket.writable = 0;
	}
	close() {
		this.#socket?.close();
		Timer.clear(this.#doRead);

		this.#socket =
		this.#session = 
		this.#doRead = undefined;
	}
	read(count) {
		const data = this.#data;
		if (!data)
			return;

		let result;
		if (this.#format) {
//@@ if count is a buffer, copy data into buffer
			if (count) {
				result = data.slice(data.position, data.position + count);
				data.position += count;
				if (data.position === data.byteLength)
					this.#data = undefined;
			}
			else {
				result = data.slice(data.position, data.end);
				this.#data = undefined;
			}
			result = result.buffer;
		}
		else {
			result = data[data.position++];
			if (data.position === data.byteLength)
				this.#data = undefined;
		}

		if (!this.#data && this.#socket.readable) {
			this.#doRead = Timer.set(() => {
				this.#doRead = undefined;
				if (this.#socket.readable)
					this.#onReadable(this.#socket.readable);
			});
		}

		return result;
	}
	write(buffer) {
		this.#session.write(this.#socket, buffer);
		return Math.max(0, this.#socket.writable - 96);		// not quite standard... like network protocols needed by client
	}
	set format(format) {
		this.#format = format === "buffer";
	}
	get format() {
		return this.#format ? "buffer" : "number";
	}
	#onWritable(count) {
		this.#socket.writable = count;
		if (!this.#ready)
			this.#messageHandler();
		else if (count > 96)			// 96 is an estimate of TLS overhead
			this.#callbacks.onWritable?.(count - 96);
	}
	#onReadable(count) {
		this.#socket.readable = count;
		if (this.#data)
			return;

		this.#messageHandler(true);
		if (!this.#ready)
			this.#messageHandler();
	}
	#onError() {
		this.#callbacks.onError?.();
	}
	#messageHandler(read) {
		if (!this.#ready) {
			if (this.#session.handshake(this.#socket)) {
				this.#ready = true;
				this.#onWritable(this.#socket.writable);
			}
			if (read)
				this.#session.read(this.#socket);
			return;
		}

		const data = this.#session.read(this.#socket);
		if (undefined === data)		// nothing to read
			return;
		if (null === data)		// closed
			return void this.#onError();
		data.position = 0;
		const readable = data.byteLength;
		if (!readable)
			return;
		this.#data = data;

		this.#callbacks.onReadable?.(readable);
	}
}

export default TLSSocket;
