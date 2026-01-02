/*
 * Copyright (c) 2021-2025  Moddable Tech, Inc.
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
 
import TextEncoder from "text/encoder";

export default {
	onResponse(response) {
		const route = this.route, data = route.data;
		route.state ??= new WeakMap;
		const state = {data};
		route.state.set(this, state);
		if (data instanceof ArrayBuffer) { 
			state.position = 0;
			state.byteLength = data.byteLength;
			response.headers.set("content-length", state.byteLength);
		}
		else if (ArrayBuffer.isView(data)) {
			state.position = data.byteOffset;
			state.byteLength = data.byteLength;
			state.data = data.buffer;
			response.headers.set("content-length", state.byteLength);
		}
		else if ("string" === typeof data) {
			state.encoder = new TextEncoder;
			state.position = 0;
			response.headers.set("transfer-encoding", "chunked");
		}
		else
			throw new Error("unsupported data type");
		response.headers.set("content-type", route.contentType ?? "text/html");
		response.status = this.route.status ?? 200;
		this.respond(response);
	},
	onWritable(count) {
		const state = this.route.state.get(this);
		if (state.encoder) {
			if (state.position === state.data.length)
				return void this.write();

			const buffer = new Uint8Array(count);
			const result = state.encoder.encodeInto(state.data.slice(state.position, state.position + count), buffer);
			this.write(buffer.subarray(0, result.written));
			state.position += result.read;
		}
		else {
			if (count > state.byteLength)
				count = state.byteLength;
			this.write(new Uint8Array(state.data, state.position, count));
			state.position += count;
			state.byteLength -= count;
		}
	}
}
