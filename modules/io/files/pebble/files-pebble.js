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
	isFile() { return (this.mode & 1) ? true : false; }
	isDirectory() { return (this.mode & 2) ? true : false; }
	isSymbolicLink() { return false; }
}

class File extends Native("xs_filepfs_destructor"){
	constructor(options) { super(); native("xs_filepfs").call(this, options); }
	close() { return native("xs_filepfs_close").call(this); }

	read(buffer /* or count */, posiiton) { return native("xs_filepfs_read").call(this, buffer /* or count */, posiiton); }
	write(buffer, position) { return native("xs_filepfs_write").call(this, buffer, position); }

	status() {
		return native("xs_filepfs_status").call(this, new Status);
	}

	flush() {};
}
File.prototype.setSize = unimplemented;

class Directory extends Native("xs_directorypfs_destructor") {
	constructor(options) { super(); native("xs_directorypfs").call(this, options); }
	close() { return native("xs_directorypfs_close").call(this); }

	openFile(options) {
		return native("xs_directorypfs_openFile").call(this, options, File.prototype);
	}
	openDirectory(options) {
		return native("xs_directorypfs_openDirectory").call(this, options, Directory.prototype);
	}

	delete(path) { return native("xs_directorypfs_delete").call(this, path); }

	status(path, options) {
		return native("xs_directorypfs_status").call(this, path, options, new Status);
	}

	createDirectory(options) { return native("xs_directorypfs_createDirectory").call(this, options); }

	scan(...path) {
		return (native("xs_directorypfs_scan").call(this, ...path))[Symbol.iterator]();
	}
}
Directory.prototype[Symbol.iterator] = Directory.prototype.scan;
Directory.prototype.createLink = unimplemented;
Directory.prototype.readLink = unimplemented;
Directory.prototype.move = unimplemented;

function unimplemented() {
	throw new Error("unsupported by PebbleOS");
}

export {Directory}
