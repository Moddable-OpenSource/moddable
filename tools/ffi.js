/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
 
import { PrerequisiteFile} from "mcmanifest";

class TabFile extends PrerequisiteFile {
	constructor(path, tool) {
		super(path, tool);
		this.tabCount = 0;
	}
	line(...strings) {
		if (this.tabCount)
			this.write("\t".repeat(this.tabCount));
		super.line(...strings);
	}
	tab(delta) {
		this.tabCount += delta;
	}
}

class Type {
	constructor(name) {
		this.name = name;
	}
	writeArgument(file, i) {
		file.write(`arg${ i }`);
	}
	writeArgumentConversion(file) {
		debugger;
	}
	writeResult(file, name) {
		file.write(`${ this.name } result = `);
	}
	writeResultConversion(file) {
		debugger;
	}
}

class BigInt64Type extends Type {
	get tsType() { return "bigint"; }
	writeArgumentConversion(file, i) {
		file.line(`${ this.name } arg${ i } = XS->toBigInt64(the, mxArgv(${ i }));`);
	}
	writeResultConversion(file) {
		file.line("XS->fromBigInt64(the, mxResult, result);");
	}
}

class BigUint64Type extends Type {
	get tsType() { return "bigint"; }
	writeArgumentConversion(file, i) {
		file.line(`${ this.name } arg${ i } = XS->toBigUint64(the, mxArgv(${ i }));`);
	}
	writeResultConversion(file) {
		file.line("XS->fromBigUint64(the, mxResult, result);");
	}
}

class IntegerType extends Type {
	get tsType() { return "number"; }
	writeArgumentConversion(file, i) {
		file.line(`${ this.name } arg${ i } = (${ this.name })XS->toInteger(the, mxArgv(${ i }));`);
	}
	writeResultConversion(file) {
		file.line("XS->fromInteger(the, mxResult, (txInteger)result);");
	}
}

class NumberType extends Type {
	get tsType() { return "number"; }
	writeArgumentConversion(file, i) {
		file.line(`${ this.name } arg${ i } = (${ this.name })XS->toNumber(the, mxArgv(${ i }));`);
	}
	writeResultConversion(file) {
		file.line("XS->fromNumber(the, mxResult, (txNumber)result);");
	}
}

class PointerType extends Type {
	get tsType() { return "ArrayBuffer"; }
	writeArgument(file, i) {
		file.write(`(${ this.name })*arg${ i }`);
	}
	writeArgumentConversion(file, i) {
		file.line(`void** arg${ i } = XS->toArrayBufferHandle(the, mxArgv(${ i }), 0);`);
	}
}

class PointerSizeType extends Type {
	get tsType() { return "ArrayBuffer"; }
	constructor(name) {
		const bracket = name.indexOf("[");
		super(name.slice(0, bracket) + "*");
		this.count = parseInt(name.slice(bracket + 1, -1));
	}
	writeArgument(file, i) {
		file.write(`(${ this.name })*arg${ i }`);
	}
	writeArgumentConversion(file, i) {
		file.line(`void** arg${ i } = XS->toArrayBufferHandle(the, mxArgv(${ i }), ${ this.count } * sizeof(${ this.name.slice(0, -1) }));`);
	}
	writeResultConversion(file) {
		file.line("if (result) {");
		file.tab(1);
		file.line(`XS->fromArrayBuffer(the, mxResult, result, ${ this.count } * sizeof(${ this.name.slice(0, -1) }), -1);`);
		file.line("free(result);");
		file.tab(-1);
		file.line("} else {");
		file.tab(1);
		file.line("XS->abort(the, XS_NOT_ENOUGH_MEMORY_EXIT);");
		file.tab(-1);
		file.line("}");
	}
}

class StringType extends Type {
	get tsType() { return "string"; }
	writeArgument(file, i) {
		file.write(`*arg${ i }`);
	}
	writeArgumentConversion(file, i) {
		file.line(`char** arg${ i } = XS->toStringHandle(the, mxArgv(${ i }));`);
	}
	writeResultConversion(file) {
		if (this.name.startsWith("const ")) {
			file.line("XS->fromStringX(the, mxResult, (txString)result);");
		}
		else {
			file.line("if (result) {");
			file.tab(1);
			file.line("XS->fromString(the, mxResult, (txString)result);");
			file.line("free(result);");
			file.tab(-1);
			file.line("} else {");
			file.tab(1);
			file.line("XS->abort(the, XS_NOT_ENOUGH_MEMORY_EXIT);");
			file.tab(-1);
			file.line("}");
		}
	}
}

