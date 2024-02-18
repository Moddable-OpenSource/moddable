import {TOOL, FILE} from "tool";

export default class extends TOOL {
	constructor(argv) {
		super(argv);
	}
	toHex(value) {
		let result = value.toString(16);
		result = "0".repeat(6 - result.length) + result;
		return result;
	}
	reportRanges(name, ranges) {
		let c = ranges.length;
		this.report(`#define mxCharSet_${name} ${c}`);
		if (c) {
			let string = `static const txInteger ICACHE_RODATA_ATTR gxCharSet_${name}[mxCharSet_${name}] = {`
			for (let i = 0; i < c; i++) {
				if (i % 16 == 0) {
					this.report(string);
					string = "\t";
				}
				let range = ranges[i];
				string += `0x${this.toHex(range)}, `;
			}
			this.report(string);
			this.report("};");
		}
		else
			this.report(`#define gxCharSet_${name} C_NULL`);
	}
	reportNames(names) {
		let c = names.length;
		this.report(`#define mxCharSet_String_Property ${c}`);
		this.report(`static const txCharSetUnicodeStringProperty ICACHE_RODATA_ATTR gxCharSet_String_Property[mxCharSet_String_Property] = {`);
		for (let i = 0; i < c; i++) {
			let name = names[i];
			this.report(`\t{ "${name}", mxCharSet_${name}, gxCharSet_${name}, mxStrings_${name}, gxStrings_${name} },`);
		}
		this.report("};");
	}
	reportStrings(name, strings) {
		let c = strings.length;
		this.report(`#define mxStrings_${name} ${c}`);
		this.report(`static const txString ICACHE_RODATA_ATTR gxStrings_${name}[mxStrings_${name}] = {`);
		let buffer = "\t";
		for (let i = 0; i < c; i++) {
			let string = '"';
			string += encodeURI(strings[i].bytes).replaceAll("%", "\\x");
			string += '",';
			if ((buffer.length + string.length) > 160) {
				this.report(buffer);
				buffer = "\t";
			}
			buffer += string;
		}
		this.report(buffer);
		this.report("};");
	}
	buildRanges(singles) {
		let ranges = [];
		let length = singles.length;
		if (length > 0) {
			let former = singles[0];
			ranges.push(former);
			for (let i = 1; i < length; i++) {
				let single = singles[i];
				former++;
				if (single != former) {
					ranges.push(former);
					ranges.push(single);
				}
				former = single;
			}
			ranges.push(former + 1);
		}
		return ranges;
	}
	run() {
		let once = true;
		let tool = this;
		let singles;
		let multiples;
		let ids = [];
		
		function testPropertyOfStrings(param) {
			for (let string of param.matchStrings) {
				const codePoints = [ ...string ].map((cp) => cp.codePointAt(0));
				if (codePoints.length == 1) {
					singles.push(codePoints[0]);
				}
				else {
					multiples.push({ length:codePoints.length, bytes:string });
				}
			}
		}
		const directory = this.resolveDirectoryPath("../../../../test262/test/built-ins/RegExp/property-escapes/generated/strings");
		const names = this.enumerateDirectory(directory);
		for (let name of names) {
			if (name.indexOf("negative") >= 0)
				continue;
			const path = this.joinPath({directory, name});
			if (this.isDirectoryOrFile(path) != 1)
				continue;
			let buffer = this.readFileString(path);
			buffer = buffer.replaceAll("/^\\p", "'");
			buffer = buffer.replaceAll("$/v", "'");
			try {
				once = true;
				singles = [];
				multiples = [];
				eval(buffer);
				singles.sort((a, b) => {
					return a - b;
				});
				multiples.sort((a, b) => {
					if (a.length > b.length) return -1;
					if (a.length < b.length) return 1;
					return b.bytes.localeCompare(a.bytes);
				});
				let ranges = this.buildRanges(singles);
				name = name.slice(0, -3);
				ids.push(name);
				this.reportRanges(name, ranges);
				this.reportStrings(name, multiples);
				ids.sort();
			}
			catch(e) {
			}
		}
		this.reportNames(ids);
	}
}
