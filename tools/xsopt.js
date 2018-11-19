import {TOOL, FILE} from "tool";

class Code {
	constructor(param) {
		this.param = param;
		this.previous = null;
		this.next = null;
		this.tag = null;
	}
	index(tool) {
		this.tag = tool.tag++;
	}
	optimize(tool) {
		return this.next;
	}
	parse(tool) {
	}
	serialize1(tool) {
		this.offset = tool.codesSize;
		tool.codesSize += this.size;
	}
	serialize2(tool) {
		this.offset = tool.codesSize;
		tool.codesSize += this.size;
	}
	serialize3(tool) {
		this.offset = tool.codesSize;
		tool.codesSize += this.size;
	}
	report(tool, tabs) {
		let result = "";
		let offset = this.offset;
		if (offset < 100000)
			result += " ";
		if (offset < 10000)
			result += " ";
		if (offset < 1000)
			result += " ";
		if (offset < 100)
			result += " ";
		if (offset < 10)
			result += " ";
		result += offset;
		for (let tab = 0; tab < tabs; tab++)
			result += "    ";
		result += " ";
		result += this.name;
		let param = this.reportParam(tool);
		if (param !== undefined) {
			result += " ";
			result += this.reportParam(tool);
		}
		tool.report(result);
		return this.next;
	}
	reportParam(tool) {
		return this.param;
	}
}

class BranchCode extends Code {
	parse(tool) {
		let param = this.param;
		let offset = this.next.offset + param;
		let code = this;
		if (param < 0) {
			code = code.previous;
			while (code) {
				if (code.offset == offset)
					break;
				code = code.previous;
			}
		}
		else {
			code = code.next;
			while (code) {
				if (code.offset == offset)
					break;
				code = code.next;
			}
		}
		this.target = code;
	}
	reportParam(tool) {
		return this.target.offset + " (" + this.param + ")";;
	}
	serialize1(tool) {
		this.offset = tool.codesSize;
		tool.codesSize += 2;
		tool.codesDelta += 3;
	}
	serialize2(tool) {
		let offset = this.target.offset - (tool.codesSize + 5);
		if ((offset < -32768) || (offset + tool.codesDelta > 32767)) {
			this.id += 2;
			this.size = 5;
		}
		else if ((offset < -128) || (offset + tool.codesDelta > 127)) {
			this.id += 1;
			this.size = 3;
			tool.codesDelta -= 2;
		}
		else {
			this.size = 2;
			tool.codesDelta -= 3;
		}
		super.serialize2(tool);
	}
	serialize3(tool) {
		super.serialize3(tool);
		this.param = this.target.offset - tool.codesSize;
	}
}

class CodeCode extends BranchCode {
	report(tool, tabs) {
		super.report(tool, tabs);
		let code = this.next;
		let target = this.target;
		tabs++;
		while (code != target) {
			code = code.report(tool, tabs);
		}
		return target;
	}
}

class EndCode extends Code {
}

class HostCode extends Code {
	parse(tool) {
		this.param = tool.hosts[this.param];
	}
	reportParam(tool) {
		return this.param.param;
	}
	serialize1(tool) {
		this.param.usage++;
		super.serialize1(tool);
	}
}

class IntegerCode extends Code {
	serialize1(tool) {
		let param = this.param;
		if ((param < -32768) || (param > 32767)) {
			this.id += 2;
			this.size = 5;
		}
		else if ((param < -128) || (param > 127)) {
			this.id += 1;
			this.size = 3;
		}
		else
			this.size = 2;
		super.serialize1(tool);
	}
}

class KeyCode extends Code {
	reportParam(tool) {
		let param = this.param;
		if (param)
			return param.param;
		return '?';
	}
	serialize1(tool) {
		let param = this.param;
		if (param)
			param.usage++;
		super.serialize1(tool);
	}
}

class StackCode extends Code {
	serialize1(tool) {
		let param = this.param;
		if (param > 65535) {
			this.id += 2;
			this.size = 5;
		}
		else if (param > 255) {
			this.id += 1;
			this.size = 3;
		}
		else
			this.size = 2;
		super.serialize1(tool);
	}
}

