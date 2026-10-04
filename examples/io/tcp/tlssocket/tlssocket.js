/*
 * Copyright (c) 2021-2026  Moddable Tech, Inc.
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

const maximumSupportedVersion = 0x303;
const defaultMinimumVersion = 0x303;

function toProtocolVersion(value) {
	switch (value) {
		case "TLSv1.1": return 0x302;
		case "TLSv1.2": return 0x303;
		case "TLSv1.3": return 0x304;
	}
	throw new RangeError(`${value}: unsupported`);
}

class TLS {
	#socket;
	#ready = false;
	#session;
	#callbacks;
	#data;
	#format;		// true == buffer, false = number
	#doRead;

	constructor(options) {
		const {TCP, tls, format, target, onReadable, onWritable, onError, ...tcp} = options;

		if (!tls)
			throw new Error("tls required");
		const host = tls.host;
		if (!host)
			throw new Error("tls.host required");

		let maximumVersion = maximumSupportedVersion;
		if (undefined !== tls.maximumVersion) {
			maximumVersion = toProtocolVersion(tls.maximumVersion);
			if (maximumVersion > maximumSupportedVersion)
				maximumVersion = maximumSupportedVersion;
		}
		let minimumVersion = (maximumVersion < defaultMinimumVersion) ? maximumVersion : defaultMinimumVersion;
		if (undefined !== tls.minimumVersion) {
			minimumVersion = toProtocolVersion(tls.minimumVersion);
			if (minimumVersion > maximumSupportedVersion)
				throw new RangeError("tls.minimumVersion: unsupported");
			if (minimumVersion > maximumVersion)
				throw new RangeError("tls.minimumVersion: greater than tls.maximumVersion");
		}

		const session = {
			tls_server_name: String(host),
			protocolVersion: maximumVersion,
			minProtocolVersion: minimumVersion,
			maxProtocolVersion: maximumVersion
		};
		for (const name in tls) {
			const value = tls[name];
			switch (name) {
				case "host":
				case "minimumVersion":
				case "maximumVersion":
					break;		// applied above
				case "applicationLayerProtocol":
					session.tls_application_layer_protocol_negotiation = value;
					break;
				case "maximumFragmentLength":
					session.tls_max_fragment_length = value;
					break;
				case "ca":
					session.certificate = value;
					break;
				case "clientCertificate":
					session.clientCertificates = Array.isArray(value) ? value : [value];
					break;
				case "clientKey":
					session.clientKey = Array.isArray(value) ? value[0] : value;		// one client key supported
					break;
				case "trace":
				case "verify":
					session[name] = value;
					break;
			}
		}

		this.#callbacks = {onReadable, onWritable, onError};
		if (undefined !== target)
			this.target = target;
		this.format = format ?? "buffer";

		this.#session = new Session(session);
		this.#session.initiateHandshake();

		this.#socket = new TCP.io({
			...TCP,
			...tcp,
			onReadable: count => this.#onReadable(count),
			onWritable: count => this.#onWritable(count),
			onError: () => this.#onError()
		});
		this.#socket.readable =
		this.#socket.writable = 0;
	}
	close() {
		if (this.#ready) {
			try {
				this.#session?.close(this.#socket);
			}
			catch {
				/* this space intentionally left blank */
			}
		}
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
			if (undefined !== count) {
				const available = data.byteLength - data.position;
				if ("object" === typeof count) {
					result = count.byteLength;
					if (result > available)
						result = available;

					const buffer = ArrayBuffer.isView(count) ? new Uint8Array(count.buffer, count.byteOffset, result) : new Uint8Array(count.buffer);
					buffer.set(data.subarray(data.position, data.position + result)); 
					data.position += result;
				}
				else {
					if (count > available)
						throw new Error("invalid")
					result = data.slice(data.position, data.position + count).buffer;
					data.position += count;
				}
				if (data.position === data.byteLength)
					this.#data = undefined;
			}
			else {	// could optimize when entire buffer is requested
				result = data.slice(data.position, data.position + data.byteLength).buffer;
				this.#data = undefined;
			}
		}
		else {
			result = data[data.position++];
			if (data.position === data.byteLength)
				this.#data = undefined;
		}

		if (!this.#data && this.#socket.readable) {
			this.#doRead ??= Timer.set(() => {
				this.#doRead = undefined;
				if (this.#socket.readable)
					this.#onReadable(this.#socket.readable);
			});
		}

		return result;
	}
	write(buffer, options) {
		if (buffer instanceof DataView)
			buffer = new Uint8Array(buffer.buffer, buffer.byteOffset, buffer.byteLength);
		return this.#session.write(this.#socket, buffer, options);
	}
	set format(format) {
		if (("buffer" != format) && ("number" != format))
			throw new RangeError;
		this.#format = format == "buffer";
	}
	get format() {
		return this.#format ? "buffer" : "number";
	}
	get remoteAddress() {
		return this.#socket.remoteAddress;
	}
	get remotePort() {
		return this.#socket.remotePort;
	}
	#onWritable(count) {
		this.#socket.writable = count;
		if (!this.#ready)
			this.#messageHandler();
		else if (count > 96)			// 96 is an estimate of TLS overhead
			this.#callbacks.onWritable?.call(this, count - 96);
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
		this.#callbacks.onError?.call(this);
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

		this.#callbacks.onReadable?.call(this, readable);
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default TLS;
