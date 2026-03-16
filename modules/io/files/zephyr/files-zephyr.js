/*
 * Copyright (c) 2024-2026  Moddable Tech, Inc.
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
 
class Status {
	isFile() { return native("xs_stat_isFile").call(this); }
	isDirectory() { return native("xs_stat_isDirectory").call(this); }
	isSymbolicLink() {return false;}
}

class File extends Native("xs_filezephyr_destructor"){
	constructor(options) { super(); native("xs_filezephyr").call(this, options); }
	close() { return native("xs_filezephyr_close").call(this); }

	read(buffer /* or count */, posiiton) { return native("xs_filezephyr_read").call(this, buffer /* or count */, posiiton); }
	write(buffer, position) { return native("xs_filezephyr_write").call(this, buffer, position); }

	status() {
		return native("xs_filezephyr_status").call(this, new Status);
	}

	setSize(length) { return native("xs_filezephyr_setSize").call(this, length); }

	flush() { return native("xs_filezephyr_flush").call(this); }
}

class DirectoryIterator extends Native("xs_directory_iterator_zephyr_destructor") {
	constructor(directory, path) { super(); native("xs_directory_iterator_zephyr").call(this, directory, path); }
	next() { return native("xs_directory_iterator_zephyr_next").call(this); }
	return() { return native("xs_directory_iterator_zephyr_return").call(this); }
}
Object.setPrototypeOf(DirectoryIterator.prototype, Iterator.prototype);

class Directory extends Native("xs_directoryzephyr_destructor") {
	constructor(options) { super(); native("xs_directoryzephyr").call(this, options); }
	close() { return native("xs_directoryzephyr_close").call(this); }

	openFile(options) {
		return native("xs_directoryzephyr_openFile").call(this, options, File.prototype);
	}
	openDirectory(options) {
		return native("xs_directoryzephyr_openDirectory").call(this, options, Directory.prototype);
	}

	delete(path) { return native("xs_directoryzephyr_delete").call(this, path); }

	move(from, to) { return native("xs_directoryzephyr_move").call(this, from, to); }

	status(path, options) {
		if (undefined === path)
			throw new Error("path required");
		return native("xs_directoryzephyr_status").call(this, path, options, new Status);
	}

	createDirectory(options) { return native("xs_directoryzephyr_createDirectory").call(this, options); }

	scan(...path) {
		return new DirectoryIterator(this, ...path);
	}
}
// @ts-expect-error assigning scan as iterator
Directory.prototype[Symbol.iterator] = Directory.prototype.scan;

export {Directory}
