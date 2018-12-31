const unknown = {
	getValue() {
		return this;
	},
	toString() {
		return '?';
	}
};

class Local {
	constructor(key, value) {
		this.key = key;
		this.value = value;
	}
	getValue() {
		return this.value;
	}
	reportParam(tool) {
		if (this.key)
			return tool.keyString(this.key);
		return '?';
	}
	setValue(value) {
		this.value = value;
	}
}

class Closure extends Local {
	constructor(key, value) {
		super(key, value);
		this.reference = null;
	}
	getValue() {
		if (this.reference)
			return this.reference.getValue();
		return this.value;
	}
	reportParam(tool) {
		if (this.reference)
			return this.reference.reportParam(tool);
		if (this.key)
			return tool.keyString(this.key);
		return '?';
	}
	setValue(value) {
		if (this.reference)
			this.reference.setValue(value);
		else
			this.value = value;;
	}
}

class Transfer {
	constructor(key, _import, _from, _exports) {
		this.key = key;
		this.value = unknown;
		if (_from) {
			let module = tool.getModule(_from);
			if (module) {
				let key = tool.keyString(_import);
				if (key in module)
					this.value = module[key];
			}
		}
		this._import = _import;
		this._from = _from;
		this._exports = _exports;
	}
	getValue() {
		return this.value;
	}
	reportParam(tool) {
		return tool.keyString(this.key);
	}
	setValue(value) {
		this.value = value;;
	}
}

