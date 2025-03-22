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
	isFile() @ "xs_stat_isFile"
	isDirectory() @ "xs_stat_isDirectory"
	isSymbolicLink() @ "xs_stat_isSymbolicLink"
}

function fstatus() @ "xs_fileposix_status"

class File @ "xs_fileposix_destructor"{
	constructor(options) @ "xs_fileposix"
	close() @ "xs_fileposix_close"

	read(buffer /* or count */, posiiton) @ "xs_fileposix_read"
	write(buffer, position) @ "xs_fileposix_write"

	status() {
		return fstatus.call(this, new Status);
	}

	setSize(length) @ "xs_fileposix_setSize"

	flush() @ "xs_fileposix_flush"
}


class DirectoryIterator @ "xs_directory_iterator_posix_destructor" {
	constructor(directory, path) @ "xs_directory_iterator_posix"
	next() @ "xs_directory_iterator_posix_next"
	return() @ "xs_directory_iterator_posix_return"
}
Object.setPrototypeOf(DirectoryIterator.prototype, Iterator.prototype);

function openFile(options) @ "xs_directoryposix_openFile"
function openDirectory(options) @ "xs_directoryposix_openDirectory"
function status(path) @ "xs_directoryposix_status"

class Directory @ "xs_directoryposix_destructor" {
	constructor(options) @ "xs_directoryposix"
	close() @ "xs_directoryposix_close"

	openFile(options) {
		return openFile.call(this, options, File.prototype);
	}
	openDirectory(options) {
		return openDirectory.call(this, options, Directory.prototype);
	}

	delete(path) @ "xs_directoryposix_delete"

	move(from, to) @ "xs_directoryposix_move"

	status(path, options) {
		return status.call(this, path, options, new Status);
	}

	createDirectory(options) @ "xs_directoryposix_createDirectory"
	createLink(path, target) @ "xs_directoryposix_createLink"

	readLink(path) @ "xs_directoryposix_readLink"

	scan(...path) {
		return new DirectoryIterator(this, ...path);
	}
}
Directory.prototype[Symbol.iterator] = Directory.prototype.scan;

export {Directory}