class StringCode extends Code {
	reportParam(tool) {
		return '"' + this.param + '"';
	}
	serialize1(tool) {
		let param = tool.strlen(this.param) + 1;
		if (param > 65535) {
			this.id += 2;
			this.size = 5 + param;
		}
		else if (param > 255) {
			this.id += 1;
			this.size = 3 + param;
		}
		else
			this.size = 2 + param;
		super.serialize1(tool);
	}
}

class Host {
	constructor(arity, key, param) {
		this.arity = arity;
		this.key = key;
		this.param = param;
		this.usage = 0;
	}
	serialize(tool) {
		if (this.usage) {
			this.index = tool.hostsCount;
			this.size = tool.strlen(this.param) + 1;
			let key = this.key;
			if (key)
				key.usage++;
			tool.hostsCount++;
			tool.hostsSize += 3 + this.size;
		}
	}
}

class Key {
	constructor(param) {
		this.param = param;
		this.usage = 0;
	}
	serialize(tool) {
		if (this.usage) {
			this.index = tool.keysCount;
			this.size = tool.strlen(this.param) + 1;
			tool.keysCount++;
			tool.keysSize += this.size;
		}
	}
}

const constructors = {
	ADD: class extends Code {
	},
	ARGUMENT: class extends Code {
	},
	ARGUMENTS: class extends Code {
	},
	ARGUMENTS_SLOPPY: class extends Code {
	},
	ARGUMENTS_STRICT: class extends Code {
	},
	ARRAY: class extends Code {
	},
	ASYNC_FUNCTION: class extends KeyCode {
	},
	ASYNC_GENERATOR_FUNCTION: class extends KeyCode {
	},
	AT: class extends Code {
	},
	AWAIT: class extends Code {
	},
	BEGIN_SLOPPY: class extends Code {
	},
	BEGIN_STRICT: class extends Code {
	},
	BEGIN_STRICT_BASE: class extends Code {
	},
	BEGIN_STRICT_DERIVED: class extends Code {
	},
	BIT_AND: class extends Code {
	},
	BIT_NOT: class extends Code {
	},
	BIT_OR: class extends Code {
	},
	BIT_XOR: class extends Code {
	},
	BRANCH: class extends BranchCode {
		optimize(tool) {
			if (this.target == this.next) {
				tool.remove(this);
			}
			else if (this.target instanceof EndCode) {
				tool.replace(this, new this.target.constructor());
			}
			return this.next;
		}
	},
	BRANCH_ELSE: class extends BranchCode {
	},
	BRANCH_IF: class extends BranchCode {
	},
	BRANCH_STATUS: class extends BranchCode {
	},
	CALL: class extends Code {
	},
	CALL_TAIL: class extends Code {
	},
	CATCH: class extends BranchCode {
	},
	CHECK_INSTANCE: class extends Code {
	},
	CLASS: class extends Code {
	},
	CODE: class extends CodeCode {
	},
	CODE_ARCHIVE: class extends CodeCode {
	},
	CONST_CLOSURE: class extends StackCode {
	},
	CONST_LOCAL: class extends StackCode {
	},
	CONSTRUCTOR_FUNCTION: class extends KeyCode {
	},
	CURRENT: class extends Code {
	},
	DEBUGGER: class extends Code {
	},
	DECREMENT: class extends Code {
	},
	DELETE_PROPERTY: class extends KeyCode {
	},
	DELETE_PROPERTY_AT: class extends Code {
	},
	DELETE_SUPER: class extends KeyCode {
	},
	DELETE_SUPER_AT: class extends Code {
	},
	DIVIDE: class extends Code {
	},
	DUB: class extends Code {
	},
	DUB_AT: class extends Code {
	},
	END: class extends EndCode {
	},
	END_ARROW: class extends EndCode {
	},
	END_BASE: class extends EndCode {
	},
	END_DERIVED: class extends EndCode {
	},
	ENVIRONMENT: class extends Code {
	},
	EQUAL: class extends Code {
	},
	EVAL: class extends Code {
	},
	EVAL_ENVIRONMENT: class extends Code {
	},
	EVAL_REFERENCE: class extends KeyCode {
	},
	EXCEPTION: class extends Code {
	},
	EXPONENTIATION: class extends Code {
	},
	EXTEND: class extends Code {
	},
	FALSE: class extends Code {
	},
	FILE: class extends KeyCode {
	},
	FOR_AWAIT_OF: class extends Code {
	},
	FOR_IN: class extends Code {
	},
	FOR_OF: class extends Code {
	},
	FUNCTION: class extends KeyCode {
	},
	GENERATOR_FUNCTION: class extends KeyCode {
	},
	GET_CLOSURE: class extends StackCode {
	},
	GET_LOCAL: class extends StackCode {
	},
	GET_PROPERTY: class extends KeyCode {
	},
	GET_PROPERTY_AT: class extends Code {
	},
	GET_SUPER: class extends KeyCode {
	},
	GET_SUPER_AT: class extends Code {
	},
	GET_THIS: class extends Code {
	},
	GET_VARIABLE: class extends KeyCode {
	},
	GLOBAL: class extends Code {
	},
	HOST: class extends HostCode {
	},
	IN: class extends Code {
	},
	INCREMENT: class extends Code {
	},
	INSTANCEOF: class extends Code {
	},
	INSTANTIATE: class extends Code {
	},
	INTEGER: class extends IntegerCode {
	},
	INTRINSIC: class extends Code {
	},
	LEFT_SHIFT: class extends Code {
	},
	LESS: class extends Code {
	},
	LESS_EQUAL: class extends Code {
	},
	LET_CLOSURE: class extends StackCode {
	},
	LET_LOCAL: class extends StackCode {
	},
	LINE: class extends Code {
	},
	MINUS: class extends Code {
	},
	MODULE: class extends Code {
	},
	MODULO: class extends Code {
	},
	MORE: class extends Code {
	},
	MORE_EQUAL: class extends Code {
	},
	MULTIPLY: class extends Code {
	},
	NAME: class extends KeyCode {
	},
	NEW: class extends Code {
	},
	NEW_CLOSURE: class extends KeyCode {
	},
	NEW_LOCAL: class extends KeyCode {
	},
	NEW_PROPERTY: class extends Code {
	},
	NEW_TEMPORARY: class extends Code {
	},
	NOT: class extends Code {
	},
	NOT_EQUAL: class extends Code {
	},
	NULL: class extends Code {
	},
	NUMBER: class extends Code {
	},
	OBJECT: class extends Code {
	},
	PLUS: class extends Code {
	},
	POP: class extends Code {
	},
	PROGRAM_ENVIRONMENT: class extends Code {
	},
	PROGRAM_REFERENCE: class extends KeyCode {
	},
	PULL_CLOSURE: class extends StackCode {
	},
	PULL_LOCAL: class extends StackCode {
	},
	REFRESH_CLOSURE: class extends StackCode {
	},
	REFRESH_LOCAL: class extends StackCode {
	},
	RESERVE: class extends StackCode {
	},
	RESET_CLOSURE: class extends StackCode {
	},
	RESET_LOCAL: class extends StackCode {
	},
	RESULT: class extends Code {
	},
	RETHROW: class extends Code {
	},
	RETRIEVE: class extends StackCode {
	},
	RETRIEVE_TARGET: class extends Code {
	},
	RETRIEVE_THIS: class extends Code {
	},
	RETURN: class extends Code {
	},
	SET_CLOSURE: class extends StackCode {
	},
	SET_LOCAL: class extends StackCode {
	},
	SET_PROPERTY: class extends KeyCode {
	},
	SET_PROPERTY_AT: class extends Code {
	},
	SET_SUPER: class extends KeyCode {
	},
	SET_SUPER_AT: class extends Code {
	},
	SET_THIS: class extends Code {
	},
	SET_VARIABLE: class extends KeyCode {
	},
	SIGNED_RIGHT_SHIFT: class extends Code {
	},
	START_ASYNC: class extends Code {
	},
	START_ASYNC_GENERATOR: class extends Code {
	},
	START_GENERATOR: class extends Code {
	},
	STORE: class extends StackCode {
	},
	STORE_ARROW: class extends Code {
	},
	STRICT_EQUAL: class extends Code {
	},
	STRICT_NOT_EQUAL: class extends Code {
	},
	STRING: class extends StringCode {
	},
	STRING_ARCHIVE: class extends StringCode {
	},
	SUBTRACT: class extends Code {
	},
	SUPER: class extends Code {
	},
	SWAP: class extends Code {
	},
	SYMBOL: class extends KeyCode {
	},
	TARGET: class extends Code {
	},
	TEMPLATE: class extends Code {
	},
	THIS: class extends Code {
	},
	THROW: class extends Code {
	},
	THROW_STATUS: class extends Code {
	},
	TO_INSTANCE: class extends Code {
	},
	TRANSFER: class extends Code {
	},
	TRUE: class extends Code {
	},
	TYPEOF: class extends Code {
	},
	UNCATCH: class extends Code {
	},
	UNDEFINED: class extends Code {
	},
	UNSIGNED_RIGHT_SHIFT: class extends Code {
	},
	UNWIND: class extends StackCode {
	},
	VAR_CLOSURE: class extends StackCode {
	},
	VAR_LOCAL: class extends StackCode {
	},
	VOID: class extends Code {
	},
	WITH: class extends Code {
	},
	WITHOUT: class extends Code {
	},
	YIELD: class extends Code {
	},
	Host,
	Key,
}

