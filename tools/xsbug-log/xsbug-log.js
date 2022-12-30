/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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

const net = require('node:net');
const { exec } = require('node:child_process');
const { Machine } = require('./xsbug-machine.js');

const portIn = process.env.XSBUG_PORT || 5002;
let log = "";
let connections = 0;
const view = {};

const server = net.createServer(target => { 
	connections++;
	target.setEncoding("utf8");
	target.on('end', () => {
		if (0 === --connections)
			process.exit(0);
	});

	const machine = new Machine(target, target);
	machine.doSetAllBreakpoint([], false, true);		// break on exceptions
	machine.onLogged = function(path, line, data) {
		log += data;
		if (log.endsWith("\n")) {
			console.log(log.slice(0, log.length - 1));
			log = "";
		}
	};
	machine.onBroken = function(path, line, text) {
		view.frames.forEach((frame, index) => {
			let line = "  #" + index + ": " + frame.name;
			if (frame.path)
				line += " " + frame.path + ":" + frame.line
			console.log(line);
		});

		this.doGo();
	};
	machine.onViewChanged = function(name, items) {
		view[name] = items;
	};
});

server.listen(portIn, () => { 
   console.log(`xsbug-log listening on port ${portIn}. ^C to exit.`);
});

let command = process.argv[2];
for (let i = 3; i < process.argv.length; i++) 
	command += ` ${process.argv[i]}`;

exec(command);