class UnsignedType extends Type {
	get tsType() { return "number"; }
	writeArgumentConversion(file, i) {
		file.line(`${ this.name } arg${ i } = (${ this.name })XS->toUnsigned(the, mxArgv(${ i }));`);
	}
	writeResultConversion(file) {
		file.line("XS->fromUnsigned(the, mxResult, (txUnsigned)result);");
	}
}

class VoidType extends Type {
	get tsType() { return "void"; }
	constructor() {
		super("void");
	}
	writeResult(file, name) {
	}
	writeResultConversion(file, name) {
	}
}

const ArgumentConstructors = Object.freeze({
	"char*": StringType,
	"double": NumberType,
	"double*": PointerType,
	"double[]": PointerSizeType,
	"float": NumberType,
	"float*": PointerType,
	"float[]": PointerSizeType,
	"int8_t": IntegerType,
	"int8_t*": PointerType,
	"int8_t[]": PointerSizeType,
	"int16_t": IntegerType,
	"int16_t*": PointerType,
	"int16_t[]": PointerSizeType,
	"int32_t": IntegerType,
	"int32_t*": PointerType,
	"int32_t[]": PointerSizeType,
	"int64_t": BigInt64Type,
	"int64_t*": PointerType,
	"int64_t[]": PointerSizeType,
	"uint8_t": UnsignedType,
	"uint8_t*": PointerType,
	"uint8_t[]": PointerSizeType,
	"uint16_t": UnsignedType,
	"uint16_t*": PointerType,
	"uint16_t[]": PointerSizeType,
	"uint32_t": UnsignedType,
	"uint32_t*": PointerType,
	"uint32_t[]": PointerSizeType,
	"uint32_t[]": PointerSizeType,
	"uint64_t": BigUint64Type,
	"uint64_t*": PointerType,
	"uint64_t[]": PointerSizeType,
	"void*": PointerType,
}, true);

const ResultConstructors =  Object.freeze({
	"char*": StringType,
	"const char*": StringType,
	"double": NumberType,
	"double[]": PointerSizeType,
	"float": NumberType,
	"float[]": PointerSizeType,
	"int8_t": IntegerType,
	"int8_t[]": PointerSizeType,
	"int16_t": IntegerType,
	"int16_t[]": PointerSizeType,
	"int32_t": IntegerType,
	"int32_t[]": PointerSizeType,
	"int64_t": BigInt64Type,
	"int64_t[]": PointerSizeType,
	"uint8_t": UnsignedType,
	"uint8_t[]": PointerSizeType,
	"uint16_t": UnsignedType,
	"uint16_t[]": PointerSizeType,
	"uint32_t": UnsignedType,
	"uint32_t[]": PointerSizeType,
	"uint64_t": BigUint64Type,
	"uint64_t[]": PointerSizeType,
	"void": VoidType,
}, true);