export default class extends TOOL {
	constructor(argv) {
		super(argv);
		this.hosts = null;
		this.keys = null;
		this.prepare(constructors);
		
		this.inputPath = "";
		this.optimization = 1;
		this.outputPath = "";
		this.verbose = false;
		
		let argc = argv.length;
		for (let argi = 1; argi < argc; argi++) {
			let option = argv[argi], name, path;
			switch (option) {
			case "-o":
				argi++;
				if (argi >= argc)
					throw new Error("-o: no directory!");
				name = argv[argi];
				if (this.outputPath)
					throw new Error("-o '" + name + "': too many directories!");
				path = this.resolveDirectoryPath(name);
				if (!path)
					throw new Error("-o '" + name + "': directory not found!");
				this.outputPath = path;
				break;
			case "-0":
				this.optimization = 0;
				break;
			case "-1":
				this.optimization = 1;
				break;
			case "-v":
				this.verbose = true;
				break;
			default:
				name = argv[argi];
				if (this.inputPath)
					throw new Error("'" + name + "': too many files!");
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("'" + name + "': file not found!");
				this.inputPath = path;
				break;
			}
		}
		if (!this.inputPath)
			throw new Error("no file!");
		if (this.outputPath) {
			let parts = this.splitPath(this.inputPath);
			parts.directory = this.outputPath;
			this.outputPath = this.joinPath(parts);
		}
		else
			this.outputPath = this.inputPath;
	}
	append(code) {
		let last = this.last;
		if (last) {
			last.next = code;
			code.previous = last;
		}
		else
			this.first = code;;
		this.last = code;
	}
	optimize() {
		let code = this.first;
		while (code) {
			code = code.optimize(this);
		}
	}
	parse(path) {
		let code;
		this.first = null;
		this.last = null;
		this.read(path, constructors);
		code = this.first;
		while (code) {
			code.parse(this);
			code = code.next;
		}
	}
	prepare(constructors) @ "xsopt_prepare";
	read(path, constructors) @ "xsopt_read";
	remove(code) {
		let previous = code.previous;
		let next = code.next;
		if (previous)
			previous.next = next;
		else
			this.first = next;
		if (next)
			next.previous = previous;
		else
			this.last = previous;
	}
	replace(code, by) {
		let previous = by.previous = code.previous;
		let next = by.next = code.next;
		if (previous)
			previous.next = by;
		else
			this.first = by;
		if (next)
			next.previous = by;
		else
			this.last = by;
	}
	run() {
		let code;
		this.parse(this.inputPath);
		if (this.optimization)
			this.optimize();
		this.serialize(this.outputPath);
	}
	serialize(path) {
		let code;
		this.codesDelta = 0;
		this.codesSize = 0;
		code = this.first;
		while (code) {
			code.serialize1(this);
			code = code.next;
		}
		this.codesSize = 0;
		code = this.first;
		while (code) {
			code.serialize2(this);
			code = code.next;
		}
		
		this.hostsCount = 0;
		this.hostsSize = 0;
		if (this.hosts) {
			this.hostsSize += 2;
			this.hosts.forEach(host => host.serialize(this));
		}
		
		this.keysCount = 0;
		this.keysSize = 2;
		this.keys.forEach(key => key.serialize(this));

		this.codesSize = 0;
		code = this.first;
		while (code) {
			code.serialize3(this);
			code = code.next;
		}
		
		if (this.verbose) {
			code = this.first;
			while (code) {
				code = code.report(this, 0);
			}
		}
		
		this.write(path);
	}
	write(path) @ "xsopt_write";

}



