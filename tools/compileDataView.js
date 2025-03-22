/*
* Copyright (c) 2021-2025  Moddable Tech, Inc.
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
*/

/*

	to do:

		- bitfields that align to byte and are a multiple of 8 in size special case
		- faster generated code for set of embedded views
		- initializers
		- ViewArray doesn't work with JSON

*/

const Version = 3;

const byteCounts = {
	Int8: 1,
	Int16: 2,
	Int32: 4,
	Uint8: 1,
	Uint16: 2,
	Uint32: 4,
	Float16: 2,
	Float32: 4,
	Float64: 8,
	BigInt64: 8,
	BigUint64: 8
};

const shiftCounts = {
	Int8: 0,
	Int16: 1,
	Int32: 2,
	Uint8: 0,
	Uint16: 1,
	Uint32: 2,
	Float16: 1,
	Float32: 2,
	Float64: 3,
	BigInt64: 3,
	BigUint64: 3
};

const TypeAliases = {
	uint8_t:  "Uint8",
	uint16_t:  "Uint16",
	uint32_t:  "Uint32",
	uint64_t:  "BigUint64",
	int8_t:  "Int8",
	int16_t:  "Int16",
	int32_t:  "Int32",
	int64_t:  "BigInt64",
	float16_t:  "Float16",
	float32_t:  "Float32",
	float64_t:  "Float64",
	float:  "Float32",
	double:  "Float64",
	boolean: "Boolean",
	bool: "Boolean"
};

const TypeScriptTypeAliases = {
	Uint8: "number",
	Uint16: "number",
	Uint32: "number",
	BigUint64: "bigint",
	Int8: "number",
	Int16: "number",
	Int32: "number",
	BigInt64: "bigint",
	Float16: "number",
	Float32: "number",
	Float64: "number",
	Boolean: "boolean"
}

const isTypedef = Symbol("typedef");
const anonymous = Symbol("anonymous");

let className;
let superClassName;
let classUsesEndian;
let output;
let classes;
let properties;
let bitfields;
let jsonOutput;
let fromOutput;
let byteOffset;
let littleEndian;
let hostEndian
let useEndian;	// = littleEndian ?? hostEndian
let language;
let platform;
let doSet;
let doGet;
let doExport;
let pack;
let extendsClass;
let imports;
let outputByteLength;
let checkByteLength;
let union;
let enumState;
let enums;
let enumContext;
let classAlign;
let anonymousUnion;
let json;
let bitfieldsLSB;
let comments;
let jsdocComment;
let jsdocClassComment;
let header;
let usesText;
let usesEndianProxy;
let usesViewArray;
let conditionals;
let final;
let paddingPrefix;
let injectInterface;
let exports;
let outputSource;

class Output extends Array {
	add(line) {
		if ("object" === typeof line) {
			line = line[language];
			if (undefined === line)
				throw new Error(`no generated code for ${language}`);
		}
		if (line.length) this.push(line);
	}
	push(value, etc) {
		if (etc) throw new Error("unexpected");
		if (undefined === value)
			return;
		super.push(value);
	}
}

const hex = "0123456789ABCDEF";
function toHex(value, byteCount = 4) {
	let result = "";
	let nybbles = byteCount * 2;
	while (value && nybbles--) {
		result = hex[value & 15] + result;
		value = (value & 0xFFFFFFF0) >>> 4;
	}
	return "0x" + (("" === result) ? "0" : result);
}

function booleanSetting(value, name) {
	if ("true" === value)
		return true;

	if ("false" === value)
		return false;

	throw new Error(`invalid ${name} "${value}" specified`);
}

function endField(byteCount) {
	if (undefined !== union)
		union = Math.max(byteCount, union);
	else
		byteOffset += byteCount;
}

function validateName(name) {
	//@@ naive
	const c = name.charCodeAt(0);
	if ((48 <= c) && (c < 58))
		throw new Error(`invalid name "${name}"`);		
	if (name.includes(";") || name.includes("+")  || name.includes("-") || name.includes(".") || name.includes("&"))
		throw new Error(`invalid name "${name}"`);

	return name;
}

function splitSource(source) {
	let parts = [], part = "";
	let map = [];
	let line = 1;
	let directive = false, lineStart = true;
	let error;

splitLoop:
	for (let i = 0, length = source.length; i < length; i++) {
		const c = source[i]

		if ("\n" === c)
			lineStart = true;

		switch (c) {
			case "*":
				if ("/" === part) {
					part = "";

					const endComment = source.indexOf("*/", i);
					if (endComment < 0) {
						error = `unterminated comment, line ${line}`;
						break splitLoop;
					}
					parts.push(source.slice(i - 1, endComment + 2));
					map.push(line);
					line += source.slice(i, endComment).split("\n").length - 1;
					i = endComment + 1;		// with ++ in for loop, this gives next character
					continue splitLoop;
				}
				// fall through to handle "*" as separate part

			case "|":
			case "&":
			case "=":
				if (part) {
					parts.push(part);
					map.push(line);
				}
				if (c === source[i + 1]) {
					parts.push(c + c);
					i += 1;
				}
				else
					parts.push(c);
				map.push(line);
				part = "";
				break;

			case "{":
			case "}":
			case ":":
			case ";":
			case "[":
			case "]":
			case "(":
			case ")":
			case "+":
			case "-":
			case "^":
			case "!":
			case ",":
				if (part) {
					parts.push(part);
					map.push(line);
				}
				parts.push(c);
				map.push(line);
				part = "";
				break;

			case "\n":
			case "\t":
			case " ":
				if (part) {
					parts.push(part);
					map.push(line);
				}
				if ("\n" === c) {
					if (directive) {
						directive = false;
						parts.push(c);
						map.push(line);
					}
					line += 1;
				}

				part = "";
				break;

			case "/":
				if ("/" === part) {
					part = "";
					i = source.indexOf("\n", i);
					if (i < 0)
						break splitLoop;
					i -= 1;		// so line ending is parsed
					continue splitLoop;
				}
				part += c;
				break;

			case "#":
				if (lineStart)
					directive = true;
				part += c;
				break;

			default:
				part += c;
				break;
		}

		if ((" " !== c) && ("\t" !== c) && ("\n" !== c))
			lineStart = false;
	}

	if (part) {
		parts.push(part);
		map.push(line);
	}

	return {parts, map, error};
}

