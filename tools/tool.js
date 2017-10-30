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

const inline = "args.txt";

export default class Tool {
	constructor(argv) {
		this.errorCount = 0;
		this.warningCount = 0;
		if ((argv.length == 2) && (argv[1] == inline)) {
			let path = this.resolveFilePath(inline);
			let args = FS.readFileSync(path);
			args = args.trim().split(/\s+/);
			argv.length = 1;
			args.forEach(arg => argv.push(arg));
		}
	}
	get ipAddress() @ "Tool_prototype_get_ipAddress";
	get currentDirectory() @ "Tool_prototype_get_currentDirectory";
	set currentDirectory(it) @ "Tool_prototype_set_currentDirectory";
	get currentPlatform() @ "Tool_prototype_get_currentPlatform";
	execute(command) @ "Tool_prototype_execute";
	getenv(name) @ "Tool_prototype_getenv";
	hash(d, string) @  "Tool_prototype_fsvhash";
	joinPath(parts) @ "Tool_prototype_joinPath";
	report(message) @ "Tool_prototype_report";
	reportError(message) @ "Tool_prototype_reportError";
	reportWarning(message) @ "Tool_prototype_reportWarning";
	resolveDirectoryPath(message) @ "Tool_prototype_resolveDirectoryPath";
	resolveFilePath(message) @ "Tool_prototype_resolveFilePath";
	resolvePath(message) @ "Tool_prototype_resolvePath";
	splitPath(path) @ "Tool_prototype_splitPath";
	strlen(string) @  "Tool_prototype_strlen";
	then() @  "Tool_prototype_then";
}
