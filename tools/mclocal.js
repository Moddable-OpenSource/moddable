/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

import { FILE, TOOL } from "tool";

export default class extends TOOL {
	constructor(argv) {
		super(argv);
		this.debug = false;
		this.inputPaths = [];
		this.name = "locals";
		this.outputPath = null;
		this.slash = (this.currentPlatform == "win") ? "\\" : "/";
		this.characters = null;
		var argc = argv.length;
		var name, path;
		for (let argi = 1; argi < argc; argi++) {
			let option = argv[argi], name, path;
			switch (option) {
			case "-d":
				this.debug = true;
				break;
			case "-o":
				argi++;	
				if (argi >= argc)
					throw new Error("-o: no output directory!");
				name = argv[argi];
				if (this.outputDirectory)
					throw new Error("-o '" + name + "': too many output directories!");
				path = this.resolveDirectoryPath(name);
				if (!path)
					throw new Error("-o '" + name + "': output directory not found!");
				this.outputPath = path;
				break;
			case "-r":
				argi++;	
				if (argi >= argc)
					throw new Error("-r: no name!");
				this.name = argv[argi];
				break;
			case "-s":
				this.characters = [];
				break;
			default:
				name = argv[argi];
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("'" + name + "': input file not found!");
				this.inputPaths.push(path);
				break;
			}
		}
		if (this.inputPaths.length == 0)
			throw new Error("no input files!");
		if (!this.outputPath)
			this.outputPath = this.currentDirectory;
	}
	build(length, keys, values, G, H) {
		// adapted from https://github.com/mixu/perfect
		var buckets = [];
		var modulo, bucket;
		keys.forEach(key => {
			modulo = this.hash(0, key) % length;
			bucket = buckets[modulo];
			if (bucket) 
				bucket.push(key);
			else
				bucket = [ key ];
			buckets[modulo] = bucket;
		});
		buckets.sort((a, b) => b ? (a ? b.length - a.length : b.length) : (a ? 0 - a.length : 0));
		for (modulo = 0; modulo < length; modulo++) {
			bucket = buckets[modulo];
			if (!bucket || (bucket.length <= 1))
				break;
			var c = bucket.length, i = 0;
			var d = 1, slots = [], slot, used = {};
			while (i < c) {
				slot = this.hash(d, bucket[i]) % length;
				if ((H[slot] !== undefined) || used[slot]) {
					d++;
					i = 0;
					slots = [];
					used = {};
				} 
				else {
					i++;
					slots.push(slot);
					used[slot] = true;
				}
			}
			G[this.hash(0, bucket[0]) % length] = d;
			bucket.forEach((key, index) => {
				H[slots[index]] = values[key];
			});
		}
		var slots = [];
		H.forEach((value, index) => {
			if (value === undefined)
				slots.push(index);
		});
		for(; modulo < length; modulo++ ) {
			bucket = buckets[modulo];
			if (!bucket || bucket.length == 0)
				break;
			var key = bucket[0];
			var slot = slots.pop();
			G[this.hash(0, key) % length] = 0 - slot - 1;
			H[slot] = values[key];
		}
	}
	run() {
		var locals = this.inputPaths.map(path => {
			var parts = this.splitPath(path);
			return {
				name: parts.name,
				dictionary: JSON.parse(this.readFileString(path)),
			};
		});
		var all = {}
		locals.forEach(local => {
			for (var key in local.dictionary)
				all[key] = true;
		});
		var keys = Object.keys(all);
		var length = keys.length;
		if (length < 127)
			length = 127;
		var values = {};
		var debug = new Array(length).fill();
		keys.forEach((key, index) => {
			values[key] = index;
			debug[index] = key;
		})
		locals.forEach(local => {
			var dictionary = local.dictionary;
			var table = local.table = new Array(length).fill();
			keys.forEach((key, index) => {
				if (key in dictionary)
					table[index] = dictionary[key];
				else {
					this.reportWarning(null, 0, "'" + local.name + "': missing '" + key + "'!");
					table[index] = "";
				}
			});
		});
		var G = new Array(length).fill(0);
		var H = new Array(length).fill();
		this.build(length, keys, values, G, H);
		
		var parts = { directory:this.outputPath, name:this.name, extension:".mhi" };
		var file = new FILE(this.joinPath(parts), "wb");
		this.write32(file, length);
		for (var index of G)
			this.write32(file, index);
		if (this.debug) {
			this.write32(file, length);
			var offset = (1 + length) << 3;
			for (var index of H) {
				this.write32(file, offset);
				var string = debug[index];
				if (string)
					offset += this.strlen(string) + 1;
			}
			for (var index of H) {
				var string = debug[index];
				if (string) {
					file.writeString(string);
					file.writeByte(0);
				}
			}
		}	
		file.close();
		
		var characters = this.characters;
		var parts = { directory:this.outputPath, extension:".mhr" };
		locals.forEach(local => {
			var table = local.table;
			parts.name = this.name + "." + local.name;
			var file = new FILE(this.joinPath(parts), "wb");
			this.write32(file, length);
			var offset = (1 + length) << 2;
			for (var index of H) {
				this.write32(file, offset);
				var string = table[index];
				if (string)
					offset += this.strlen(string) + 1;
			}
			for (var index of H) {
				var string = table[index];
				if (string) {
					if (characters) {
						let c = string.length;
						for (let i = 0; i < c; i++) {
							characters[string.charCodeAt(i)] = string.charAt(i);
						}
					}
					file.writeString(string);
					file.writeByte(0);
				}
			}
			file.close();
		});
		if (characters) {
			var string = characters.join("");
			var parts = { directory:this.outputPath, name:this.name, extension:".txt" };
			var file = new FILE(this.joinPath(parts), "wb");
			file.writeString(string);
			file.close();
		}
	}
	write32(file, value) {
		file.writeByte(value & 0xff);
		file.writeByte((value >> 8) & 0xff);
		file.writeByte((value >> 16) & 0xff);
		file.writeByte((value >> 24) & 0xff);
	}
}
