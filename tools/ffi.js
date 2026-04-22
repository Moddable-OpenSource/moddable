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

class IntegerType extends Type {
	writeArgumentConversion(file, i) {
		file.line(`${ this.name } arg${ i } = (${ this.name })XS->toInteger(the, mxArgv(${ i }));`);
	}
	writeResultConversion(file) {
		file.line("XS->integer(the, mxResult, (txInteger)result);");
	}
}

class NumberType extends Type {
	writeArgumentConversion(file, i) {
		file.line(`${ this.name } arg${ i } = (${ this.name })XS->toNumber(the, mxArgv(${ i }));`);
	}
	writeResultConversion(file) {
		file.line("XS->number(the, mxResult, (txNumber)result);");
	}
}

class PointerType extends Type {
	writeArgument(file, i) {
		file.write(`(${ this.name })*arg${ i }`);
	}
	writeArgumentConversion(file, i) {
		file.line(`void** arg${ i } = XS->toArrayBufferHandle(the, mxArgv(${ i }));`);
	}
}

class StringType extends Type {
	writeArgument(file, i) {
		file.write(`*arg${ i }`);
	}
	writeArgumentConversion(file, i) {
		file.line(`char** arg${ i } = XS->toStringHandle(the, mxArgv(${ i }));`);
	}
	writeResultConversion(file) {
		if (this.name.startsWith("const ")) {
			file.line("XS->stringX(the, mxResult, (txString)result);");
		}
		else {
			file.line("if (result) {");
			file.tab(1);
			file.line("XS->string(the, mxResult, (txString)result);");
			file.line("free(result);");
			file.tab(-1);
			file.line("}");
		}
	}
}

class UnsignedType extends Type {
	writeArgumentConversion(file, i) {
		file.line(`${ this.name } arg${ i } = (${ this.name })XS->toUnsigned(the, mxArgv(${ i }));`);
	}
	writeResultConversion(file) {
		file.line("XS->_unsigned(the, mxResult, (txUnsigned)result);");
	}
}

class VoidType extends Type {
	constructor() {
		super("void");
	}
	writeResult(file, name) {
	}
	writeResultConversion(file, name) {
	}
}

const ArgumentConstructors = {
	"char*": StringType,
	"double": NumberType,
	"double*": PointerType,
	"float": NumberType,
	"float*": PointerType,
	"int8_t": IntegerType,
	"int8_t*": PointerType,
	"int16_t": IntegerType,
	"int16_t*": PointerType,
	"int32_t": IntegerType,
	"int32_t*": PointerType,
	"uint8_t": UnsignedType,
	"uint8_t*": PointerType,
	"uint16_t": UnsignedType,
	"uint16_t*": PointerType,
	"uint32_t": UnsignedType,
	"uint32_t*": PointerType,
	"void*": PointerType,
};

const ResultConstructors = {
	"char*": StringType,
	"const char*": StringType,
	"double": NumberType,
	"float": NumberType,
	"int8_t": IntegerType,
	"int16_t": IntegerType,
	"int32_t": IntegerType,
	"uint8_t": UnsignedType,
	"uint16_t": UnsignedType,
	"uint32_t": UnsignedType,
	"void": VoidType,
};

class FFIGlue {
	constructor(tool) {
		this.tool = tool;
	}
	addMCConfigCFile(tool, signatures) {
		const source = tool.tmpPath + tool.slash + "mc.ffi.c";
		const file = new TabFile(source, tool);
		this.generateCFile(file, signatures, false);
		file.close();
		const target = "mc.ffi.c.o";
		tool.cFiles.push({ target, source });
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
		this.generateCFile(file, signatures, true);
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
		if (type.endsWith(" *")) {
			type = type.slice(0, -2) + "*";
		}
		return type;
	}
	parseSignatures(functions) {
		const tool = this.tool;
		const signatures = [];
		if (!(typeof(functions) == "object")) {
			tool.reportError(null, 0, `functions: not an object!`);
		}
		else {
			for (let name in functions) {
				const signature = functions[name];
				if (!(typeof(signature) == "object")) {
					tool.reportError(null, 0, `functions.${ name }: not an object!`);
					signatures.push({ name });
				}
				else {
					let resultType = this.normalizeType(signature.returns || "void"); 
					let Constructor = ResultConstructors[resultType];
					if (!Constructor)
						tool.reportError(null, 0, `functions.${ name }.returns: invalid type!`);
					else
						resultType = new Constructor(resultType);
					
					const argumentTypes = signature.arguments;
					if (argumentTypes) {
						if (!Array.isArray(argumentTypes)) {
							tool.reportError(null, 0, `functions.${ name }.arguments: not an array!`);
						}
						else {
							for (let i = 0; i < argumentTypes.length; i++) {
								let argumentType = this.normalizeType(argumentTypes[i]);
								let Constructor = ArgumentConstructors[argumentType];
								if (!Constructor)
									tool.reportError(null, 0, `functions.${ name }.arguments[${ i }]: invalid type!`);
								else
									argumentTypes[i] = new Constructor(argumentType);
							}
						}
					}
					else 
						argumentTypes = [];
					signatures.push({ name, resultType, argumentTypes });
				}
			}
			if (signatures.length == 0)
				tool.reportError(null, 0, `functions: empty object!`);
		}
		if (tool.errorCount) {
			throw new Error(`Invalid functions: ${ tool.errorCount } error(s)!`);
		}
		return signatures;
	}
	generateCFile(file, signatures, mod) {
		file.line(`/* FFI GENERATED FILE; DO NOT EDIT! */`);
		file.line();
		file.line(`#include "xsffi.h"`);
		file.line();
		if (mod) {
			file.line(`txAPI* XS = NULL;`);
		}
		else {
			file.line(`extern txAPI _XS;`);
			file.line(`txAPI* XS = &_XS;`);
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
			file.line(`XS->defineID(the, XS->id(the, "${ signature.name }"), 0, 0);`);
			file.line(`XS->pop(the);`);
		}
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
	}
	mcRun(tool, description) {
		let path = tool.fragmentPath.slice(0, -3) + "-ffi.mk";
		path = tool.resolveFilePath(path);
		if (!path)
			throw new Error("No FFI support!");
		tool.fragmentPath = path;
		const signatures = this.parseSignatures(description.functions);
		this.addSources(tool, description.sources);
		this.addMCRunCFile(tool, signatures);
		this.addMCRunCFolder(tool);
	}
}

export default FFIGlue;
