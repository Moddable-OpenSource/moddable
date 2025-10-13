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
 
class Status {
	isFile() { return native("xs_stat_isFile").call(this); }
	isDirectory() { return native("xs_stat_isDirectory").call(this); }
	isSymbolicLink() { return native("xs_stat_isSymbolicLink").call(this); }
}

class File extends Native("xs_fileposix_destructor"){
	constructor(options) { super(); native("xs_fileposix").call(this, options); }
	close() { return native("xs_fileposix_close").call(this); }

	read(buffer /* or count */, posiiton) { return native("xs_fileposix_read").call(this, buffer /* or count */, posiiton); }
	write(buffer, position) { return native("xs_fileposix_write").call(this, buffer, position); }

	status() {
		return native("xs_fileposix_status").call(this, new Status);
	}

	setSize(length) { return native("xs_fileposix_setSize").call(this, length); }

	flush() { return native("xs_fileposix_flush").call(this); }
}

class DirectoryIterator extends Native("xs_directory_iterator_posix_destructor") {
	constructor(directory, path) { super(); native("xs_directory_iterator_posix").call(this, directory, path); }
	next() { return native("xs_directory_iterator_posix_next").call(this); }
	return() { return native("xs_directory_iterator_posix_return").call(this); }
}
Object.setPrototypeOf(DirectoryIterator.prototype, Iterator.prototype);

class Directory extends Native("xs_directoryposix_destructor") {
	constructor(options) { super(); native("xs_directoryposix").call(this, options); }
	close() { return native("xs_directoryposix_close").call(this); }

	openFile(options) {
		return native("xs_directoryposix_openFile").call(this, options, File.prototype);
	}
	openDirectory(options) {
		return native("xs_directoryposix_openDirectory").call(this, options, Directory.prototype);
	}

	delete(path) { return native("xs_directoryposix_delete").call(this, path); }

	move(from, to) { return native("xs_directoryposix_move").call(this, from, to); }

	status(path, options) {
		return native("xs_directoryposix_status").call(this, path, options, new Status);
	}

	createDirectory(options) { return native("xs_directoryposix_createDirectory").call(this, options); }
	createLink(path, target) { return native("xs_directoryposix_createLink").call(this, path, target); }

	readLink(path) { return native("xs_directoryposix_readLink").call(this, path); }

	scan(...path) {
		return new DirectoryIterator(this, ...path);
	}
}
Directory.prototype[Symbol.iterator] = Directory.prototype.scan;

export {Directory}
