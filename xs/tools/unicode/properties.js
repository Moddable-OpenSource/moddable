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
	reportCategorySets(category) {
		category.properties.sort((a, b) => a.name.localeCompare(b.name));
		for (let property of category.properties) {
			let ranges = property.ranges;
			let c = ranges.length;
			this.report(`#define mxCharSet_${category.name}_${property.name} ${c * 2}`);
			let string = `static const txInteger ICACHE_RODATA_ATTR gxCharSet_${category.name}_${property.name}[mxCharSet_${category.name}_${property.name}] = {`
			for (let i = 0; i < c; i++) {
				if (i % 8 == 0) {
					this.report(string);
					string = "\t";
				}
				let range = ranges[i];
				string += `0x${this.toHex(range[0])}, `;
				string += `0x${this.toHex(range[1])}, `;
			}
			this.report(string);
			this.report("};");
		}
	}
	reportCategoryProperties(category) {
		let aliases = category.aliases;
		aliases.sort((a, b) => a.name.localeCompare(b.name));
		this.report(`#define mxCharSet_${category.name} ${aliases.length}`)
		this.report(`static const txCharSetUnicodeProperty ICACHE_RODATA_ATTR gxCharSet_${category.name}[mxCharSet_${category.name}] = {`);
		for (let alias of aliases) {
			const property = alias.property;
			this.report(`\t{ "${alias.name}", mxCharSet_${category.name}_${property.name}, gxCharSet_${category.name}_${property.name} },`);
		}
		this.report("};");
	}
	run() {
		let once = true;
		let tool = this;
		let categories = [];
		let category;
		let property;
		
		function assert() {
		}
		function buildString(symbols) {
			if (once) {
				let ranges = [];
				for (let codePoint of symbols.loneCodePoints) {
					ranges.push([codePoint, codePoint + 1]);
				}
				for (let range of symbols.ranges) {
					ranges.push([range[0], range[1] + 1]);
				}
				ranges.sort((a, b) => a[0] - b[0]);
				property.ranges = ranges;
				once = false;
			}
		}
		function testPropertyEscapes(regexp, symbols, string) {
			if (string[1] == "p") {
				let aliasName = string.slice(3, -1);
				const parts = aliasName.split("=");
				if (parts.length == 2) {
					aliasName = parts[1];
				}
				let alias = category.aliases.find(item => item.name == aliasName);
				if (!alias)
					category.aliases.push({ name:aliasName, property });
			}
		}
		const directory = this.resolveDirectoryPath("../../../../test262/test/built-ins/RegExp/property-escapes/generated");
		const names = this.enumerateDirectory(directory);
		for (let name of names) {
			const path = this.joinPath({directory, name});
			if (this.isDirectoryOrFile(path) != 1)
				continue;
			let categoryName = "Binary_Property";
			let propertyName = name;
			const parts = name.split("_-_");
			if (parts.length == 2) {
				categoryName = parts[0];
				propertyName = parts[1];
			}
			category = categories.find(item => item.name == categoryName);
			if (!category) {
				category = { name:categoryName, properties:[], aliases:[] };
				categories.push(category);
			}
			property = { name:propertyName.slice(0, -3) };
			category.properties.push(property);

			let buffer = this.readFileString(path);
			buffer = buffer.replaceAll("/^\\p", "'");
			buffer = buffer.replaceAll("/^\\P", "'");
			buffer = buffer.replaceAll("$/u", "'");
			buffer = buffer.replaceAll("\\P{Any}", "");
			try {
				once = true;
				eval(buffer);
			}
			catch(e) {
			}
		}
		categories.sort((a, b) => a.name.localeCompare(b.name));
		categories.forEach(category => this.reportCategorySets(category));
		categories.forEach(category => this.reportCategoryProperties(category));
	}
}
