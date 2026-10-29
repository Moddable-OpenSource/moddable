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

import Session from "ssl/session";
import Timer from "timer";

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
	#error;
	#session;
	#callbacks;
	#data;
	#format;		// true == buffer, false = number
	#doRead;
	#tickets;		// the Key-Value store of TLS 1.3 session tickets, open for the life of the socket

	constructor(options) {
		const {TCP, tls, format, target, onReadable, onWritable, onError, ...tcp} = options;

		if (!tls)
			throw new Error("tls required");
		const host = tls.host;
		if (!host)
			throw new Error("tls.host required");

		let maximumVersion = Session.maximumVersion ?? 0x303;		// the session's: TLS 1.2 at most for one that does not say
		let minimumVersion;
		if (undefined !== tls.minimumVersion) {
			minimumVersion = toProtocolVersion(tls.minimumVersion);
			if (minimumVersion > maximumVersion)
				throw new RangeError("tls.minimumVersion: unsupported");
		}
		if (undefined !== tls.maximumVersion) {
			const version = toProtocolVersion(tls.maximumVersion);
			if (version < maximumVersion)
				maximumVersion = version;
		}
		if (undefined === minimumVersion)
			minimumVersion = (maximumVersion < defaultMinimumVersion) ? maximumVersion : defaultMinimumVersion;
		else if (minimumVersion > maximumVersion)
			throw new RangeError("tls.minimumVersion: greater than tls.maximumVersion");
		let tickets;

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
				case "keyShares":		// TLS 1.3: groups to send key shares for in the ClientHello (default 1; more avoids a HelloRetryRequest at the cost of generating keys the server may not use)
					session.tls13_key_shares = value;
					break;
				case "tickets":		// TLS 1.3 resumption: {io, path, max}, the Key-Value store to keep session tickets in (provider and path), and how many hosts to keep (default 8)
					if (!value.io || (undefined === value.path))
						throw new Error("tls.tickets: io and path required");
					if ((undefined !== value.max) && !(value.max >= 1))
						throw new RangeError("tls.tickets.max: invalid");
					tickets = value;
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

		if (tickets) {
			this.#tickets = tickets.io.open({path: tickets.path, format: "buffer"});
			session.tickets = this.#tickets;
			session.ticketsMax = tickets.max;
		}
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
	get error() {
		return this.#error;
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
		this.#session?.release?.();		// also when the handshake did not complete (the original session has no release)
		this.#socket?.close();
		this.#tickets?.close();
		Timer.clear(this.#doRead);

		this.#socket =
		this.#session = 
		this.#tickets =
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
		this.#callbacks.onError?.call(this, this.#error);		// the handshake or protocol error, if there was one
	}
	#messageHandler(read) {
		if (!this.#session)		// closed while reporting the error of the previous message
			return;
		if (!this.#ready) {
			try {		// the handshake runs here and in the read that follows it
				if (this.#session.handshake(this.#socket)) {
					this.#ready = true;
					this.#onWritable(this.#socket.writable);
				}
				else if (read)
					this.#session.read(this.#socket);
			}
			catch (e) {		// the handshake failed (certificate rejected, protocol error): report it, do not escape the callback
				this.#error = e;
				this.close();
				this.#onError();
				return;
			}
			if (!this.#ready || !this.#socket?.readable)
				return;
			// the handshake completed with records still buffered (a NewSessionTicket right behind the server's Finished, or data): read them now
		}

		let data;
		try {
			data = this.#session.read(this.#socket);
		}
		catch (e) {		// a protocol error after the handshake (a record that fails authentication, an alert): report it, do not escape the callback
			this.#error = e;
			this.close();
			return void this.#onError();
		}
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
