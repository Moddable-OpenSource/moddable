import {TOOL, FILE} from "tool";

export default class extends TOOL {
	constructor(argv) {
		super(argv);
	}
	
	printCaseIndex(which, array) {
		const c = array.length;
		this.report(`#define mxCharCase${which}Count ${c}`);
		let string = `const txCharCase gxCharCase${which}[mxCharCase${which}Count] ICACHE_XS6RO_ATTR = {`;
		for (let i = 0; i < c; i++) {
			if (i % 8 == 0) {
				this.report(string);
				string = "\t";
			}
			let item = array[i];
			let operand = 0;
			let delta = item.delta;
			if (item.count == 0) {
				if (delta < 0) {
					delta = -delta;
					operand |= 0x80;
				}
				else
					operand |= 0x40;
				if (delta == 1) {
					if (item.from & 1)
						operand |= 0x10;
					else
						operand |= 0x20;
				}
			}
			else if (item.count > 0) {
				operand = item.count;	
			}
			string += `{0x${this.toCode(item.from)},0x${this.toByte(item.to - item.from + 1)},0x${this.toByte(operand.toString(16))},0x${this.toCode(delta)}},`;
		}
		this.report(string);
		this.report("};");
	}

	compress(items, specials, which, flag) {
		let results = [];
		let c = items.length;
		let item = items[0];
		let from = item.code;
		let count = item.count;
		let delta = item.delta;
		let to = from;
		for (let i = 1; i < c; i++) {
		  let item = items[i];
		  if ((count == 0) && (to + 1 == item.code) && (delta == item.delta) && (count == item.count)) {
			to++;
		  }
		  else {
			results.push({from, to, count, delta});
			from = item.code;
			count = item.count;
			delta = item.delta;
			to = from;
		  }
		}
		results.push({from, to, count, delta});
		
		let former = results[0];
		if ((former.count == 0) && (Math.abs(former.delta) == 1))
			delta = former.delta;
		else
			delta = 0;
		let i = 1;
		while (i < results.length) {
			let current = results[i];
			if (delta && (current.count == 0) && (current.delta == delta) && ((former.to + 2) == current.from)) {
				former.to = current.to;
				results.splice(i, 1);
			}
			else {
				former = current;
				if ((former.count == 0) && (Math.abs(former.delta) == 1))
					delta = former.delta;
				else
					delta = 0;
				i++;
			}
		}

		let string;
		c = specials.length;
		if (c) {
			this.report(`#define mxSpecialCharCase${which}Count ${c}`);
			string = `const txInteger gxSpecialCharCase${which}[mxSpecialCharCase${which}Count] ICACHE_XS6RO_ATTR = {`;
			for (let i = 0; i < c; i++) {
				if (i % 32 == 0) {
					this.report(string);
					string = "\t";
				}
				string += `0x${specials[i].toString(16)},`;
			}		
			this.report(string);
			this.report("};");
		}
		
		let indexGroups = [ [], [] ];
		c = results.length;
		for (let i = 0; i < c; i++) {
			let result = results[i];
			let from = result.from;
			result.from &= 0xFFFF;
			result.to &= 0xFFFF;
			indexGroups[from >> 16].push(result);
		}
		this.printCaseIndex(which + 0, indexGroups[0]);
		this.printCaseIndex(which + 1, indexGroups[1]);
	}
	filterIgnoreCases() {
		let results = [];
		let items = this.uppers;
		let c = items.length;
		for (let i = 0; i < c; i++) {
			let item = items[i];
			let code = item.code;
			if (code >= 0x10000)
				continue;
			let delta = item.delta;
			let mapping = code + delta;
			if (mapping >= 0x10000)
				continue;
			if ((code >= 128) && (mapping < 128))
				continue;
			results.push({ code, count:0, delta });
		}
		this.ignoreCases = results;
	}
	parseCaseFolding() {
		const path = this.resolveFilePath("./data/CaseFolding.txt");
		const string = this.readFileString(path);
		const lines = string.split("\n");
		const foldings = [];
		for (let line of lines) {
			if (line.length == 0)
				continue;
			if (line[0] == "#")
				continue;
			const fields = line.split(";");
			const status = fields[1].trim();
			if (status == "F")
				continue;
			if (status == "T")
				continue;
			const code = parseInt(fields[0], 16);
			const delta = parseInt(fields[2].trim(), 16) - code;
			foldings.push({ code, count:0, delta })
		}
		this.foldings = foldings;
	}
	parseSpecialCasing() {
		const path = this.resolveFilePath("./data/SpecialCasing.txt");
		const string = this.readFileString(path);
		const lines = string.split("\n");
		const specialLowers = [];
		const specialUppers = [];
		for (let line of lines) {
			if (line.length == 0)
				continue;
			if (line[0] == "#")
				continue;
			const fields = line.split(";");
			if (fields.length == 5) {
				const code = parseInt(fields[0], 16);
				let lowers = fields[1].trim().split(" ").map(value => parseInt(value, 16));
				let uppers = fields[3].trim().split(" ").map(value => parseInt(value, 16));
				if (lowers.length > 1) {
					const item = this.lowers.find(item => item.code == code);
					if (item) {
						item.count = lowers.length;
						item.delta = specialLowers.length;
					}
					else
						this.lowers.push({ code, count:lowers.length, delta:specialLowers.length });
					for (let value of lowers)
						specialLowers.push(value);
				}
				if (uppers.length > 1) {
					const item = this.uppers.find(item => item.code == code);
					if (item) {
						item.count = uppers.length;
						item.delta = specialUppers.length;
					}
					else
						this.uppers.push({ code, count:uppers.length, delta:specialUppers.length });
					for (let value of uppers)
						specialUppers.push(value);
				}
			}
		}
		this.specialLowers = specialLowers;
		this.specialUppers = specialUppers;
	}
	parseUnicodeData() {
		const path = this.resolveFilePath("./data/UnicodeData.txt");
		const string = this.readFileString(path);
		const lines = string.split("\n");
		const uppers = [];
		const lowers = [];
		for (let line of lines) {
			const fields = line.split(";");
			const code = parseInt(fields[0], 16);
			let upper = fields[12];
			let lower = fields[13];
			if (upper) {
				const delta = parseInt(upper, 16) - code;
				uppers.push({ code, count:0, delta })
			}
			if (lower) {
				const delta = parseInt(lower, 16) - code;
				lowers.push({ code, count:0, delta })
			}
		}
		this.lowers = lowers;
		this.uppers = uppers;
	}
	pushConditional(items, code, delta) {
		const item = items.find(item => item.code == code);
		if (item) {
			item.count = -1;
			item.delta = delta;
		}
		else
			items.push({ code, count:-1, delta })
	}
	run() {
		this.parseCaseFolding();
		this.parseUnicodeData();
		this.filterIgnoreCases();
		this.parseSpecialCasing();
		this.pushConditional(this.lowers, 0x03a3, 0); // fxFinalSigmaToLower
		this.lowers.sort((a, b) => a.code - b.code);
		this.uppers.sort((a, b) => a.code - b.code);
		this.compress(this.foldings, [], "Fold", 1);
		this.compress(this.ignoreCases, [], "Ignore", 1);
		this.compress(this.lowers, this.specialLowers, "ToLower", 0);
		this.compress(this.uppers, this.specialUppers, "ToUpper", 1);
	}
	toByte(code) {
		let result = code.toString(16).toUpperCase();
		if (result.length == 1)
			result = "0" + result;
		return result;
	}
	toCode(code) {
		let result = code.toString(16).toUpperCase();
		if (result.length == 1)
			result = "000" + result;
		else if (result.length == 2)
			result = "00" + result;
		else if (result.length == 3)
			result = "0" + result;
		return result;
	}
}
