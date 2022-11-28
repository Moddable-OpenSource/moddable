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
 
export default {
	onRequest(request) {
		this.valid =	("GET" === request.method) &&
						("text/event-stream" === request.headers.get("accept")?.toLowerCase());
	},
	onResponse(response) {
		if (!this.valid) {
			response.status = 400;
			this.respond(response);
			return;
		}

		response.headers.set("content-type", "text/event-stream");
		response.headers.set("cache-control", "no-cache, no-transform");
		response.headers.set("connection", "keep-alive");
		response.headers.set("transfer-encoding", "chunked");
		this.respond(response);
	},
	onWritable(count) {
		if (this.valid) {
			delete this.valid;
			Object.defineProperty(this, "write", {value: sseWrite});
//			Object.defineProperty(this, "queue", {value: []});
			this.route.onConnection?.call(this);
		}
	}
}
function sseWrite(msg) {
	let parts;
	if (msg) {
		parts = (msg.data ?? "").split("\n");
		for (let i = 0; i < parts.length; i++)
			parts[i] = "data: " + parts[i];
		if (msg.event)
			parts.push(`event: ${msg.event}`);
		if (msg.id)
			parts.push(`id: ${msg.id}`);
	}
	else
		parts = [":"];		// for keep-alive
	parts.push("", "");		// end of message
	
	const buffer = ArrayBuffer.fromString(parts.join("\n"));
//	this.queue.push(buffer);
	this.__proto__.write.call(this, buffer);
}