function flushBitfields(bitsToAdd = 32) {
	let total = 0, type, endian, byteCount;

	for (let i = 0; i < bitfields.length; i++)
		total += bitfields[i].bitCount;

	if ((0 == total) || ((total + bitsToAdd) <= 32))
		return;

	if (total <= 8) {
		type = "Uint8";
		byteCount = 1;
		endian = "";
	}
	else {
		type = (total <= 16) ? "Uint16" : "Uint32";
		byteCount = (total <= 16) ? 2 : 4;
		endian = useEndian ? ", true" : ", false";
	}

	let bitsOutput = 0;
	while (bitfields.length) {
		const bitfield = bitfields.shift();
		const bitOffset = bitfieldsLSB ? bitsOutput : ((byteCount << 3) - bitsOutput - bitfield.bitCount);
		const mask = (2 ** bitfield.bitCount) - 1;
		const shiftLeft = bitOffset ? " << " + bitOffset : "";
		const shiftRight = bitOffset ? " >> " + bitOffset : "";

		if (doGet && !bitfield.name.startsWith(paddingPrefix)) {
			if (bitfield.jsdocComment)
				output.push(bitfield.jsdocComment);

			output.add({
				javascript: `   get ${bitfield.name}() {`,
				typescript: `   get ${bitfield.name}(): ${bitfield.boolean ? "boolean" : "number"} {`,
			});
			if (bitfield.boolean)
				output.push(`      return Boolean(this.get${type}(${byteOffset}${endian}) & ${toHex(mask << bitOffset, byteCount)});`);
			else
				output.push(`      return (this.get${type}(${byteOffset}${endian})${shiftRight}) & ${toHex(mask, byteCount)};`);
			output.push(`   }`);
		}

		if (doSet && !bitfield.name.startsWith(paddingPrefix)) {
			output.push(bitfield.jsdocComment);

			output.add({
				javascript: `   set ${bitfield.name}(value) {`,
				typescript: `   set ${bitfield.name}(value: ${bitfield.boolean ? "boolean" : "number"}) {`,
			});
			if (bitfield.boolean) {
				output.push(`      const t = this.get${type}(${byteOffset}${endian});`);
				output.push(`      this.set${type}(${byteOffset}, value ? (t | ${toHex(1 << bitOffset)}) : (t & ${toHex(~(mask << bitOffset), byteCount)})${endian});`);
			}
			else if (1 === bitfield.bitCount) {
				output.push(`      const t = this.get${type}(${byteOffset}${endian});`);
				output.push(`      this.set${type}(${byteOffset}, (value & 1) ? (t | ${toHex(1 << bitOffset)}) : (t & ${toHex(~(mask << bitOffset), byteCount)})${endian});`);
			}
			else {
				output.push(`      const t = this.get${type}(${byteOffset}${endian}) & ${toHex(~(mask << bitOffset), byteCount)};`);
				output.push(`      this.set${type}(${byteOffset}, t | ((value & ${toHex(mask, byteCount)})${shiftLeft})${endian});`);
			}
			output.push(`   }`);
		}

		bitsOutput += bitfield.bitCount;
	}

	bitfields.length = 0;

	endField(byteCount);
}

