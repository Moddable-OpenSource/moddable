/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

/*
	file
*/

export class File @ "xs_file_destructor" {
	constructor(dictionary) @ "xs_File";

	read(type, count) @ "xs_file_read";
	write(...items) @ "xs_file_write";

	close() @ "xs_file_close";

	get length() @ "xs_file_get_length";
	get position() @ "xs_file_get_position";
	set position() @ "xs_file_set_position";

	static delete(path) @ "xs_file_delete";
	static exists(path) @ "xs_file_exists";
	static rename(path, name) @ "xs_file_rename";
};

export class Iterator @ "xs_file_iterator_destructor" {
	constructor(path) @ "xs_File_Iterator";
	next() @ "xs_file_iterator_next";

	[Symbol.iterator]() {
		return {
			iterator: this,
			next() {
				const result = {value: this.iterator.next()};
				result.done = undefined === result.value;
				return result;
			}
		};
	}
};

export class Directory {
	static create(path) @ "xs_directory_create";
	static delete(path) @ "xs_directory_delete";
};

export class System {
	static config() @ "xs_file_system_config";
	static info() @ "xs_file_system_info";
};

export default Object.freeze({
	File, Iterator, System, Directory
});
