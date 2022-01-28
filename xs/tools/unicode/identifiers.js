import {TOOL, FILE} from "tool";

export default class extends TOOL {
	constructor(argv) {
		super(argv);
	}
	compress(buffer) {
	  let results = [];
	  let q = 0;
	  let offset = 0;
	  let i = 0;
	  for (;;) {
		while ((q < 0x110000) && (buffer[q] == 0)) {
		  q++;
		}
		if (q == 0x110000)
			break;
		results.push(q - offset);
		offset = q;
		while ((q < 0x110000) && (buffer[q] == 1)) {
		  q++;
		}
		q--;
		results.push(q - offset);
		offset = q;
		q++;
	  }
	  return results;
	}
	parse(string, which) {
		const begin = string.indexOf(which);
		const end = string.indexOf("# ================================================", begin);
		const lines = string.slice(begin, end).split("\n");
		const re = /([0-9A-F]+)(\.\.([0-9A-F]+))?/;
		const buffer = new Int8Array(0x110000);
		buffer["$".codePointAt(0)] = 1;
		buffer["_".codePointAt(0)] = 1;
		for (let line of lines) {
			if (line.length == 0)
				continue;
			if (line[0] == "#")
				continue;
			const r = line.match(re);
			let from = parseInt(r[1], 16);
			buffer[from++] = 1;
			let to = r[3]
			if (to) {
				to = parseInt(to, 16);
				while (from <= to)
					buffer[from++] = 1;
			}
		}
		return buffer;
	}
	print(which, results) {
		let c = results.length;
		let string = `\t#define mx${which}Count ${c}`;
		string += `\n\tstatic const txInteger gx${which}[mx${which}Count] ICACHE_RODATA_ATTR = {`;
		for (let i = 0; i < c; i++) {
			const result = results[i];
			if (i % 32 == 0)
				string += "\n\t\t";
			string += `${results[i]},`;
		}		
		string += "\n\t};";
		this.report(string);
	}
	run() {
		const path = this.resolveFilePath("./data/DerivedCoreProperties.txt");
		const string = this.readFileString(path);
		const firstBuffer = this.parse(string, "# Derived Property: ID_Start");
		const nextBuffer = this.parse(string, "# Derived Property: ID_Continue");
		firstBuffer[0x0024] = 1;
		firstBuffer[0x005F] = 1;
		nextBuffer[0x0024] = 1;
		nextBuffer[0x005F] = 1;
		nextBuffer[0x200C] = 1;
		nextBuffer[0x200D] = 1;
		this.print("IdentifierFirst", this.compress(firstBuffer));
		this.print("IdentifierNext", this.compress(nextBuffer));
	}
}