function setPragma(setting, value) {
	switch (setting) {
		case "extends":
			extendsClass = value ? validateName(value) : "DataView"
			break;

		case "endian":
			if ("little" === value)
				littleEndian = true;
			else if ("big" === value)
				littleEndian = false;
			else if ("host" === value)
				littleEndian = undefined;
			else
				throw new Error(`invalid endian "${value}" specified`);

			useEndian = littleEndian ?? hostEndian;
			break;

		case "hostEndian":
			if (output.length || final.length || className)
				throw new Error("too late to set hostEndian");

			if ("little" === value)
				hostEndian = true;
			else if ("big" === value)
				hostEndian = false;
			else if ("unknown" === value)
				hostEndian = undefined;
			else
				throw new Error(`invalid hostEndian "${value}" specified`);

			useEndian = littleEndian ?? hostEndian;
			break;

		case "pack":
			if (")" === value) {
				value =  kDefaultPack;
				pos -= 1;
			}
			else {
				value = parseInt(value);
				if (0 === value)
					value = kDefaultPack;
				if (![1, 2, 4, 8, 16].includes(value))
					throw new Error(`invalid pack`);
			}
			pack = value;
			break;

		case "language":
			if (output.length || final.length || className)
				throw new Error("too late to set language");

			const languageParts = value.split('/');
			if (languageParts.length < 1 || languageParts.length > 2)
				throw new Error(`invalid language "${value}" specified; missing <language>{/<platform>}`);

			if (!["javascript", "typescript"].includes(languageParts[0]))
				throw new Error(`invalid language "${value}" specified; language "${languageParts[0]}" unknown`);
			if (2 == languageParts.length && !["xs", "node", "web"].includes(languageParts[1]))
				throw new Error(`invalid language "${value}" specified; platform "${languageParts[1]}" unknown`);

			language = languageParts[0];
			platform = languageParts[1] ?? "xs";
			break;

		case "set":
			doSet = booleanSetting(value, setting);
			break;

		case "get":
			doGet = booleanSetting(value, setting);
			break;

		case "export":
			doExport = booleanSetting(value, setting);
			break;

		case "outputByteLength":
			outputByteLength = booleanSetting(value, setting);
			break;

		case "checkByteLength":
			checkByteLength = booleanSetting(value, setting);
			break;

		case "json":
			json = booleanSetting(value, setting);
			break;

		case "bitfields":
			if ("lsb" === value)
				bitfieldsLSB = true;
			else if ("msb" === value)
				bitfieldsLSB = false;
			else
				throw new Error(`invalid bitfields "${value}" specified`);
			break;

		case "comments":
			if (!["header", "false", "true"].includes(value))
				throw new Error(`invalid comments "${value}" specified`);
			comments = value;
			break;

		case "inject":
			if (jsdocClassComment) {
				final.push(jsdocClassComment);
				jsdocClassComment = "";
			}
			final.push(`${className ? '   ' : ''}${value};`);
			break;

		case "injectInterface":
			injectInterface.push(`   ${value};`);
			break;

		case "import":
			imports.push(`import ${value};`);
			break;

		case "outputSource":
			outputSource = booleanSetting(value, setting);
			break;

		default:
			throw new Error(`unknown pragma "${setting}"`);
			break;
	}
}

const kDefaultPack = 16;

