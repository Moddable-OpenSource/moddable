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
	isSymbolicLink() {return false;}
}

function fstatus() @ "xs_filezephyr_status"

class File @ "xs_filezephyr_destructor"{
	constructor(options) @ "xs_filezephyr"
	close() @ "xs_filezephyr_close"

	read(buffer /* or count */, posiiton) @ "xs_filezephyr_read"
	write(buffer, position) @ "xs_filezephyr_write"

	status() {
		return fstatus.call(this, new Status);
	}

	setSize(length) @ "xs_filezephyr_setSize"

	flush() @ "xs_filezephyr_flush"
}


class DirectoryIterator @ "xs_directory_iterator_zephyr_destructor" {
	constructor(directory, path) @ "xs_directory_iterator_zephyr"
	next() @ "xs_directory_iterator_zephyr_next"
	return() @ "xs_directory_iterator_zephyr_return"
}
Object.setPrototypeOf(DirectoryIterator.prototype, Iterator.prototype);

function openFile(options) @ "xs_directoryzephyr_openFile"
function openDirectory(options) @ "xs_directoryzephyr_openDirectory"
function status(path) @ "xs_directoryzephyr_status"

class Directory @ "xs_directoryzephyr_destructor" {
	constructor(options) @ "xs_directoryzephyr"
	close() @ "xs_directoryzephyr_close"

	openFile(options) {
		return openFile.call(this, options, File.prototype);
	}
	openDirectory(options) {
		return openDirectory.call(this, options, Directory.prototype);
	}

	delete(path) @ "xs_directoryzephyr_delete"

	move(from, to) @ "xs_directoryzephyr_move"

	status(path, options) {
		return status.call(this, path, options, new Status);
	}

	createDirectory(options) @ "xs_directoryzephyr_createDirectory"
	createLink(path, target) {throw new Error("unsupported");}


	readLink(path) {throw new Error("unsupported");}

	scan(...path) {
		return new DirectoryIterator(this, ...path);
	}
}
Directory.prototype[Symbol.iterator] = Directory.prototype.scan;

export {Directory}
