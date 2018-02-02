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


import CLI from "cli";
import {File, Iterator, System} from "file";

CLI.install(function(command, parts) {
	switch (command) {
		case "ls":
			let item, iterator = new Iterator(parts[0] ? parts[0] : "/");
			while (item = iterator.next())
				this.line(item.name.padEnd(32), (undefined === item.length) ? "-" : item.length.toString());
			break;

		case "rm":
			if (parts.length < 1)
				throw new Error("path missing");
			File.delete(parts[0]);
			break;

		case "more": {
			if (parts.length < 1)
				throw new Error("path missing");
			let file = new File(parts[0]);
			let length = file.length;
			while (length) {
				let size = (length > 256) ? 256 : length;
				this.write(file.read(String, size));
				length -= size;
			}
			this.line();
			file.close();
			}
			break;

		case "hexdump": {
			if (parts.length < 1)
				throw new Error("path missing");
			let file = new File(parts[0]);
			let length = file.length;
			while (length) {
				let size = (length > 16) ? 16 : length;
				let string = file.position.toString(16).padStart(7, "0");
				let data = new Uint8Array(file.read(ArrayBuffer, size));
				for (let i = 0; i < size; i++)
					string += " " + data[i].toString(16).padStart(2, "0");
				this.line(string);
				length -= size;
			}
			file.close();
			}
			break;

		case "mv":
			if (parts.length < 2)
				throw new Error("path missing");
			File.rename(parts[0], parts[1]);
			break;

		case "df":
			let info = System.info();
			this.line("Filesystem     Used           Available      Capacity");
			this.line("/".padEnd(15), (Math.ceil(info.used / 1024) + " KB").padEnd(15),
						  (Math.floor((info.total - info.used) / 1024) + " KB").padEnd(15), (Math.round(info.total / 1024) + " KB").padEnd(15));
			break;

		case "help":
			this.line("ls [path] - list contents of path");
			this.line("rm name - delete file");
			this.line("mv old new - rename file");
			this.line("more name - show file content as text");
			this.line("hexdump name - show file content in hex");
			this.line("df - file system info");
			break;

		default:
			return false;
	}
	return true;
});
