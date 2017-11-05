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

import { FILE } from "tool";

// for commodetto

export default class extends FILE {
	constructor(path, mode) {
		super(path, mode ? "wb+" : "rb");
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
			this.writeBuffer(it);
		else if (typeof(it) == "number")
			this.writeByte(it);
		else
			this.writeString(it);
	}
}
