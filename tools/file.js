/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import * as FS from "fs";

export default class File {
	constructor(path, mode) {
		this.fd = FS.openSync(path, mode ? "wb+" : "rb");
	}
	close() {
		if (this.fd) {
			FS.closeSync(this.fd);
			this.fd = null;
		}
	}
	write() {
		let c = arguments.length; 
		for (let i = 0; i < c; i++)
			this.writeAux(arguments[i]);
	}
	writeAux(it) {
		if (it instanceof Array)
			it.forEach(item => this.writeAux(item));
		else if (it instanceof ArrayBuffer)
			FS.writeBufferSync(this.fd, it);
		else if (typeof(it) == "number")
			FS.writeByteSync(this.fd, it);
		else
			FS.writeSync(this.fd, it);
	}
};