class FFIGlue {
	constructor(tool) {
		this.tool = tool;
	}
	addMCConfigCFile(tool, signatures) {
		const source = tool.tmpPath + tool.slash + "mc.ffi.c";
		const file = new TabFile(source, tool);
		this.generateCFile(file, signatures, false, false);
		file.close();
		const target = "mc.ffi.c.o";
		tool.cFiles.push({ target, source });
	}
	addMCConfigDTSFile(tool, signatures) {
		const source = tool.tmpPath + tool.slash + "mc.ffi.d.ts";
		const file = new TabFile(source, tool);
		this.generateDTSFile(file, signatures, "mc/ffi");
		file.close();
		const target = "mc" + tool.slash + "ffi";
		tool.dtsFiles.push({ target, source });
	}
	addMCConfigJSFile(tool, description) {
		const folder = "mc";
		tool.createDirectory(tool.modulesPath + tool.slash + folder);
		const source = tool.tmpPath + tool.slash + "mc.ffi.js";
		const file = new TabFile(source, tool);
		this.generateMCConfigJSFile(file);
		file.close();
		const target = folder + tool.slash + "ffi.xsb";
		tool.jsFiles.push({ source, target });
		if (tool.preloads.length)
			tool.preloads.push("mc" + tool.slash + "ffi.xsb");
	}
	addMCRunCFile(tool, signatures) {
		const source = tool.tmpPath + tool.slash + "mc.ffi.c";
		const file = new TabFile(source, tool);
		this.generateCFile(file, signatures, true, tool.windows);
		file.close();
		const target = "mc.ffi.c.o";
		tool.cFiles.push({ target, source });
	}
	addMCRunCFolder(tool) {
		const folders = tool.cFolders;
		const directory = tool.xsPath + tool.slash + "includes";
		if (!folders.already[directory]) {
			folders.already[directory] = true;
			folders.push(directory);
		}
	}
	addMCRunDTSFile(tool, signatures) {
		const source = tool.tmpPath + tool.slash + "mc.ffi.d.ts";
		const file = new TabFile(source, tool);
		this.generateDTSFile(file, signatures, "ffi");
		file.close();
		const target = "ffi";
		tool.dtsFiles.push({ target, source });
	}
	addSources(tool, sources) {
		const files = tool.cFiles;
		const folders = tool.cFolders;
		sources.forEach(source => {
			const parts = tool.splitPath(source);
			const directory = parts.directory;
			const target = parts.name + ".c.o";
			if (!files.already[source]) {
				files.already[source] = true;
				files.push({ target, source });
			}
			if (!folders.already[directory]) {
				folders.already[directory] = true;
				folders.push(directory);
			}
		});
	}
	addXSFile(tool) {
		const files = tool.cFiles;
		const source = tool.xsPath + tool.slash + "sources" + tool.slash + "xsffi.c";
		const target = "xsffi.c.o";
		if (!files.already[source]) {
			files.already[source] = true;
			files.push({ target, source });
		}
	}
	normalizeType(type) {
		this.count = undefined;
		if (type.endsWith(" *")) {
			type = type.slice(0, -2) + "*";
		}
		else if (type.endsWith("]")) {
			const bracket = type.indexOf("[");
			if (bracket > 0) {
				const count = type.slice(bracket + 1, -1);
				if (/^[1-9][0-9]*$/.test(count)) {
					type = type.slice(0, bracket) + "[]";
				}
			}
		}
		return type;
	}
	parseSignatures(functions) {
		const tool = this.tool;
		const signatures = [];
		if (!(typeof(functions) == "object")) {
			tool.reportError(null, 0, `functions: not an object`);
		}
		else {
			for (let name in functions) {
				const signature = functions[name];
				if (!(typeof(signature) == "object")) {
					tool.reportError(null, 0, `functions.${ name }: not an object`);
					signatures.push({ name });
				}
				else {
					let resultType = signature.returns || "void"; 
					let Constructor = ResultConstructors[this.normalizeType(resultType)];
					if (!Constructor)
						tool.reportError(null, 0, `functions.${ name }.returns: invalid type ${resultType}`);
					else
						resultType = new Constructor(resultType, this.count);
					
					const argumentTypes = signature.arguments;
					if (argumentTypes) {
						if (!Array.isArray(argumentTypes)) {
							tool.reportError(null, 0, `functions.${ name }.arguments: not an array`);
						}
						else {
							for (let i = 0; i < argumentTypes.length; i++) {
								let argumentType = argumentTypes[i];
								let Constructor = ArgumentConstructors[this.normalizeType(argumentType)];
								if (!Constructor)
									tool.reportError(null, 0, `functions.${ name }.arguments[${ i }]: invalid type ${argumentType}`);
								else
									argumentTypes[i] = new Constructor(argumentType, this.count);
							}
						}
					}
					else 
						argumentTypes = [];
					signatures.push({ name, resultType, argumentTypes });
				}
			}
			if (signatures.length == 0)
				tool.reportError(null, 0, `functions: empty object`);
		}
		if (tool.errorCount) {
			throw new Error(`Invalid functions: ${ tool.errorCount } error(s)`);
		}
		return signatures;
	}
	generateCFile(file, signatures, mod, windows) {
		file.line(`/* FFI GENERATED FILE; DO NOT EDIT! */`);
		file.line();
		file.line(`#include "xsffi.h"`);
		file.line();
		if (mod) {
			file.line(`txAPI* XS = NULL;`);
		}
		else {
			file.line(`extern txAPI gxAPI;`);
			file.line(`txAPI* XS = &gxAPI;`);
		}
		file.line();
		for (let signature of signatures) {
			let length = signature.argumentTypes.length;
			file.write(`extern ${ signature.resultType.name } `);
			file.write(`${ signature.name }(`);
			for (let i = 0; i < length; i++) {
				if (i > 0)
					file.write(`, `);
				file.write(`${ signature.argumentTypes[i].name } arg${ i }`);
			}
			file.write(`);\n`);
		}
		file.line();
		for (let signature of signatures) {
			let length = signature.argumentTypes.length;
			file.line(`static void xs_${ signature.name }(txMachine* the) {`);
			file.tab(1);
			for (let i = 0; i < length; i++) {
				signature.argumentTypes[i].writeArgumentConversion(file, i);
			}
			
			file.write(`\t`);
			signature.resultType.writeResult(file);
			file.write(`${ signature.name }(`);
			for (let i = 0; i < length; i++) {
				if (i > 0)
					file.write(`, `);
				signature.argumentTypes[i].writeArgument(file, i);
			}
			file.write(`);\n`);
			
			signature.resultType.writeResultConversion(file);
			file.tab(-1);
			file.line(`}`);
			file.line();
		}
		if (mod) {
			file.line(`void fxBuildFFI(txMachine* the, txAPI* api) {`);
			file.tab(1);
			file.line(`XS = api;`);
		}
		else {
			file.line(`void FFI_destructor(void* the) {`);
			file.line(`}`);
			file.line();
			file.line(`void FFI_constructor(txMachine* the) {`);
			file.tab(1);
		}
		for (let signature of signatures) {
			let length = signature.argumentTypes.length;
			file.line(`XS->newHostFunction(the, xs_${ signature.name }, ${ length }, 0, 0);`);
			file.line(`XS->push(the, mxThis);`);
			file.line(`XS->defineID(the, XS->id(the, "${ signature.name }"), 0, 0x0E);`);
			file.line(`XS->pop(the);`);
		}
		file.tab(-1);
		file.line(`}`);
	}
	generateDTSFile(file, signatures, moduleName) {
		file.line(`/* FFI GENERATED FILE; DO NOT EDIT! */`);
		file.line();
		file.line(`declare module "${ moduleName }" {`);
		file.tab(1);
		file.line(`export default class FFI {`);
		file.tab(1);
		file.line(`constructor();`);
		for (let signature of signatures) {
			let args = signature.argumentTypes.map((type, i) => `arg${ i }: ${ type.tsType }`).join(", ");
			file.line(`${ signature.name }(${ args }): ${ signature.resultType.tsType };`);
		}
		file.tab(-1);
		file.line(`}`);
		file.tab(-1);
		file.line(`}`);
	}
	generateMCConfigJSFile(file) {
		file.line(`/* FFI GENERATED FILE; DO NOT EDIT! */`);
		file.line();
		file.line(`class FFI extends Native("FFI_destructor") {`);
		file.tab(1);
		file.line(`constructor() { super(); native("FFI_constructor").call(this); }`);
		file.tab(-1);
		file.line(`}`);
		file.line(`export default FFI;`);
	}
	mcConfig(tool, description) {
		const signatures = this.parseSignatures(description.functions);
		this.addXSFile(tool);
		this.addSources(tool, description.sources);
		this.addMCConfigJSFile(tool);
		this.addMCConfigCFile(tool, signatures);
		this.addMCConfigDTSFile(tool, signatures);
	}
	mcRun(tool, description) {
		let path = tool.fragmentPath.slice(0, -3) + "-ffi.mk";
		path = tool.resolveFilePath(path);
		if (!path)
			throw new Error("No FFI support!");
		tool.fragmentPath = path;
		const signatures = this.parseSignatures(description.functions);
		if (tool.platform == "pebble") {
			path = tool.mainPath + tool.slash + ".." + tool.slash + "c";
			path = tool.resolveDirectoryPath(path);
			if (!path)
				throw new Error("No c directory!");
			const file = new TabFile(path + tool.slash + "mc.ffi.c", tool);
			this.generateCFile(file, signatures, true, false);
			file.close();
		}
		else {
			this.addSources(tool, description.sources);
			this.addMCRunCFile(tool, signatures);
			this.addMCRunCFolder(tool);
		}
		this.addMCRunDTSFile(tool, signatures);
	}
}

export default FFIGlue;
