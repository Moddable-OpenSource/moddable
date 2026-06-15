/*
 * Copyright (c) 2019-2025  Moddable Tech, Inc.
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
 
class Inflate extends Native("xs_inflate_destructor") {
	chunks = [];
	strm = {avail_in: 0};

	constructor(options = {}) {
		super();
		native("xs_inflate").call(this, options);

		this.onData = onData;
		this.onEnd = onEnd;
	}
	close() { return native("xs_inflate_close").call(this); }

	push(buffer, flush, output) {
		this.strm.avail_in = native("xs_inflate_push").call(this, buffer, flush, output);
	}
}

function onData(chunk) {
	this.chunks.push(chunk);
}

function onEnd(err) {
	const chunks = this.chunks, length = chunks.length;
	delete this.chunks;

	this.err = err;
	if (err)
		return;

	// join chunks
	let total = 0;
	for (let i = 0; i < length; i++)
		total += chunks[i].byteLength;

	this.result = new Uint8Array(total);
	for (let i = 0, offset = 0; i < length; i++) {
		const chunk = chunks[i];
		this.result.set(new Uint8Array(chunk), offset);
		offset += chunk.byteLength;
	}
}

export default Inflate;
