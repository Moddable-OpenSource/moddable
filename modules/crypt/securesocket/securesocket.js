/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

import {Socket} from "socket";
import Session from "ssl/session";

class SecureSocket {
	constructor(dict) {
		const sock = dict.sock ?? new Socket(dict);

		this.sock = sock;
		this.handshaking = true;
		try {
			this.ssl = new Session({tls_server_name: dict.host, ...dict.secure});
		}
		catch (e) {
			sock.close();
			throw e;
		}
		sock.callback = (message, value) => {
			if (this.closing)
				return;

			try {
				switch (message) {
					case Socket.connected:
						this.ssl.initiateHandshake(this.sock);
						this.messageHandler(0);
						break;
					case Socket.readable:
						do {
							this.messageHandler(value);
							if (this.closing)
								return;
							value = this.sock.read();
						} while (value && !this.closing);
						this.messageHandler(0);
						break;
					case Socket.writable:
						if (this.handshaking)
							this.messageHandler(0);
						else {
							if (value > 128)			//@@ 128 is an estimate of TLS overhead
								this.callback(3, value - 128)
						}
						break;
					default:
						this.closing = message;
						break;
				}
			}
			catch (e) {
				this.closing = Socket.error;
			}

			if (this.closing) {
				this.callback(this.closing);
				this.close();
			}
		}
	}
	read() @ "xs_securesocket_read";
	write() @ "xs_securesocket_write";
	close() {
		this.closing = true;
		this.sock?.close();
		delete this.ssl;
		delete this.sock;
	}

	messageHandler(bytesAvailable) {
		if (this.handshaking) {
			if (this.ssl.handshake(this.sock, bytesAvailable)) {
				delete this.handshaking;
				this.callback(1);
			}
			return;
		}

		if (bytesAvailable <= 0)
			return;

		if (0 == this.ssl.bytesAvailable) {
			// decode a packet to get how many bytes available in plain
			if (!this.ssl.read(this.sock))
				return;
		}

		while (!this.closing) {
			bytesAvailable = this.ssl.bytesAvailable;
			if (bytesAvailable <= 0)
				break;
			if (bytesAvailable > 1024)
				bytesAvailable = 1024;

			this.data = this.ssl.read(this.sock, bytesAvailable);	// returns Uint8Array
			this.data.position = this.data.byteOffset;
			this.data.end = this.data.position + this.data.byteLength;
			this.callback(2, this.data.byteLength);
		}
		delete this.data;
	}
}

export default SecureSocket;
