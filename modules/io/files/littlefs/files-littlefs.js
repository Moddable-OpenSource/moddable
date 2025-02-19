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

function fstatus() @ "xs_filelittlefs_status"

class File @ "xs_filelittlefs_destructor"{
	constructor(options) @ "xs_filelittlefs"
	close() @ "xs_filelittlefs_close"

	read(buffer /* or count */, posiiton) @ "xs_filelittlefs_read"
	write(buffer, position) @ "xs_filelittlefs_write"

	status() {
		return fstatus.call(this, new Status);
	}

	setSize(length) @ "xs_filelittlefs_setSize"

	flush() @ "xs_filelittlefs_flush"
}

class DirectoryIterator @ "xs_directory_iterator_littlefs_destructor" {
	constructor(directory, path) @ "xs_directory_iterator_littlefs"
	next() @ "xs_directory_iterator_littlefs_next"
	return() @ "xs_directory_iterator_littlefs_return"
}
Object.setPrototypeOf(DirectoryIterator.prototype, Iterator.prototype);

function openFile(options) @ "xs_directorylittlefs_openFile"
function openDirectory(options) @ "xs_directorylittlefs_openDirectory"
function status(path) @ "xs_directorylittlefs_status"

class Directory @ "xs_directorylittlefs_destructor" {
	constructor(options) @ "xs_directorylittlefs"
	close() @ "xs_directorylittlefs_close"

	openFile(options) {
		return openFile.call(this, options, File.prototype);
	}
	openDirectory(options) {
		return openDirectory.call(this, options, Directory.prototype);
	}

	delete(path) @ "xs_directorylittlefs_delete"

	move(from, to) @ "xs_directorylittlefs_move"

	status(path) {
		return status.call(this, path, new Status);
	}

	createDirectory(options) @ "xs_directorylittlefs_createDirectory"
	createLink(path, target) @ "xs_directorylittlefs_link"

	readLink(path) @ "xs_directorylittlefs_link"

	scan(...path) {
		return new DirectoryIterator(this, ...path);
	}
}

export {Directory}
