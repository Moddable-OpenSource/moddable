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
	zip
*/

export class ZIP @ "xs_zip_destructor" {
	constructor(buffer) @ "xs_zip";

	file(path) {
		return new File(this, path);
	}
	map(path) @ "xs_zip_file_map";
	iterate(path) {
		return new Iterator(this, path);
	}
};

class File @ "xs_zip_file_destructor" {
	constructor(zip, path) @ "xs_zip_File";
	read(type, count) @ "xs_zip_file_read";

	close() @ "xs_zip_file_close";

	get length() @ "xs_zip_file_get_length";
	get position() @ "xs_zip_file_get_position";
	set position() @ "xs_zip_file_set_position";
	get method() @ "xs_zip_file_get_method";
	get crc() @ "xs_zip_file_get_crc";
};

class Iterator @ "xs_zip_file_iterator_destructor" {
	constructor(zip, path) @ "xs_zip_file_Iterator";
	next() @ "xs_zip_file_iterator_next";
}

export default Object.freeze({
	ZIP
});