class Code {
	constructor(param) {
		this.param = param;
		this.previous = null;
		this.next = null;
		this.stackLevel = 0;
		this.scopeLevel = 0;
	}
	bind(tool) {
		return this.next;
	}
	evaluate(tool) {
		return this.bind(tool);
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
		result += " [";
		let stackLevel = this.stackLevel;
		if (stackLevel < 100)
			result += " ";
		if (stackLevel < 10)
			result += " ";
		result += stackLevel;
			result += " ";
		let scopeLevel = this.scopeLevel;
		if (scopeLevel < 100)
			result += " ";
		if (scopeLevel < 10)
			result += " ";
		result += scopeLevel;
		result += "] ";
		for (let tab = 0; tab < tabs; tab++)
			result += "    ";
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

class BinaryCode extends Code {
	bind(tool) {
		tool.popStack();
		return this.next;
	}
	compute(left, right) {
		return unknown;
	}
	evaluate(tool) {
		let right = tool.popStack();
		let left = tool.popStack();
		if ((left !== unknown) && (right !== unknown))
			tool.pushStack(this.compute(left, right));
		else
			tool.pushStack(unknown);
		return this.next;
	}
}

class UnaryCode extends Code {
	bind(tool) {
		return this.next;
	}
	compute(value) {
		return unknown;
	}
	evaluate(tool) {
		let value = tool.popStack();
		if (value !== unknown)
			tool.pushStack(this.compute(value));
		else
			tool.pushStack(unknown);
		return this.next;
	}
}

class UnknownCode extends Code {
	bind(tool) {
		tool.pushStack(unknown);
		return this.next;
	}
}

class CallCode extends UnknownCode {
	bind(tool) {
		tool.popStack();
		tool.popStack();
		let length = tool.popStack();
		while (length > 0) {
			tool.popStack();
			length--;
		}
		return super.bind(tool);
	}
}

class CodeCode extends BranchCode {
	bind(tool) {
		let formerFunction = tool.currentFunctiom;
		let formerFrameIndex = tool.frameIndex;
		let formerScopeIndex = tool.scopeIndex;
		let formerStackIndex = tool.stackIndex;
		
		tool.currentFunction = tool.getStack(0);
		tool.pushStack(undefined); // frame;
		tool.frameIndex = tool.stackIndex;
		tool.pushStack(undefined); // environment;
		tool.scopeIndex = tool.stackIndex;
		let code = this.next;
		let target = this.target;
		while (code != target) {
			code.stackLevel = formerStackIndex - tool.stackIndex;
			code.scopeLevel = formerStackIndex - tool.scopeIndex;
			code = code.bind(tool);
		}
		
		tool.stackIndex = formerStackIndex;
		tool.scopeIndex = formerScopeIndex;
		tool.frameIndex = formerFrameIndex;
		tool.currentFunctiom = formerFunction;
		
		return target;
	}
	evaluate(tool) {
		let formerFunction = tool.currentFunctiom;
		let formerFrameIndex = tool.frameIndex;
		let formerScopeIndex = tool.scopeIndex;
		let formerStackIndex = tool.stackIndex;
		
		tool.currentFunction = tool.getStack(0);
		tool.pushStack(undefined); // frame;
		tool.frameIndex = tool.stackIndex;
		tool.pushStack(undefined); // environment;
		tool.scopeIndex = tool.stackIndex;
		let code = this.next;
		let target = this.target;
		while (code != target) {
			code = code.evaluate(tool);
		}
		
		tool.stackIndex = formerStackIndex;
		tool.scopeIndex = formerScopeIndex;
		tool.frameIndex = formerFrameIndex;
		tool.currentFunctiom = formerFunction;
		
		return target;
	}
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
	reportParam(tool) {
		return tool.hosts[this.param].param;
	}
}

class KeyCode extends Code {
	reportParam(tool) {
		return tool.keyString(this.param);
	}
}

class FunctionCode extends KeyCode {
	bind(tool) {
		this.closures = [];
		tool.pushStack(this);
		return this.next;
	}
	evaluate(tool) {
		tool.pushStack(this);
		return this.next;
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

class ClosureCode extends StackCode {
	bind(tool) {
		this.closure = tool.getScope(this.param);
		return this.next;
	}
	evaluate(tool) {
		return this.next;
	}
	reportParam(tool) {
		let result = "" + this.param;
		if (this.closure && (this.closure instanceof Closure))
			result += " (" + this.closure.reportParam(tool) + ")";
		else
			result += " (@@@@)";
		return result;
	}
}

class LocalCode extends StackCode {
	bind(tool) {
		this.local = tool.getScope(this.param);
		return this.next;
	}
	evaluate(tool) {
		return this.next;
	}
	reportParam(tool) {
		let result = "" + this.param;
		if (this.local && (this.local instanceof Local))
			result += " (" + this.local.reportParam(tool) + ")";
		else
			result += " (@@@@)";
		return result;
	}
}

class StringCode extends Code {
	bind(tool) {
		tool.pushStack(this.param);
		return this.next;
	}
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
	}
}

const constructors = {
	ADD: class extends BinaryCode {
		compute(left, right) {
			return left + right;
		}
	},
	ARGUMENT: class extends UnknownCode {
	},
	ARGUMENTS: class extends UnknownCode {
	},
	ARGUMENTS_SLOPPY: class extends UnknownCode {
	},
	ARGUMENTS_STRICT: class extends UnknownCode {
	},
	ARRAY: class extends UnknownCode {
	},
	ASYNC_FUNCTION: class extends FunctionCode {
	},
	ASYNC_GENERATOR_FUNCTION: class extends FunctionCode {
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
	BIT_AND: class extends BinaryCode {
		compute(left, right) {
			return left & right;
		}
	},
	BIT_NOT: class extends UnaryCode {
		compute(value) {
			return ~value;
		}
	},
	BIT_OR: class extends BinaryCode {
		compute(left, right) {
			return left | right;
		}
	},
	BIT_XOR: class extends BinaryCode {
		compute(left, right) {
			return left ^ right;
		}
	},
	BRANCH: class extends BranchCode {
		optimize(tool) {
			if (this.target == this.next) {
				tool.count(0);
				tool.remove(this);
			}
			else if (this.target instanceof EndCode) {
				tool.count(0);
				tool.replace(this, new this.target.constructor());
			}
			return this.next;
		}
	},
	BRANCH_ELSE: class extends BranchCode {
		evaluate(tool) {
			this.value = tool.popStack();
			return this.next;
		}
		optimize(tool) {
			if (this.value !== unknown) {
				tool.count(1);
				let pop = new constructors.POP();
				tool.replace(this, pop);
				if (!this.value) {
					pop.next = this.target;
					return this.target;
				}
			}
			return this.next;
		}
		reportParam(tool) {
			let result = super.reportParam(tool);
			if (this.value !== unknown)
				result += " ***"
			return result;
		}
	},
	BRANCH_IF: class extends BranchCode {
		evaluate(tool) {
			this.value = tool.popStack();
			return this.next;
		}
		optimize(tool) {
			if (this.value !== unknown) {
				tool.count(2);
				let pop = new constructors.POP();
				tool.replace(this, pop);
				if (this.value) {
					pop.next = this.target;
					return this.target;
				}
			}
			return this.next;
		}
		reportParam(tool) {
			let result = super.reportParam(tool);
			if (this.value !== unknown)
				result += " " + this.value
			return result;
		}
	},
	BRANCH_STATUS: class extends BranchCode {
	},
	CALL: class extends CallCode {
	},
	CALL_TAIL: class extends CallCode {
	},
	CATCH: class extends BranchCode {
	},
	CHECK_INSTANCE: class extends Code {
	},
	CLASS: class extends Code {
		bind(tool) {
			tool.popStack();
			tool.popStack();
			tool.popStack();
			return this.next;
		}
	},
	CODE: class extends CodeCode {
	},
	CODE_ARCHIVE: class extends CodeCode {
	},
	CONST_CLOSURE: class extends ClosureCode {
		evaluate(tool) {
			this.closure.setValue(tool.getStack(0));
			return this.next;
		}
	},
	CONST_LOCAL: class extends LocalCode {
		evaluate(tool) {
			this.local.setValue(tool.getStack(0));
			return this.next;
		}
	},
	CONSTRUCTOR_FUNCTION: class extends FunctionCode {
	},
	CURRENT: class extends UnknownCode {
	},
	DEBUGGER: class extends Code {
	},
	DECREMENT: class extends UnaryCode {
		compute(value) {
			return value--;
		}
	},
	DELETE_PROPERTY: class extends KeyCode {
		evaluate(tool) {
			tool.setStack(0, unknown);
			return this.next;
		}
	},
	DELETE_PROPERTY_AT: class extends Code {
		bind(tool) {
			tool.popStack();
			return this.next;
		}
		evaluate(tool) {
			tool.popStack();
			tool.setStack(0, unknown);
			return this.next;
		}
	},
	DELETE_SUPER: class extends KeyCode {
		evaluate(tool) {
			tool.setStack(0, unknown);
			return this.next;
		}
	},
	DELETE_SUPER_AT: class extends Code {
		bind(tool) {
			tool.popStack();
			return this.next;
		}
		evaluate(tool) {
			tool.popStack();
			tool.setStack(0, unknown);
			return this.next;
		}
	},
	DIVIDE: class extends BinaryCode {
		compute(left, right) {
			return left / right;
		}
	},
	DUB: class extends Code {
		bind(tool) {
			tool.pushStack(tool.getStack(0));
			return this.next;
		}
	},
	DUB_AT: class extends Code {
		bind(tool) {
			tool.pushStack(tool.getStack(1));
			tool.pushStack(tool.getStack(0));
			return this.next;
		}
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
		bind(tool) {
			if (tool.getStack(0))
				tool.pushStack(tool.getStack(0).closures);
			else
				tool.pushStack(null);
			return this.next;
		}
	},
	EQUAL: class extends BinaryCode {
		compute(left, right) {
			return left == right;
		}
	},
	EVAL: class extends Code {
		bind(tool) {
			tool.setStack(0, unknown);
			return this.next;
		}
	},
	EVAL_ENVIRONMENT: class extends Code {
	},
	EVAL_REFERENCE: class extends KeyCode {
		bind(tool) {
			tool.pushStack(unknown);
			return this.next;
		}
	},
	EXCEPTION: class extends UnknownCode {
	},
	EXPONENTIATION: class extends BinaryCode {
		compute(left, right) {
			return left ** right;
		}
	},
	EXTEND: class extends UnknownCode {
	},
	FALSE: class extends Code {
		bind(tool) {
			tool.pushStack(false);
			return this.next;
		}
	},
	FILE: class extends KeyCode {
	},
	FOR_AWAIT_OF: class extends Code {
	},
	FOR_IN: class extends Code {
	},
	FOR_OF: class extends Code {
	},
	FUNCTION: class extends FunctionCode {
	},
	GENERATOR_FUNCTION: class extends FunctionCode {
	},
	GET_CLOSURE: class extends ClosureCode {
		bind(tool) {
			tool.pushStack(unknown);
			return super.bind(tool);
		}
		evaluate(tool) {
			tool.pushStack(this.closure.getValue());
			return super.evaluate(tool);
		}
	},
	GET_LOCAL: class extends LocalCode {
		bind(tool) {
			tool.pushStack(unknown);
			return super.bind(tool);
		}
		evaluate(tool) {
			tool.pushStack(this.local.getValue());
			return super.evaluate(tool);
		}
	},
	GET_PROPERTY: class extends KeyCode {
		evaluate(tool) {
			let value = tool.popStack();
			if (value !== unknown) {
				if (value instanceof Object) {
					if (Object.isFrozen(value)) {
						let key = tool.keyString(this.param);
						let descriptor = Object.getOwnPropertyDescriptor(value, key);
						if (descriptor && descriptor.value) {
							tool.pushStack(descriptor.value);
							return this.next;
						}
					}
				}
			}
			tool.pushStack(unknown);
			return this.next;
		}
	},
	GET_PROPERTY_AT: class extends Code {
		bind(tool) {
			let key = tool.popStack();
			let value = tool.popStack();
			if ((key !== unknown) && (value !== unknown)) {
				if ((typeof key == "string") && (value instanceof Object)) {
					if (Object.isFrozen(value)) {
						if (key in value) {
							tool.pushStack(value[key]);
							return this.next;
						}
					}
				}
			}
			tool.pushStack(unknown);
			return this.next;
		}
	},
	GET_SUPER: class extends KeyCode {
		evaluate(tool) {
			tool.setStack(0, unknown);
			return this.next;
		}
	},
	GET_SUPER_AT: class extends Code {
		bind(tool) {
			tool.popStack();
			tool.setStack(0, unknown);
			return this.next;
		}
	},
	GET_THIS: class extends Code {
		bind(tool) {
			tool.pushStack(unknown);
			return this.next;
		}
	},
	GET_VARIABLE: class extends KeyCode {
		evaluate(tool) {
			tool.setStack(0, unknown);
			return this.next;
		}
	},
	GLOBAL: class extends Code {
		bind(tool) {
			tool.pushStack(unknown);
			return this.next;
		}
	},
	HOST: class extends HostCode {
		bind(tool) {
			tool.pushStack(this.param);
			return this.next;
		}
	},
	IN: class extends BinaryCode {
		compute(left, right) {
			return left in right;
		}
	},
	INCREMENT: class extends UnaryCode {
		compute(value) {
			return value++;
		}
	},
	INSTANCEOF: class extends BinaryCode {
		compute(left, right) {
			return left instanceof right;
		}
	},
	INSTANTIATE: class extends Code {
		bind(tool) {
			tool.setStack(0, unknown);
			return this.next;
		}
	},
	INTEGER: class extends Code {
		bind(tool) {
			tool.pushStack(this.param);
			return this.next;
		}
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
	},
	INTRINSIC: class extends Code {
		bind(tool) {
			tool.pushStack(unknown);
			return this.next;
		}
	},
	LEFT_SHIFT: class extends BinaryCode {
		compute(left, right) {
			return left << right;
		}
	},
	LESS: class extends BinaryCode {
		compute(left, right) {
			return left < right;
		}
	},
	LESS_EQUAL: class extends BinaryCode {
		compute(left, right) {
			return left <= right;
		}
	},
	LET_CLOSURE: class extends ClosureCode {
	},
	LET_LOCAL: class extends LocalCode {
	},
	LINE: class extends Code {
	},
	MINUS: class extends UnaryCode {
		compute(value) {
			return -value;
		}
	},
	MODULE: class extends Code {
		bind(tool) {
			let length = tool.getStack(0);
			let closures = tool.getStack(length).closures;
			for (let index = 0; index < length - 1; index++) {
				let closure = closures[index];
				let transfer = tool.getStack(length - 1 - index);
				if (closure)
					closure.reference = transfer;
			}
			while (length > 0) {
				tool.popStack();
				length--;
			}
			return this.next;
		}
		evaluate(tool) {
			return this.next;
		}
	},
	MODULO: class extends BinaryCode {
		compute(left, right) {
			return left % right;
		}
	},
	MORE: class extends BinaryCode {
		compute(left, right) {
			return left > right;
		}
	},
	MORE_EQUAL: class extends BinaryCode {
		compute(left, right) {
			return left >= right;
		}
	},
	MULTIPLY: class extends BinaryCode {
		compute(left, right) {
			return left * right;
		}
	},
	NAME: class extends KeyCode {
	},
	NEW: class extends UnknownCode {
		bind(tool) {
			tool.popStack();
			let length = tool.popStack();
			while (length > 0) {
				tool.popStack();
				length--;
			}
			return super.bind(tool);
		}
	},
	NEW_CLOSURE: class extends KeyCode {
		bind(tool) {
			tool.scopeIndex--;
			this.closure = tool.stack[tool.scopeIndex] = new Closure(this.param, unknown);
			this.index = tool.frameIndex - tool.scopeIndex;
			return this.next;
		}
		evaluate(tool) {
			tool.scopeIndex--;
			tool.stack[tool.scopeIndex] = this.closure;
			return this.next;
		}
		reportParam(tool) {
			return super.reportParam(tool) + " (" + this.index + ")";
		}
	},
	NEW_LOCAL: class extends KeyCode {
		bind(tool) {
			tool.scopeIndex--;
			this.local = tool.stack[tool.scopeIndex] = new Local(this.param, unknown);
			this.index = tool.frameIndex - tool.scopeIndex;
			return this.next;
		}
		evaluate(tool) {
			tool.scopeIndex--;
			tool.stack[tool.scopeIndex] = this.local;
			return this.next;
		}
		reportParam(tool) {
			return super.reportParam(tool) + " (" + this.index + ")";
		}
	},
	NEW_PROPERTY: class extends Code {
		bind(tool) {
			tool.popStack();
			tool.popStack();
			tool.popStack();
			return this.next;
		}
	},
	NEW_TEMPORARY: class extends Code {
		bind(tool) {
			tool.scopeIndex--;
			tool.stack[tool.scopeIndex] = new Local(null, undefined);
			this.index = tool.frameIndex - tool.scopeIndex;
			return this.next;
		}
		reportParam(tool) {
			return "(" + this.index + ")";
		}
	},
	NOT: class extends UnaryCode {
		compute(value) {
			return !value;
		}
	},
	NOT_EQUAL: class extends BinaryCode {
		compute(left, right) {
			return left != right;
		}
	},
	NULL: class extends Code {
		bind(tool) {
			tool.pushStack(null);
			return this.next;
		}
	},
	NUMBER: class extends Code {
		bind(tool) {
			tool.pushStack(this.param);
			return this.next;
		}
	},
	OBJECT: class extends UnknownCode {
	},
	PLUS: class extends UnaryCode {
		compute(value) {
			return +value;
		}
	},
	POP: class extends Code {
		bind(tool) {
			tool.popStack();
			return this.next;
		}
	},
	PROGRAM_ENVIRONMENT: class extends Code {
	},
	PROGRAM_REFERENCE: class extends KeyCode {
		bind(tool) {
			tool.pushStack(unknown);
			return this.next;
		}
	},
	PULL_CLOSURE: class extends ClosureCode {
		bind(tool) {
			tool.popStack();
			return super.bind(tool);
		}
		evaluate(tool) {
			tool.popStack();
			return super.evaluate(tool);
		}
	},
	PULL_LOCAL: class extends LocalCode {
		bind(tool) {
			tool.popStack();
			return super.bind(tool);
		}
		evaluate(tool) {
			tool.popStack();
			return super.evaluate(tool);
		}
	},
	REFRESH_CLOSURE: class extends ClosureCode {
	},
	REFRESH_LOCAL: class extends LocalCode {
	},
	RESERVE: class extends StackCode {
		bind(tool) {
			let length = this.param;
			while (length > 0) {
				tool.pushStack(undefined);
				length--;
			}
			return this.next;
		}
	},
	RESET_CLOSURE: class extends ClosureCode {
	},
	RESET_LOCAL: class extends LocalCode {
	},
	RESULT: class extends Code {
		bind(tool) {
			tool.popStack();
			return this.next;
		}
	},
	RETHROW: class extends Code {
	},
	RETRIEVE: class extends StackCode {
		bind(tool) {
			let param = this.param;
			let closures = tool.currentFunction.closures; 
			closures.length = param;
			let index = 0;
			while (index < param) {
				tool.scopeIndex--;
				tool.stack[tool.scopeIndex] = closures[index] = new Closure(null, undefined);
				index++;
			}
			return this.next;
		}
		evaluate(tool) {
			let param = this.param;
			let closures = tool.currentFunction.closures; 
			closures.length = param;
			let index = 0;
			while (index < param) {
				tool.scopeIndex--;
				tool.stack[tool.scopeIndex] = closures[index];
				index++;
			}
			return this.next;
		}
	},
	RETRIEVE_TARGET: class extends Code {
	},
	RETRIEVE_THIS: class extends Code {
	},
	RETURN: class extends Code {
	},
	SET_CLOSURE: class extends ClosureCode {
	},
	SET_LOCAL: class extends LocalCode {
	},
	SET_PROPERTY: class extends KeyCode {
		bind(tool) {
			tool.popStack();
			return this.next;
		}
	},
	SET_PROPERTY_AT: class extends Code {
		bind(tool) {
			tool.popStack();
			tool.popStack();
			return this.next;
		}
	},
	SET_SUPER: class extends KeyCode {
		bind(tool) {
			tool.popStack();
			return this.next;
		}
	},
	SET_SUPER_AT: class extends Code {
		bind(tool) {
			tool.popStack();
			tool.popStack();
			return this.next;
		}
	},
	SET_THIS: class extends Code {
	},
	SET_VARIABLE: class extends KeyCode {
		bind(tool) {
			tool.popStack();
			return this.next;
		}
	},
	SIGNED_RIGHT_SHIFT: class extends BinaryCode {
		compute(left, right) {
			return left >> right;
		}
	},
	START_ASYNC: class extends Code {
	},
	START_ASYNC_GENERATOR: class extends Code {
	},
	START_GENERATOR: class extends Code {
	},
	STORE: class extends ClosureCode {
		bind(tool) {
			let closures = tool.getStack(0);
			if (closures) {
				let closure = tool.getScope(this.param);
				let index = 0;
				while (closures[index].reference)
					index++;
				closures[index].reference = closure;
				this.closure = closure;
			}
			return this.next;
		}
	},
	STORE_ARROW: class extends Code {
	},
	STRICT_EQUAL: class extends BinaryCode {
		compute(left, right) {
			return left === right;
		}
	},
	STRICT_NOT_EQUAL: class extends BinaryCode {
		compute(left, right) {
			return left !== right;
		}
	},
	STRING: class extends StringCode {
	},
	STRING_ARCHIVE: class extends StringCode {
	},
	SUBTRACT: class extends BinaryCode {
		compute(left, right) {
			return left - right;
		}
	},
	SUPER: class extends Code {
	},
	SWAP: class extends Code {
		bind(tool) {
			let right = tool.popStack();
			let left = tool.popStack();
			tool.pushStack(right);
			tool.pushStack(left);
		}
	},
	SYMBOL: class extends KeyCode {
		bind(tool) {
			tool.pushStack(this.param);
			return this.next;
		}
	},
	TARGET: class extends UnknownCode {
	},
	TEMPLATE: class extends Code {
	},
	THIS: class extends UnknownCode {
	},
	THROW: class extends Code {
		bind(tool) {
			tool.popStack();
			return this.next;
		}
	},
	THROW_STATUS: class extends Code {
	},
	TO_INSTANCE: class extends Code {
	},
	TRANSFER: class extends Code {
		bind(tool) {
			let length = tool.popStack();
			let _exports = [];
			while (length > 3) {
				_exports.push(tool.popStack());
				length--;
			}
			let _import = tool.popStack();
			let _from = tool.popStack();
			let key = tool.popStack();
			let transfer = new Transfer(key, _import, _from, _exports);
			tool.pushStack(transfer);
			return this.next;
		}
	},
	TRUE: class extends Code {
		bind(tool) {
			tool.pushStack(true);
			return this.next;
		}
	},
	TYPEOF: class extends Code {
	},
	UNCATCH: class extends Code {
	},
	UNDEFINED: class extends Code {
		bind(tool) {
			tool.pushStack(undefined);
			return this.next;
		}
	},
	UNSIGNED_RIGHT_SHIFT: class extends BinaryCode {
		compute(left, right) {
			return left >>> right;
		}
	},
	UNWIND: class extends StackCode {
	},
	VAR_CLOSURE: class extends ClosureCode {
	},
	VAR_LOCAL: class extends LocalCode {
	},
	VOID: class extends UnaryCode {
		compute(value) {
			return undefined;
		}
	},
	WITH: class extends Code {
	},
	WITHOUT: class extends Code {
	},
	YIELD: class extends Code {
	},
	Host,
}

class Tool {
	constructor() {
		this.optimizeBranch = 0;
		this.optimizeBranchIf = 0;
		this.optimizeBranchElse = 0;
		this.verbose = false;
		this.prepare(constructors);
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
	bind() {
		this.stack = new Array(1024).fill();
		this.frameIndex = this.scopeIndex = this.stackIndex = 1024;
		let code = this.first;
		while (code) {
			code.stackLevel = 1024 - this.stackIndex;
			code.scopeLevel = 1024 - this.scopeIndex;
			code = code.bind(this);
		}
	}
	evaluate() {
		this.stack = new Array(1024).fill();
		this.frameIndex = this.scopeIndex = this.stackIndex = 1024;
		let code = this.first;
		while (code) {
			code = code.evaluate(this);
		}
	}
	keyString(key) @ "Tool_prototype_keyString";
	getModule(_from) @ "Tool_prototype_getModule";
	getPath() @ "Tool_prototype_getPath";
	getScope(at) {
		return this.stack[this.frameIndex - at];
	}
	getStack(at) {
		return this.stack[this.stackIndex + at];
	}
	optimize() {
		let code = this.first;
		while (code) {
			code = code.optimize(this);
		}
	}
	parse(path) {
		this.hosts = null;
		this.first = null;
		this.last = null;
		this.read(path, constructors);
		let code = this.first;
		while (code) {
			code.parse(this);
			code = code.next;
		}
	}
	prepare(constructors) @ "Tool_prototype_prepare";
	
	popStack() {
		let result = this.stack[this.stackIndex];
		this.stackIndex++;
		return result;
	}
	pushStack(slot) {
		this.stackIndex--;
		this.stack[this.stackIndex] = slot;
	}
	read(path, constructors) @ "Tool_prototype_read";
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
		by.stackLevel = code.stackLevel;
		by.scopeLevel = code.scopeLevel;
	}	
	report(s) @ "Tool_prototype_report"
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
	setScope(at, slot) {
		this.stack[this.frameIndex - at] = slot;
	}
	setStack(at, slot) {
		this.stack[this.stackIndex + at] = slot;
	}
	strlen(string) @ "Tool_prototype_strlen";
	write(path) @ "Tool_prototype_write";
	count(which) @ "Tool_prototype_count";
}

let tool = new Tool();
tool.parse();
tool.bind();
tool.evaluate();
tool.optimize();
tool.serialize();

let prototype;