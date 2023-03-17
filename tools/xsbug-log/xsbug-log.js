/*
 * Copyright (c) 2022-2023  Moddable Tech, Inc.
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

let net, exec, machine;

try {
	net = require('node:net');
	exec = require('node:child_process').exec;
	Machine = require('./xsbug-machine.js').Machine;
}
catch (e) {
	if ("MODULE_NOT_FOUND" === e.code) {
		console.log("xsbug-log: missing modules! Did you npm install?");
		console.log("   cd $MODDABLE/tools/xsbug-log");
		console.log("   npm install");
	}
	else
		console.log("xsbug-log start-up error: " + e);
	process.exit();
}

class LogMachine extends Machine {
	view = {};
	log = "";

	onTitleChanged(title, tag) {
		super.onTitleChanged(title, tag);
		
		if (title && (title !== "mcsim"))
			console.log(`# Connected to "${title}"`);

		this.doSetAllBreakpoint([], false, true);		// break on exceptions
	}
	onLogged(path, line, data) {
		this.log += data;
		if (this.log.endsWith("\n")) {
			console.log(this.log.slice(0, this.log.length - 1));
			this.log = "";
		}
	}
	onBroken(path, line, text) {
		this.view.frames.forEach((frame, index) => {
			let line = "  #" + index + ": " + frame.name;
			if (frame.path)
				line += " " + frame.path + ":" + frame.line
			console.log(line);
		});

		super.onBroken(path, line, text);
	}
	onViewChanged(name, items) {
		this.view[name] = items;
	}
}

const portIn = process.env.XSBUG_PORT || 5002;
let connections = 0;

const server = net.createServer(target => { 
	connections++;
	target.setEncoding("utf8");
	target.on('end', () => {
		if (target.machine.title && ("mcsim" !== target.machine.title))
			console.log(`# Disconnected from "${target.machine.title}"`);
		if (0 === --connections)
			process.exit(0);
	});

	target.machine = new LogMachine(target, target);
});

server.listen(portIn, () => { 
   console.log(`# xsbug-log listening on port ${portIn}. ^C to exit.`);
});

let command = process.argv[2];
for (let i = 3; i < process.argv.length; i++) 
	command += ` ${process.argv[i]}`;

exec(command);