function compileDataView(input, pragmas = {}) {
	className = undefined;
	superClassName = undefined;
	classUsesEndian = false;
	output = new Output;
	properties = [];
	classes = {};
	bitfields = [];
	jsonOutput = new Output;
	fromOutput = new Output;
	byteOffset = 0;
	littleEndian = undefined;
	hostEndian = true;
	useEndian = littleEndian ?? hostEndian
	language = "javascript";
	platform = "xs";
	doSet = true;
	doGet = true;
	doExport = true;
	pack = kDefaultPack;
	extendsClass = "DataView";
	imports = [];
	outputByteLength = false;
	checkByteLength = true;
	union = undefined;
	enumState = undefined;
	enums = new Set;
	enumContext = "";
	classAlign = 0;
	anonymousUnion = false;
	json = false;
	bitfieldsLSB = true;
	comments = "header";
	jsdocComment = undefined;
	jsdocClassComment = undefined;
	header = "";
	usesText = false;
	usesEndianProxy = false;
	usesViewArray = false;
	conditionals = [{active: true, else: true}];
	paddingPrefix = "__pad";
	injectInterface = [];
	exports = [];
	outputSource = true;

	final = [];
	const errors = [];
	const lines = input.split("\n");

	const {parts, map, error} = splitSource(input);
	if (error) {
		errors.push(`   ${error}`);
		parts.length = 0;
	}

	for (const name in pragmas)
		setPragma(name, pragmas[name]);

	for (let pos = 0; pos < parts.length; ) {
		const part = parts[pos++];
		if (!part)
			throw new Error("unexpected state");

		try {
			let bitCount, arrayCount;

			if (!conditionals[conditionals.length - 1].active) {
				if (("#if" !== part) && ("#else" !== part) && ("#endif" !== part))
					continue;
			}

			if ("}" == part) {
				if (!className)
					throw new Error(`unexpected }`);

				flushBitfields();

				if (undefined !== union) {
					if (0 === union)
						throw new Error(`empty union`);

					const byteLength = union;
					union = undefined;

					endField(byteLength);
					if (anonymousUnion) {
						anonymousUnion = false;

						if (";" !== parts[pos++])
							throw new Error(`expected semicolon`);

						continue;
					}
				}
				else if (undefined !== enumState) {
					if (anonymous !== className) {
						exports.push(className);

						output.add({
							javascript: `const ${className} = Object.freeze({`,
							typescript: `enum ${className} {`,
						});
						for (let [name, { value, jsdocComment }] of enumState) {
							output.push(jsdocComment);
							if ("string" === typeof value)
								value = '"' + value + '"';
							output.add({
								javascript: `   ${name}: ${value},`,
								typescript: `   ${name} = ${value},`,
							});
						}
						output.add({
							javascript: `});`,
							typescript: `}`,
						});
						output.push(``);

						final = final.concat(output);
						output.length = 0;
						const enumBackingType = TypeAliases[enumState.backingType];
						const enumBackingBytes = byteCounts[enumBackingType];

						classes[className] = {
							byteLength:  enumBackingBytes,
							align: Math.min(pack, enumBackingBytes),
							alignLength: enumBackingBytes 
						};
						TypeAliases[className] = enumBackingType;
					}

					enumState = undefined;
					className = undefined;
					classUsesEndian = false;
					continue;
				}

				if (isTypedef === className) {
					className = validateName(parts[pos++]);
					if (classes[className])
						throw new Error(`duplicate class "${className}"`);
				}

				if (";" !== parts[pos++])
					throw new Error(`expected semicolon`);

				if (json && byteOffset) {
					if (doGet) {
						output.add({
							javascript: `   toJSON() {`,
							typescript: `   toJSON(): object {`
						});
						output.push(`      return {`);
						output = output.concat(jsonOutput);
						if (superClassName)
							output.push(`         ...super.toJSON()`);
						output.push(`      };`);
						output.push(`   }`);
					}
					jsonOutput.length = 0;

					if (doSet) {
						let interfaceTypes = `I${className}`;
						for (let parentClass = superClassName; parentClass; parentClass = classes[parentClass]?.superClassName)
							interfaceTypes += ` & I${parentClass}`;
						output.add({
							javascript: `   static from(obj) {`,
							typescript: `   static from(obj: ${interfaceTypes}): ${className} {`
						});
						if (superClassName)
							output.add({
								javascript: `      const result = super.from(obj);`,
								typescript: `      const result = <${className}> super.from(obj);`
							});
						else
							output.push(`      const result = new this();`);
						output = output.concat(fromOutput.map(e => e.replace("##LATE_CAST##", className)));
						output.push(`      return result;`);
						output.push(`   }`);
					}
					fromOutput.length = 0;
				}

				output.push(`}`);
				output.push(``);

				const start = new Output;
				if ("typescript" == language) {
					exports.push("I" + className);
					start.push(jsdocClassComment);
					if (injectInterface.length == 0)
						start.push(`type I${className} = Omit<${className}, keyof DataView | "toJSON">;`);
					else {
						start.push(`interface I${className} extends Omit<${className}, keyof DataView | "toJSON"> {`);
						injectInterface.forEach((inject) => start.push(inject));
						start.push(`}`);
						injectInterface = [];
					}
				}
				start.push(jsdocClassComment);
				exports.push(className);
				start.push(`class ${className} extends ${superClassName ?? extendsClass} {`);
				jsdocClassComment = undefined;

				let superByteLength = 0;
				for (let parentClass = superClassName; classes[parentClass]; parentClass = classes[parentClass].extendsClass)  
					superByteLength += classes[parentClass].byteLength;

				if (outputByteLength)
					start.push(`   static byteLength = ${byteOffset}`);
				if (classUsesEndian)
					start.push(`   #endian;`);

				if (outputByteLength || classUsesEndian)
					start.push(``);

				if (byteOffset > 0) {
					start.add({
						javascript: `   constructor(data, offset = 0, length = ${byteOffset}) {`,
						typescript: `   constructor(data?: ArrayBufferLike, offset = 0, length = ${byteOffset}) {`
					});

					if (!superClassName)
						start.push(`      super(data ?? new ArrayBuffer(offset + length), offset${checkByteLength ? ", length" : ""})`);
					else
						start.push(`      super(data, offset, length);`);

					if (classUsesEndian) {
						start.push(`      this.setUint8(0, 1);`);
						start.push(`      this.#endian = 1 === this.getUint16(0, true);`);
						start.push(`      this.setUint8[0] = 0;`);
					}
					start.push(`   }`);
				}

				final = final.concat(start, output);

				classes[className] = {
					byteLength: byteOffset,
					align: classAlign,
					alignLength: Math.ceil(byteOffset / classAlign) * classAlign,
					superClassName,
					superByteLength
				};

				output.length = 0;
				byteOffset = 0;
				properties.length = 0;
				className = undefined;
				classAlign = 0;
				superClassName = undefined;

				continue;
			}

			if ((part === "union") && ("{" === parts[pos])) {
				if (!className)
					throw new Error(`anonymous union must be in struct`);

				if (undefined !== union)
					throw new Error(`no nested unions`);

				union = 0;
				anonymousUnion = true;

				pos += 1;
				continue;
			}

			if (part === "union") {
				if ("{" !== parts[pos + 1])
					throw new Error(`open brace expected`);

				className = validateName(parts[pos]);
				if (classes[className])
					throw new Error(`duplicate class "${className}"`);
				classAlign = 1;

				union = 0;
				anonymousUnion = false;

				pos += 2;
				continue;
			}

			if (part === "enum") {
				if (className)
					throw new Error(`enum must be at root`);

				let backingType = 'int32_t';

				if ("{" !== parts[pos]) {
					className = validateName(parts[pos++]);
					if (classes[className])
						throw new Error(`duplicate name "${enumState.name}"`);

					if (':' == parts[pos]) {
						backingType = parts[pos + 1];
						if (!TypeAliases[backingType])
							throw new Error(`unknown enum type ${backingType}`);
						pos += 2;
					}
				}
				else
					className = anonymous;

				enumState = new Map;
				enumState.value = -1;
				enumState.backingType = backingType;

				if ("{" !== parts[pos++])
					throw new Error(`open brace expected`);

				continue;
			}

			if (("typedef" === part) && (("struct" === parts[pos]) || (("volatile" == parts[pos]) && ("struct" === parts[pos + 1])))) {
				if (className)
					throw new Error(`cannot nest structure`);

				if ("volatile" == parts[pos])
					pos += 1;

				if ("{" !== parts[pos + 1])
					throw new Error(`open brace expected`);

				className = isTypedef;
				classAlign = 1;
				jsonOutput = new Output;
				fromOutput = new Output;

				pos += 2;
				continue;
			}

			if ("struct" === part) {
				if (className)
					throw new Error(`cannot nest structure`);

				className = validateName(parts[pos]);
				if (classes[className])
					throw new Error(`duplicate class "${className}"`);

				if (":" == parts[pos + 1]) {
					superClassName = parts[pos + 2];
					if (!classes[superClassName])
						throw new Error(`unknown super class "${superClassName}"`)
					byteOffset = classes[superClassName].byteLength;
				}

				if ("{" !== parts[pos + (superClassName ? 3 : 1)])
					throw new Error(`open brace expected`);

				classAlign = 1;
				jsonOutput = new Output;
				fromOutput = new Output;

				pos += (superClassName ? 4 : 2);
				continue;
			}

			if (part.startsWith("/*")) {
				if (("header" === comments) && (1 === pos))
					header = part;
				else if ("true" === comments) {
					if (part.startsWith('/**')) {
						if (className)
							jsdocComment = '   ' + part;
						else
							jsdocClassComment = '\n' + part;
					}
					else {
						if (className)
							output.push('   ' + part);
						else
							final.push(part);
					}
				}
				continue;
			}

			if ("#pragma" === part) {
				const setting = parts[pos++];
				if ("(" !== parts[pos++])
					throw new Error(`open parenthesis expected`);

				let value = [];
				let nest = 0;
				while (true) {
					const part = parts[pos++];
					if ("\n" === part)
						throw new Error(`unbalanced parenthesis`);
					if ("(" === part)
						nest++;
					else if (")" === part) {
						if (0 === nest) {
							pos--;
							break;
						}
						nest--;
					}
					value.push(part);
				}
				value = value.join(" ");
				
				setPragma(setting, value);

				if (")" !== parts[pos++])
					throw new Error(`close parenthesis expected`);

				if ("\n" !== parts[pos++])
					throw new Error(`end of line expected`);

				continue;
			}
			if ("#if" === part) {
				const end = parts.indexOf("\n", pos);
				if (-1 == end)
					throw new Error(`syntax error`);
				if (!conditionals[conditionals.length - 1].active)
					conditionals.push({active: false});
				else {
					const expression = parts.slice(pos, end).join(" ");
					let f = new Function(`
						const p = new Proxy({}, {
							has(target, property) {
								return "defined" !== property;
							},
							get(target, property) {
								if ("__COMPILEDATAVIEW__" == property)
									return "${Version}";
								if ("__LANGUAGE_${language.toUpperCase()}__" == property)		
									return true;
								if ("__PLATFORM_${platform.toUpperCase()}__" == property)
									return true;
								return undefined;
							}
						});
						const defined = function(t) {return undefined !== t};
						with (p) {
							return ${expression};
						}
						`);
					const active = f() ?? false;
					const type = typeof active;
					if (("boolean" !== type) && ("number" !== type))
						throw new Error(`invalid value for ${expression}`);
					conditionals.push({active: !!active});
				}

				pos = end + 1;
				continue;
			}
			if ("#else" === part) {
				if ((conditionals.length < 2) || conditionals[conditionals.length - 1].else)
					throw new Error(`unmatched #else`);
				if (conditionals[conditionals.length - 2].active) {
					conditionals[conditionals.length - 1].active = !conditionals[conditionals.length - 1].active;
					conditionals[conditionals.length - 1].else = true;
				}

				const end = parts.indexOf("\n", pos);
				if (-1 !== end)
					pos = end + 1;
				continue;
			}
			if ("#endif" === part) {
				if (conditionals.length < 2)
					throw new Error(`unmatched #endif`);
				conditionals.pop();

				const end = parts.indexOf("\n", pos);
				if (-1 !== end)
					pos = end + 1;
				continue;
			}
			if ("#error" === part) {
				const end = parts.indexOf("\n", pos);
				throw new Error(parts.slice(pos, (end < 0) ? parts.length : end).join(" "))
			}

			if (part.startsWith("#"))
				throw new Error(`invalid preprocessor instruction`);

			if (";" === part)
				continue;

			if (!className)
				throw new Error(`unexpected`);

			if (enumState) {
				if (enumState.has(part))
					throw new Error(`duplicate name ${part} in enum`);

				let value = ++enumState.value;
				if ("=" === parts[pos]) {
					pos += 1;
					const comma = parts.indexOf(",", pos);
					const brace = parts.indexOf("}", pos);
					if ((-1 == comma) && (-1 == brace))
						throw new Error(`syntax error`);
					const end = Math.min((comma < 0) ? 32767 : comma, (brace < 0) ? 32767 : brace);
					const expression = parts.slice(pos, end).join(" ");
					let context = "(function () {\n" + enumContext;
					context += `return ${expression};\n`;
					context += `})();`
					enumState.value = value = eval(context);
					const type = typeof value;
					if (("number" != type) && ("string" !== type) && ("boolean" !== type))
						throw new Error(`invalid value for ${part}`);
					pos = end;
				}

				if ("," === parts[pos])
					pos += 1;
				else if ("}" === parts[pos])
					;
				else
					throw new Error(`syntax error`);

				if (enums.has(part))
					throw new Error(`duplicate enum: ${part}`);
				enums.add(part);

				enumState.set(part, { value, jsdocComment });
				jsdocComment = undefined;

				if ("string" === typeof value)
					value = '"' + value + '"';
				enumContext += `const ${part} = ${value};\n`;

				continue;
			}


			let type = part;
			let name = validateName(parts[pos++]);
			const isPadding = name.startsWith(paddingPrefix);

			if (":" === parts[pos]) {
				pos++;
				bitCount = parseInt(parts[pos++]);
				if ((bitCount <= 0) || (bitCount > 32) || isNaN(bitCount))
					throw new Error(`invalid bit count`);
			}
			else {
				if ("[" == parts[pos]) {
					const bracket = parts.indexOf("]", pos + 1);
					if (bracket < 0)
						throw new Error(`right brace expected`);

					const expression = parts.slice(pos + 1, bracket).join(" ");
					let context = "(function () {\n" + enumContext;
					context += `return ${expression};\n`;
					context += `})();`

					arrayCount = eval(context);
					pos = bracket + 1;

					arrayCount = Math.round(arrayCount);
					if ((arrayCount <= 0) || isNaN(arrayCount) || (Infinity === arrayCount))
						throw new Error(`invalid array count`);
				}
			}

			if (properties.includes(name))
				throw new Error(`duplicate name "${name}"`);
			properties.push(name);

			if (";" !== parts[pos++])
				throw new Error(`expected semicolon`);

			if (TypeAliases[type])
				type = TypeAliases[type];

			if (bitCount) {
				if (("Uint8" === type) || ("Uint16" === type) || ("Uint32" === type)) {
					const byteCount = byteCounts[type];
					if ((byteCount * 8) < bitCount)
						throw new Error(`${type} too small for bitfield`);
					type = "Uint";
				}
			}

			switch (type) {
				case "Float16":
				case "Float32":
				case "Float64":
				case "Int8":
				case "Int16":
				case "Int32":
				case "Uint8":
				case "Uint16":
				case "Uint32":
				case "BigInt64":
				case "BigUint64": {
					flushBitfields();
					if (undefined !== bitCount)
						throw new Error(`cannot use bitfield with "${type}"`);

					const byteCount = byteCounts[type];

					const align = Math.min(pack, byteCount);
					if (byteOffset % align)
						endField(align - (byteOffset % align));

					if (classAlign < align)
						classAlign = align;

					if (doGet && !isPadding) {
						output.push(jsdocComment);

						output.add({
							javascript: `   get ${name}() {`,
							typescript: `   get ${name}(): ${(undefined === arrayCount) ? TypeScriptTypeAliases[type] : `${type}Array`} {`
						});
						if (undefined === arrayCount) {
							if (1 === byteCount)
								output.push(`      return this.get${type}(${byteOffset});`);
							else {
								if (undefined !== useEndian)
									output.push(`      return this.get${type}(${byteOffset}, ${useEndian});`);
								else {
									output.push(`      return this.get${type}(${byteOffset}, this.#endian);`);
									classUsesEndian = true;
								}
							}
						}
						else {
							if ((1 === byteCount) || (hostEndian === useEndian))
								output.push(`      return new ${type}Array(this.buffer, this.byteOffset${byteOffset ? (" + " + byteOffset) : ""}, ${arrayCount});`);
							else {
								output.push(`      return new EndianArray(${useEndian}, ${shiftCounts[type]}, DataView.prototype.get${type}, DataView.prototype.set${type}, this.buffer, this.byteOffset${byteOffset ? (" + " + byteOffset) : ""}, ${arrayCount});`);
								usesEndianProxy = true;
							}
						}
						output.push(`   }`);
					}

					if (doSet && !isPadding) {
						output.push(jsdocComment);

						output.add({
							javascript: `   set ${name}(value) {`,
							typescript: `   set ${name}(value: ${(undefined === arrayCount) ? TypeScriptTypeAliases[type] : `ArrayLike<${TypeScriptTypeAliases[type]}>`}) {`,
						});
						output.push();
						if (undefined === arrayCount) {
							if (1 === byteCount)
								output.push(`      this.set${type}(${byteOffset}, value);`);
							else {
								if (undefined !== useEndian)
									output.push(`      this.set${type}(${byteOffset}, value, ${useEndian});`);
								else {
									output.push(`      this.set${type}(${byteOffset}, value, this.#endian);`);
									classUsesEndian = true;
								}
							}
						}
						else {
							if ((byteCount == 1) || (undefined !== useEndian)) {
								output.push(`      for (let i = 0, j = this.byteOffset${byteOffset ? (" + " + byteOffset) : ""}; i < ${arrayCount}; i++, j += ${byteCount})`);
								if (1 === byteCount)
									output.push(`         this.set${type}(j, value[i]);`);
								else
									output.push(`         this.set${type}(j, value[i], ${useEndian});`);
							}
							else
								output.push(`      (new ${type}Array(this.buffer, this.byteOffset${byteOffset ? (" + " + byteOffset) : ""}, ${arrayCount})).set(value);`);
						}

						output.push(`   }`);
					}

					endField((arrayCount ?? 1) * byteCount);

					if (!isPadding) {
						if (undefined === arrayCount)
							jsonOutput.push(`         ${name}: this.${name},`);
						else
							jsonOutput.push(`         ${name}: Array.from(this.${name}),`);

						fromOutput.add({
							javascript: `      if ("${name}" in obj) result.${name} = obj.${name};`,
							typescript: `      if ("${name}" in obj) result.${name} = (<##LATE_CAST##> obj).${name};`
						});
					}
					} break;

				case "char":
					flushBitfields();

					if (undefined !== bitCount)
						throw new Error(`char cannot use bitfield`);

					if (doGet && !isPadding) {
+						output.push(jsdocComment);

						output.add({
							javascript: `   get ${name}() {`,
							typescript: `   get ${name}(): string {`,
						});
						if ((undefined === arrayCount) || (1 === arrayCount))
							output.push(`      return String.fromCharCode(this.getUint8(${byteOffset}));`);
						else {
							if ("xs" === platform)
								output.push(`      return String.fromArrayBuffer(this.buffer.slice(this.byteOffset + ${byteOffset}, this.byteOffset + ${byteOffset + arrayCount}));`);
							else
								output.push(`      return new TextDecoder().decode(this.buffer.slice(this.byteOffset + ${byteOffset}, this.byteOffset + ${byteOffset + arrayCount}));`);
						}
						output.push(`   }`);
					}

					if (doSet && !isPadding) {
						output.push(jsdocComment);

						output.add({
							javascript: `   set ${name}(value) {`,
							typescript: `   set ${name}(value: string) {`,
						});
						output.push();
						if ((undefined === arrayCount) || (1 === arrayCount))
							output.push(`      this.setUint8(${byteOffset}, value.charCodeAt(0));`);
						else {
							if ("xs" === platform)
								output.push(`      const j = new Uint8Array(ArrayBuffer.fromString(value));`);
							else
								output.push(`      const j = new TextEncoder().encode(value);`);
							output.push(`      const byteLength = j.byteLength;`);
							output.push(`      if (byteLength > ${arrayCount})`);
							output.push(`         throw new Error("too long");`);
							output.push(`      for (let i = 0; i < byteLength; i++)`);
							output.push(`         this.setUint8(${byteOffset} + i, j[i]);`);
							output.push(`      for (let i = byteLength; i < ${arrayCount}; i++)`);
							output.push(`         this.setUint8(${byteOffset} + i, 0);`);
						}
						output.push(`   }`);
					}

					endField(arrayCount ?? 1);

					if (!isPadding) {
						jsonOutput.push(`         ${name}: this.${name},`);

						fromOutput.add({
							javascript: `      if ("${name}" in obj) result.${name} = obj.${name};`,
							typescript: `      if ("${name}" in obj) result.${name} = (<##LATE_CAST##> obj).${name};`,
						});
					}
					
					usesText = true;
					break;

				case "Uint":
					if (undefined === bitCount)
						throw new Error(`number of bits in bitfield missing`);

					flushBitfields(bitCount);

					bitfields.push({
						name,
						bitCount,
						jsdocComment
					});

					if (!isPadding) {
						jsonOutput.push(`         ${name}: this.${name},`);

						fromOutput.add({
							javascript: `      if ("${name}" in obj) result.${name} = obj.${name};`,
							typescript: `      if ("${name}" in obj) result.${name} = (<##LATE_CAST##> obj).${name};`,
						});
					}
					break;

				case "Boolean":
					flushBitfields(1);

					if (undefined !== arrayCount)
						throw new Error(`Boolean cannot have array`);

					if (undefined !== bitCount)
						throw new Error(`cannot use bitfield with "${type}"`);

					bitfields.push({
						name,
						bitCount: 1,
						boolean: true,
						jsdocComment
					});

					if (!isPadding) {
						jsonOutput.push(`         ${name}: this.${name},`);

						fromOutput.add({
							javascript: `      if ("${name}" in obj) result.${name} = obj.${name};`,
							typescript: `      if ("${name}" in obj) result.${name} = (<##LATE_CAST##> obj).${name};`,
						});
					}
					break;

				default: {
					if (!classes[type])
						throw new Error(`unknown type "${type}"`);

					flushBitfields();

					if (undefined !== bitCount)
						throw new Error(`cannot use bitfield with "${type}"`);

					const align = Math.min(pack, classes[type].align);
					if (byteOffset % align)
						endField(align - (byteOffset % align));

					if (doGet && !isPadding) {
						output.push(jsdocComment);

						if (undefined === arrayCount) {
							output.add({
								javascript: `   get ${name}() {`,
								typescript: `   get ${name}(): ${type} {`,
							});
								
							output.push(`      return new ${type}(this.buffer, this.byteOffset${byteOffset ? (" + " + byteOffset) : ""});`);
							output.push(`   }`);
						}
						else {
							output.add({
								javascript: `   get ${name}() {`,
								typescript: `   get ${name}(): ArrayLike<${type}> {`,
							});

							output.push(`      return new ViewArray(${type}, this.buffer, this.byteOffset${byteOffset ? (" + " + byteOffset) : ""}, ${arrayCount}, ${classes[type].alignLength});`);
							output.push(`   }`);

							usesViewArray = true;
						}
					}

					if (doSet && !isPadding) {
						output.push(jsdocComment);

						if (undefined === arrayCount) {
							output.add({
								javascript: `   set ${name}(value) {`,
								typescript: `   set ${name}(value: ${type}) {`,
							});
							output.push(`      for (let i = 0; i < ${classes[type].byteLength}; i++)`);
							output.push(`         this.setUint8(i + ${byteOffset}, value.getUint8(i));`);
							output.push(`   }`);
						}
						else {
							output.add({
								javascript: `   set ${name}(value) {`,
								typescript: `   set ${name}(value: ArrayLike<${type}>) {`,
							});
							output.push(`      for (let i = 0, offset = 0; i < ${arrayCount}; i++, offset += ${classes[type].alignLength}) {`) ;
							output.push(`         for (let j = 0, v = value[i]; j < ${classes[type].byteLength}; j++) {`);
							output.push(`            this.setUint8(j + offset + ${byteOffset}, v.getUint8(j));`);
							output.push(`         }`);
							output.push(`      }`);
							output.push(`   }`);
						}
					}

					if (undefined === arrayCount)
						endField(classes[type].byteLength);
					else
						endField(arrayCount * classes[type].alignLength);


					if (!isPadding) {
						jsonOutput.push(`         ${name}: this.${name}.toJSON(),`);

						fromOutput.add({
							javascript: `      if ("${name}" in obj) result.${name} = ${type}.from(obj.${name});`,
							typescript: `      if ("${name}" in obj) result.${name} = ${type}.from((<##LATE_CAST##> obj).${name});`,
						});
					}
					} break;
			}
			jsdocComment = undefined;

		}
		catch (e) {
			errors.push(`   ${e}, line ${map[pos]}: ${lines[map[pos] - 1]}`);
		}
	}

	if (conditionals.length > 1)
		errors.push(`   missing #endif`);

	if (className)
		errors.push(`   incomplete struct at end of file`);

	if (usesText && ("node" === platform))
		imports.push('import {TextEncoder, TextDecoder} from "util";');

	if (imports.length)
		final.unshift(...imports, "");

	if (header)
		final.unshift(header, "");

	if (usesEndianProxy)
		final.push(EndianProxy);

	if (usesViewArray)
		final.push(ViewArray);

	if (exports.length && doExport) {
		final.push("");
		let exportLine = "export { ";
		if (exports.length > 3) {
			final.push(exportLine);
			exportLine = "";
		}
		for (let i = 0; i < exports.length; ++i) {
			if (exportLine.length > 80) {
				final.push("   " + exportLine);
				exportLine = "";
			}
			exportLine = exportLine + exports[i] + ((i < (exports.length - 1)) ? ', ' : '');
		}
		if (exportLine != "") {
			if (exports.length <= 3)
				final.push(exportLine + " };");
			else {
				final.push("   " + exportLine);
				final.push("};");
			}
		}
	}

	if (outputSource) {
		final.push("");
		final.push("/*");
		final.push(`\tView classes generated by https://phoddie.github.io/compileDataView on ${new Date} from the following description:`);
		final.push("*/");
		final.push(input.replace(/^|\n/g, '\n// '));
	}

	if (errors.length) {
		errors.unshift("/*")
		errors.push("*/")
		errors.push("")
	}

	return {
		script: final.join("\n"),
		errors: errors.join("\n"),
		language: ("typescript" === language) ? 'ts' : 'js',
		platform
	}
}

