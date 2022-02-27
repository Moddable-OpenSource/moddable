import {TOOL, FILE} from "tool";

const mxHangulSBase = 0xAC00;
const mxHangulSCount = 11172;
const mxHangulLBase = 0x1100;
const mxHangulLCount = 19;
const mxHangulVBase = 0x1161;
const mxHangulVCount = 21;
const mxHangulTBase = 0x11A7;
const mxHangulTCount = 28;
const mxHangulNCount = 588;

export default class extends TOOL {
	constructor(argv) {
		super(argv);
	}
	buildCompositions() {
		let compositions = [];
		this.decompositions.forEach(decomposition => {
			if (decomposition.compatible)
				return;
			if (decomposition.excluded)
				return;
			let to = decomposition.code;
			let from = decomposition.form[0];
			let code = decomposition.form[1];
			let composition = compositions.find(item => item.code == code);
			if (!composition) {
				composition = { code, formers: [] };
				compositions.push(composition);
			}
			composition.formers.push({ from, to });	
		});
		compositions.sort((a, b) => a.code - b.code);
		compositions.forEach(composition => {
			composition.formers.sort((a, b) => a.from - b.from);
		});
		this.compositions = compositions;
	}
	findDecomposition(code) {
		let decompositions = this.decompositions;
		let min = 0;
		let max = decompositions.length;
		while (min < max) {
			let mid = (min + max) >> 1;
			let result = decompositions[mid];
			let found = result.code;
			if (code < found)
				max = mid;
			else if (code > found)
				min = mid + 1;
			else
				return result;
		}
		return null;
	}
	findOrder(code) {
		let combinations = this.combinations;
		let min = 0;
		let max = combinations.length;
		while (min < max) {
			let mid = (min + max) >> 1;
			let result = combinations[mid];
			let found = result.code;
			if (code < found)
				max = mid;
			else if (code > found)
				min = mid + 1;
			else
				return result.order;
		}
		return 0;
	}
	parseCompositionExclusions() {
		const path = this.resolveFilePath("./data/CompositionExclusions.txt");
		const string = this.readFileString(path);
		const lines = string.split("\n");
		for (let line of lines) {
			if (line.length == 0)
				continue;
			if (line[0] == "#")
				continue;
			const code = parseInt(line, 16);
			const decomposition = this.findDecomposition(code);
			if (decomposition)
				decomposition.excluded = true;
			else
				this.report(code);
		}
	}
	parseUnicodeData() {
		const path = this.resolveFilePath("./data/UnicodeData.txt");
		const string = this.readFileString(path);
		const lines = string.split("\n");
		const decompositions = [];
		const combinations = [];
		for (let line of lines) {
			const fields = line.split(";");
			const code = parseInt(fields[0], 16);
			let form = fields[5];
			if (form) {
				let compatible = false;
				form = form.split(" ");
				if (form[0].match(/<[^>]+>/)) {
					compatible = true;
					form = form.slice(1);
				}
				form = form.map(item => parseInt(item, 16));
				let excluded = compatible ? false : form.length == 1;
				decompositions.push({ code, compatible, excluded, form })
			}
			let order = fields[3];
			if (order) {
				order = parseInt(order);
				if (order > 0) 
					combinations.push({ code, order })
			}	
		}
		this.decompositions = decompositions;
		this.combinations = combinations;
	}
	printCombinationIndex(which, array) {
		const c = array.length;
		this.report(`#define mxCombinationCount${which} ${c}`);
		let string = `static const txCombination gxCombinationIndex${which}[mxCombinationCount${which}] ICACHE_XS6RO_ATTR = {`;
		for (let i = 0; i < c; i++) {
			if (i % 8 == 0) {
				this.report(string);
				string = "\t";
			}
			let item = array[i];
			string += `{0x${this.toCode(item.code)},${item.count},${item.order}},`;
		}
		this.report(string);
		this.report("};");
	}
	printCombinations() {
		let indexGroups = [ [], [] ];
		let combinations = this.combinations;
		let c = combinations.length;
		let combination = combinations[0];
		let from = combination.code;
		let order = combination.order;
		let to = from;
		for (let i = 1; i < c; i++) {
			let combination = combinations[i];
			if ((to + 1 == combination.code) && (order == combination.order)) {
				to++;
			}
			else {
				indexGroups[from >> 16].push({code:from & 0xFFFF, count:to - from + 1, order});
				from  = combination.code;
				order = combination.order;
				to = from;
			}
		}
		indexGroups[from >> 16].push({code:from & 0xFFFF, count:to - from + 1, order});
		this.printCombinationIndex(0, indexGroups[0]);
		this.printCombinationIndex(1, indexGroups[1]);
	}
	printCompositionData(which, array) {
		const c = array.length;
		this.report(`#define mxCompositionSize${which} ${c * 2}`);
		let string = `static const txU2 gxCompositionData${which}[mxCompositionSize${which}] ICACHE_XS6RO_ATTR = {`;
		for (let i = 0; i < c; i++) {
			if (i % 16 == 0) {
				this.report(string);
				string = "\t";
			}
			let item = array[i];
			string += `0x${this.toCode(item.from)},0x${this.toCode(item.to)},`;
		}
		this.report(string);
		this.report("};");
	}
	printCompositionIndex(which, array) {
		const c = array.length;
		this.report(`#define mxCompositionCount${which} ${c * 2}`);
		let string = `static const txU2 gxCompositionIndex${which}[mxCompositionCount${which}] ICACHE_XS6RO_ATTR = {`;
		for (let i = 0; i < c; i++) {
			if (i % 16 == 0) {
				this.report(string);
				string = "\t";
			}
			let item = array[i];
			string += `0x${this.toCode(item.code)},${item.offset},`;
		}
		this.report(string);
		this.report("};");
	}
	printCompositions() {
		let indexGroups = [ [], [] ];
		let dataGroups = [ [], [] ];
		this.compositions.forEach(composition => {
			let code = composition.code;
			let which = code >> 16;
			let index = indexGroups[which];
			let data = dataGroups[which];
			composition.code &= 0xFFFF;
			composition.offset = data.length;
			index.push(composition);
			composition.formers.forEach(starter => {
				starter.from &= 0xFFFF;
				starter.to &= 0xFFFF;
				data.push(starter);
			});
		});
		indexGroups[0].push({ code:0xFFFF, offset:dataGroups[0].length });
		indexGroups[1].push({ code:0xFFFF, offset:dataGroups[1].length });
		this.printCompositionData(0, dataGroups[0]);
		this.printCompositionIndex(0, indexGroups[0]);
		this.printCompositionData(1, dataGroups[1]);
		this.printCompositionIndex(1, indexGroups[1]);
	}
	printDecompositionData(which, bytes) {
		const buffer = new Uint8Array(ArrayBuffer.fromString(bytes));
		const c = buffer.length;
		this.report(`#define mxDecompositionSize${which} ${c}`);
		let string = `const txU1 gxDecompositionData${which}[mxDecompositionSize${which}] ICACHE_XS6RO_ATTR = {`;
		for (let i = 0; i < c; i++) {
			if (i % 32 == 0) {
				this.report(string);
				string = "\t";
			}
			let value = buffer[i];
			if (value < 16)
				value = "0" + value.toString(16);
			else
				value = value.toString(16);
			string += `0x${value},`;
		}
		this.report(string);
		this.report("};");
	}
	printDecompositionIndex(which, array) {
		const c = array.length;
		this.report(`#define mxDecompositionCount${which} ${c * 2}`);
		let string = `static const txU2 gxDecompositionIndex${which}[mxDecompositionCount${which}] ICACHE_XS6RO_ATTR = {`;
		for (let i = 0; i < c; i++) {
			if (i % 16 == 0) {
				this.report(string);
				string = "\t";
			}
			let item = array[i];
			let offset = item.offset;
			if (item.compatible)
				offset |= 0x8000;
			string += `0x${this.toCode(item.code)},0x${this.toCode(offset)},`;
		}
		this.report(string);
		this.report("};");
	}
	printDecompositions() {
		let arrays = [ [], [], [] ];
		let strings = [ "", "", "" ];
		let max = 0;
		for (let decomposition of this.decompositions) {
			let code = decomposition.code;
			let which = code >> 16;
			let array = arrays[which];
			array.push(decomposition);
			decomposition.code &= 0xFFFF;
			let string = strings[which];
			decomposition.offset = this.strlen(string);
			strings[which] = string + String.fromCodePoint(...decomposition.form);
			if (max < decomposition.form.length)
				max = decomposition.form.length;
		}
		arrays[0].push({ code:0xFFFF, offset:this.strlen(strings[0]) });
		arrays[1].push({ code:0xFFFF, offset:this.strlen(strings[1]) });
		arrays[2].push({ code:0xFFFF, offset:this.strlen(strings[2]) });
		
		this.printDecompositionData(0, strings[0]);
		this.printDecompositionIndex(0, arrays[0]);
		this.printDecompositionData(1, strings[1]);
		this.printDecompositionIndex(1, arrays[1]);
		this.printDecompositionData(2, strings[2]);
		this.printDecompositionIndex(2, arrays[2]);
		this.report(`#define mxDecompositionBufferCount ${max}`);
	}
	run() {
		this.parseUnicodeData();
		this.parseCompositionExclusions();
		this.decompositions.forEach(item => {
			if (item.excluded)
				return;
			let order = this.findOrder(item.code);
			if (order == 0) {
				order = this.findOrder(item.form[0]);
				if (order == 0)
					return;
			}
			item.excluded = true;
		});
		this.buildCompositions();
		this.printCombinations();
		this.printCompositions();
		this.printDecompositions();
//  	this.test();
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
/*	
	compose(codes) {
		let c = codes.length;
		if (c < 2)
			return codes;
		let resultLength = 1;
		let i = 1;
		while (i < c) {
			let code = codes[i];
			let flag = true;
			let composition = this.findComposition(code);
			if (composition) {
				let cc = this.findOrder(code);
				let bi = resultLength - 1;
				while (bi >= 0) {
					let bcode = codes[bi];
					let bcc = this.findOrder(bcode);
					if (bcc == 0) {
						let former = this.composePair(composition, bcode, code);
						if (former) {
							codes[bi] = former;
							flag = false;
						}
						break;
					}
					if (bcc >= cc)
						break;
					bi--;
				}
			}
			if (flag) {
				codes[resultLength] = code;
				resultLength++;
			}
			i++;
		}
		return codes.slice(0, resultLength);
	}
	composePair(composition, starter, code) {
		if (composition == this) {
			if ((mxHangulVBase <= code) && (code < mxHangulVBase + mxHangulVCount)) {
				if ((mxHangulLBase <= starter) && (starter < mxHangulLBase + mxHangulLCount)) {
					starter -= mxHangulLBase;
					code -= mxHangulVBase;
					return mxHangulSBase + (starter * mxHangulNCount) + (code * mxHangulTCount);
				}
			}
			else if ((mxHangulTBase <= code) && (code < mxHangulTBase + mxHangulTCount)) {
				if ((mxHangulSBase <= starter) && (starter < mxHangulSBase + mxHangulSCount) && ((starter - mxHangulSBase) % mxHangulTCount == 0)) {
					return starter + code - mxHangulTBase;
				}
			}
		}
		else {
			let former = composition.formers.find(item => item.from == starter);
			if (former)
				return former.to;
		}
	}
	decompose(result, source, compatible) {
		let c = source.length;
		for (let i = 0; i < c; i++) {
			let code = 	source[i];
			if ((mxHangulSBase <= code) && (code < mxHangulSBase + mxHangulSCount)) {
				let sIndex = code - mxHangulSBase;
				let lIndex = Math.idiv(sIndex, mxHangulNCount);
				let vIndex = Math.idiv(sIndex % mxHangulNCount, mxHangulTCount);
				let tIndex = sIndex % mxHangulTCount;
				result.push(mxHangulLBase + lIndex);
				result.push(mxHangulVBase + vIndex);
				if (tIndex > 0)
					result.push(mxHangulTBase + tIndex);
				result.starterIndex = result.length;
			}
			else {
				let decomposition = this.findDecomposition(code);
				if (!decomposition || ((decomposition.compatible && !compatible))) {
					let cc = this.findOrder(code);
					if (cc == 0) {
						result.push(code);
						result.starterIndex = result.length;
					}
					else {
						let index = result.length - 1;
						let starterIndex = result.starterIndex - 1;
						while (index > starterIndex) {
							let fcode = result[index];
							let fcc = this.findOrder(fcode);
							if (fcc <= cc) {
								result.splice(index + 1, 0, code);
								break;
							}
							index--;
						}
						if (index == starterIndex)
							result.splice(index + 1, 0, code);
					}
				}
				else
					this.decompose(result, decomposition.form, compatible);
			}
		}
	}
	findComposition(code) {
		if ((mxHangulVBase <= code) && (code < mxHangulVBase + mxHangulVCount))
			return this;
		if ((mxHangulTBase <= code) && (code < mxHangulTBase + mxHangulTCount))
			return this;
		return this.compositions.find(item => item.code == code);
	}
	nfd(source) {
		let result = [];
		result.starterIndex = 0;
		this.decompose(result, source, false);
		return result;
	}
	nfkd(source) {
		let result = [];
		result.starterIndex = 0;
		this.decompose(result, source, true);
		return result;
	}
	test() {
		const path = this.resolveFilePath("./data/NormalizationTest.txt");
		const string = this.readFileString(path);
		const lines = string.split("\n");
		for (let line of lines) {
			if (line.length == 0)
				continue;
			if (line[0] == "#")
				continue;
			if (line[0] == "@")
				continue;
			const fields = line.split(";");
			const codes = fields[0].split(" ").map(code => parseInt(code, 16));
			let nfd = this.nfd(codes);
			let nfkd = this.nfkd(codes);
			const nfdString = this.toCodes(nfd);
			const nfkdString = this.toCodes(nfkd);
			const nfcString = this.toCodes(this.compose(nfd));
			const nfkcString = this.toCodes(this.compose(nfkd));
			this.report(`${fields[0]}`);
			if ((fields[1] != nfcString) || (fields[2] != nfdString) || (fields[3] != nfkcString) || (fields[4] != nfkdString)) {
				this.report(`\t${fields[1]};${fields[2]};${fields[3]};${fields[4]}`);
				this.report(`\t${nfcString};${nfdString};${nfkcString};${nfkdString}`);
			}
		}
	}
	toCodes(codes) {
		let result = "";
		codes.forEach(code => {
			if (result)
				result += " ";
			result += this.toCode(code);
		});
		return result;
	}
*/
}
