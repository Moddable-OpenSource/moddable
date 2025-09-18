/*
 * Copyright (c) 2024-2025  Moddable Tech, Inc.
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

class Storage @ "xs_directorystorage_destructor" {
	constructor() {throw new TypeError}
	close() @ "xs_directorystorage_close"

	delete(key) @ "xs_directorystorage_delete"
	read(key) @  "xs_directorystorage_read"
	write(key, value) @  "xs_directorystorage_write"
	[Symbol.iterator]() {
		return new StorageIterator(this);
	}

	get format() @ "xs_directorystorage_format_get"
	set format() @ "xs_directorystorage_format_set"
}

function open(options, prototype) @ "xs_directorystorage_open"

class StorageIterator @ "xs_storageIterator_destructor" {
	constructor(storage) @ "xs_storageIterator_constructor"
	next() @ "xs_storageIterator_next"
	return() @ "xs_storageIterator_return"
}

export default Object.freeze({
	open(options) {
		return open(options, Storage.prototype);
	}
}, true);
