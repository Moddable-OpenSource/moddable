/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

class Flash extends Native("xs_flashstorage_destructor") {
	constructor() {throw new TypeError}
	close() { return native("xs_flashstorage_close").call(this); }

	eraseBlock(start, end) { return native("xs_flashstorage_eraseBlock").call(this, start, end); }

	read(byteLength, byteOffset) { return native("xs_flashstorage_read").call(this, byteLength, byteOffset); }
	write(buffer, byteOffset) { return native("xs_flashstorage_write").call(this, buffer, byteOffset); }

	status() { return native("xs_flashstorage_status").call(this); }

	get format() {
		return "buffer";
	}
	set format(value) {
		if (value != "buffer")
			throw new RangeError("invalid");
	}
}

function open(options, constructor) { return native("xs_flashstorage_open").call(this, options, constructor); }

class FlashPartitionIterator extends Native("xs_flashIterator_destructor") {
	constructor() { super(); native("xs_flashIterator_").call(this); }
	next() { return native("xs_flashIterator_next").call(this); }
	return() { return native("xs_flashIterator_return").call(this); }
}

export default Object.freeze({
	open(options) {
		return open(options, Flash.prototype);		
	},
	[Symbol.iterator]() {
		return new FlashPartitionIterator;
	}
}, true);
