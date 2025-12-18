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
 

class Deflate extends Native("xs_deflate_destructor") {
	err = 0;
	chunks = [];

	constructor(options = {}) {
		super();
		native("xs_deflate").call(this, options);
		this.onData = onData;
		this.onEnd = onEnd;
	}
	close() { return native("xs_deflate_close").call(this); }

	push(buffer, end) { return native("xs_deflate_push").call(this, buffer, end); }
}

function onData(chunk) {
	this.chunks.push(chunk);
}

function onEnd(err) {
	this.err = err;
	if (err) {
		delete this.chunks;
		return;
	}

	// join chunks
	let total = 0;
	for (let i = 0, chunks = this.chunks, length = chunks.length; i < length; i++)
		total += chunks[i].byteLength;

	this.result = new Uint8Array(total);
	for (let i = 0, offset = 0, chunks = this.chunks, length = chunks.length; i < length; i++) {
		this.result.set(new Uint8Array(this.chunks[i]), offset);
		offset += chunks[i].byteLength;
	}
	delete this.chunks;
}

export default Deflate;
