/*
 * Copyright (c) 2016-2023 Moddable Tech, Inc.
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

const inline = "args.txt";

export class FILE @ "FILE_prototype_destructor" {
	constructor(path, flags) @ "FILE_prototype_constructor";
	close() @ "FILE_prototype_close";
	dump(path) @ "FILE_prototype_dump";
	line(...strings) {
		let c = arguments.length; 
		for (let i = 0; i < c; i++)
			this.writeString(arguments[i]);
		this.writeString("\n");
	}
	write() @ "FILE_prototype_write";
	writeBuffer(buffer) @ "FILE_prototype_writeBuffer";
	writeByte(byte) @ "FILE_prototype_writeByte";
	writeString(string) @ "FILE_prototype_writeString";
}

export class TOOL {
	constructor(argv) {
		let command = argv.slice();

		this.errorCount = 0;
		this.warningCount = 0;
		if ((argv.length == 2) && (argv[1] == inline)) {
			let path = this.resolveFilePath(inline);
			let args = this.readFileString(path);
			args = args.trim().split(/\s+/);
			argv.length = 1;
			args.forEach(arg => argv.push(arg));
		}
		
		let path = this.getenv("MODDABLE") + "/tools/VERSION";
		if ("win" === this.currentPlatform)
			path = path.replaceAll("/", "\\");
		const fileVersion = (this.isDirectoryOrFile(path) == 1) ? this.readFileString(path) : undefined;
		const toolsVersion = this.getToolsVersion();
		if (fileVersion === toolsVersion)
			return;

		trace(`Moddable SDK tools mismatch between binary (${toolsVersion}) and source (${fileVersion})! Rebuilding tools.\n`);

		command = command.map(item => {
			item = item.replaceAll('"', '\\"');
			item = (item.indexOf(" ") < 0) ? item : `"${item}"`;
			return item;
		});
		command = command.join(" ");
		if ("win" === this.currentPlatform) {
			command = `wmic process where "name='tools.exe'" delete >nul 2>&1 & pushd %MODDABLE%\\build\\makefiles\\win && build clean && build && popd && ${command}`;
			this.then("cmd", "/C", command);
		}
		else {
			command = `pushd $MODDABLE/build/makefiles/${this.currentPlatform} && make clean && make && popd && ${command}`;
			this.then("bash", "-c", command);
		}
		this.run = function() {};
	}
	get ipAddress() @ "Tool_prototype_get_ipAddress";
	get currentDirectory() @ "Tool_prototype_get_currentDirectory";
	set currentDirectory(it) @ "Tool_prototype_set_currentDirectory";
	get currentPlatform() @ "Tool_prototype_get_currentPlatform";
	deleteDirectory(path) @ "Tool_prototype_deleteDirectory";
	deleteFile(path) @ "Tool_prototype_deleteFile";
	createDirectory(path) @ "Tool_prototype_createDirectory";
	enumerateDirectory(path) @ "Tool_prototype_enumerateDirectory";
	execute(command) @ "Tool_prototype_execute";
	getenv(name) @ "Tool_prototype_getenv";
	getFileSize(path) @ "Tool_prototype_getFileSize";
	getToolsVersion(path) @ "Tool_prototype_getToolsVersion";
	hash(d, string) @  "Tool_prototype_fsvhash";
	isDirectoryOrFile(path) @ "Tool_prototype_isDirectoryOrFile"; // -1: directory, 0: none, 1: file
	joinPath(parts) @ "Tool_prototype_joinPath";
	readFileString(path) @ "Tool_prototype_readFileString";
	readFileBuffer(path) @ "Tool_prototype_readFileBuffer";
	report(message) @ "Tool_prototype_report";
	reportError(message) @ "Tool_prototype_reportError";
	reportWarning(message) @ "Tool_prototype_reportWarning";
	resolveDirectoryPath(message) @ "Tool_prototype_resolveDirectoryPath";
	resolveFilePath(message) @ "Tool_prototype_resolveFilePath";
	resolvePath(message) @ "Tool_prototype_resolvePath";
	setenv(name, value) @ "Tool_prototype_setenv";
	spawn(path) @ "Tool_prototype_spawn";
	splitPath(path) @ "Tool_prototype_splitPath";
	strlen(string) @  "Tool_prototype_strlen";
	then() @  "Tool_prototype_then";
	writeFileString(path, string) @ "Tool_prototype_writeFileString";
	writeFileBuffer(path, buffer) @ "Tool_prototype_writeFileBuffer";
}
