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

const CLI = {
	handlers: [],

	install(item) {
		this.handlers = this.handlers.concat(item);
	},

	execute(incoming) {
		if (!incoming)
			return;

		try {
			let parts = CLI.split(incoming);
			if (!parts) return;

			let command = parts.shift().toLowerCase();
			if ("help" !== command) {
				let found;
				for (let i = CLI.handlers.length - 1; !found && (i >= 0); i--)
					found = CLI.handlers[i].call(this, command, parts);
				if (!found)
					this.line(`Unknown command: ${command}`);
			}
			else
				CLI.distribute.call(this, command, parts);
		}
		catch (e) {
			this.line(e.toString());
		}
	},

	distribute(command, parts) {
		CLI.handlers.forEach(handler => handler.call(this, command, parts));
	},

	split(line) {
		let parts = [];
		line = line.trim();

		while (line) {
			let c = line.charAt(0);
			let end = " ", start = 0, part;
			if (('"' === c) || ("'" === c)) {
				end = c;
				start += 1;
			}

			let index = line.indexOf(end, 1);
			if (index > 0) {
				part = line.substring(start, index);
				line = line.substring(index + 1).trim();
			}
			else {
				part = line.substring(start);
				line = undefined;
			}
			parts.push(part);
		}

		return parts;
	},
};

CLI.install(function (command, parts) {
	switch (command) {
		case "welcome":
			this.line("Moddable Command Line Interface");
			break;
		case "prompt":
			this.write("> ");
			break;
		case "date":
			this.line((new Date()).toString());
			break;
		case "version":
			this.line("0.1");
			break;
		case "help":
			this.line("date - display date and time");
			this.line("version - display CLI version number");
			break;
		default:
			return false;
	}
	return true;
});

export default CLI;
