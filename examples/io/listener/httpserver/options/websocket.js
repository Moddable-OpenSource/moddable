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
 
import Base64 from "base64";
import {Digest} from "crypt";

export default {
	onRequest(request) {
		const connection = request.headers.get("connection");
		this.key = request.headers.get("sec-websocket-key");
		this.valid =	("GET" === request.method) &&
						("websocket" === request.headers.get("upgrade")?.toLowerCase()) &&
						("13" === request.headers.get("sec-websocket-version")) &&
						(connection && connection.split(",").some(item => item.trim().toLowerCase() === "upgrade")) &&
						!!this.key;
		let protocol = request.headers.get("sec-websocket-protocol");
		//@@ parse out subprotocol
	},
	onResponse(response) {
		if (!this.valid) {
			response.status = 400;
			this.respond(response);
			return;
		}

		const sha1 = new Digest("SHA1");
		sha1.write(this.key);
		sha1.write("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

		response.status = 101;		// " Web Socket Protocol Handshake" - status msg
		response.headers.set("connection", "upgrade");
		response.headers.set("upgrade", "websocket");
		response.headers.set("sec-websocket-accept", Base64.encode(sha1.close()));

		if (this.protocol)
			response.headers.set("sec-websocket-protocol", this.protocol);

		this.respond(response);

		delete this.protocol;
		delete this.key;
		delete this.valid;
	},
	onDone() {
		debugger;
	},
}
