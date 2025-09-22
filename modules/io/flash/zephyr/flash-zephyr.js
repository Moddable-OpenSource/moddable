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

class Flash @ "xs_flashstorage_destructor" {
	constructor() {throw new TypeError}
	close() @ "xs_flashstorage_close"

	eraseBlock(start, end) @ "xs_flashstorage_eraseBlock"

	read(byteLength, byteOffset) @ "xs_flashstorage_read"
	write(buffer, byteOffset) @ "xs_flashstorage_write"

	status() @ "xs_flashstorage_status"

	get format() {
		return "buffer";
	}
	set format(value) {
		if (value != "buffer")
			throw new RangeError("invalid");
	}
}

function open(options, constructor) @ "xs_flashstorage_open"

class FlashPartitionIterator @ "xs_flashIterator_destructor" {
	constructor() @ "xs_flashIterator_"
	next() @ "xs_flashIterator_next"
	return() @ "xs_flashIterator_return"
}

export default Object.freeze({
	open(options) {
		return open(options, Flash.prototype);		
	},
	[Symbol.iterator]() {
		return new FlashPartitionIterator;
	}
}, true);
