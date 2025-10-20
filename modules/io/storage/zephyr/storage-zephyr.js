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

class Storage extends Native("xs_directorystorage_destructor") {
	constructor() {throw new TypeError}
	close() { return native("xs_directorystorage_close").call(this); }

	delete(key) { return native("xs_directorystorage_delete").call(this, key); }
	read(key) { return native("xs_directorystorage_read").call(this, key); }
	write(key, value) { return native("xs_directorystorage_write").call(this, key, value); }
	[Symbol.iterator]() {
		return new StorageIterator(this);
	}

	get format() { return native("xs_directorystorage_format_get").call(this); }
	set format(value) { native("xs_directorystorage_format_set").call(this, value); }
}

function open(options, prototype) { return native("xs_directorystorage_open").call(this, options, prototype); }

class StorageIterator extends Native("xs_storageIterator_destructor") {
	constructor(storage) { super(); native("xs_storageIterator_constructor").call(this, storage); }
	next() { return native("xs_storageIterator_next").call(this); }
	return() { return native("xs_storageIterator_return").call(this); }
}

export default Object.freeze({
	open(options) {
		return open(options, Storage.prototype);
	}
}, true);