const EndianProxy =
`const Properties = Object.freeze({
	concat: Array.prototype.concat,
	copyWithin: Array.prototype.copyWithin,
	every: Array.prototype.every,
	fill: Array.prototype.fill,
	filter: Array.prototype.filter,
	find: Array.prototype.find,
	findIndex: Array.prototype.findIndex,
	forEach: Array.prototype.forEach,
	includes: Array.prototype.includes,
	indexOf: Array.prototype.indexOf,
	join: Array.prototype.join,
	map: Array.prototype.map,
	reduce: Array.prototype.reduce,
	reduceRight: Array.prototype.reduceRight,
	slice: Array.prototype.slice,
	some: Array.prototype.some,
	sort: Array.prototype.sort,
	[Symbol.isConcatSpreadable]: true
});

class EndianArray {
	constructor(little, shift, doGet, doSet, buffer, byteOffset, length) {
		if ("object" === typeof buffer) {
			byteOffset ??= 0;
			this.count = length ?? Math.trunc((buffer.byteLength - byteOffset) >> shift);
			if ((this.count << shift) > (buffer.byteLength - byteOffset))
				throw new RangeError;
			this.view = new DataView(buffer, byteOffset, this.count << shift);
		}
		else {
			this.count = buffer;
			this.view = new DataView(new ArrayBuffer(this.count << shift))
		}

		this.little = little;
		this.shift = shift;
		this.view.doGet = doGet;
		this.view.doSet = doSet;

		return new Proxy(this, this);
	}
	has(target, property) {
		if (Properties[property] || ("length" === property))
			return true;

		const index = Number(property);
		if (isNaN(index))
			return Reflect.has(target, property);

		return (index >= 0) && (index < this.count)
	}
	get(target, property, receiver) {
		if (undefined !== Properties[property])
			return Properties[property];
		if ("length" === property)
			return this.count;
		
		const index = Number(property);
		if (isNaN(index))
			return Reflect.get(target, property, receiver);

		if ((index >= 0) && (index < this.count))
			return this.view.doGet(index << this.shift, this.little);
	}
	set(target, property, value, receiver) {
		if (Properties[property] || ("length" === property))
			return false;

		const index = Number(property);
		if (isNaN(index))
			return Reflect.set(target, property, value, receiver);

		if ((index < 0) || (index >= this.count))
			return;

		this.view.doSet(index << this.shift, value, this.little);
		return true;
	}
}
`;

const ViewArray =
`class ViewArray {
	constructor(View, buffer, byteOffset, count, size) {
		if ((count * size) > (buffer.byteLength - byteOffset))
			throw new RangeError;

		this.count = count;
		this.buffer = buffer;
		this.byteOffset = byteOffset;
		this.View = View;
		this.size = size;

		return new Proxy(this, this);
	}
	get(target, property, receiver) {
		const index = Number(property);
		if (isNaN(index))
			return Reflect.get(target, property, receiver);

		if ((index < 0) || (index >= this.count))
			return;

		return new (this.View)(this.buffer, this.byteOffset + (this.size * index));
	}
	set(target, property, value, receiver) {
		const index = Number(property);
		if (isNaN(index))
			return Reflect.set(target, property, value, receiver);

		if ((index < 0) || (index >= this.count))
			return;

		if (!(value instanceof this.View))
			throw new Error("bad value");

		const size = this.size;
		const src = new Uint8Array(value.buffer, value.byteOffset, size); 
		const dst = new Uint8Array(this.buffer, this.byteOffset + (size * index), size);
		dst.set(src); 

		return true;
	}
}
`;

globalThis.compileDataView = compileDataView;
export default compileDataView;
