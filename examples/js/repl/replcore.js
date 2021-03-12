/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

const backspace = String.fromCharCode(8);
const newline = "\n";

const primitives = Object.freeze(["Number", "Boolean", "BigInt", "String"]);

class REPL @ "xs_repl_destructor" {
	static receive() @ "xs_repl_receive"
	static write() @ "xs_repl_write"
	static eval() @ "xs_repl_eval"
	static get xsVersion() @ "xs_repl_get_xsVersion"

	constructor() {
		REPL.write(newline, "Moddable REPL v0.0.2", newline);
		REPL.write(`  XS engine v${REPL.xsVersion.join(".")}`, newline, newline);
		//@@ XS version
		//@@ build date
		this.history = [];
		this.prompt();
	}
	prompt() {
		delete this.historyPosition;
		delete this.postHistory;
		this.position = 0;
		this.escapeState = 0;
		this.incoming = "";
		REPL.write("> ");
	}
	poll() {
		do {
			let c = REPL.receive();
			if (undefined === c)
				return;

			if (this.escapeState) {
				if (1 === this.escapeState) {
					if (91 === c) {
						this.escapeState = 2;
						continue;
					}
				}

				this.escapeState = 0;

				if (((65 === c) || (66 === c)) && this.history.length)  {		// up / down arrow
					if (undefined === this.historyPosition) {
						this.historyPosition = 0;
						this.postHistory = this.incoming;
					}
					let incoming = this.incoming;
					this.historyPosition += (65 === c) ? +1 : -1;
					if (this.historyPosition < 0)
						this.historyPosition = 0;
					else if (this.historyPosition > this.history.length)
						this.historyPosition = this.history.length;
					if (0 === this.historyPosition)
						this.incoming = this.postHistory;
					else
						this.incoming = this.history[this.history.length - this.historyPosition];
					REPL.write(backspace.repeat(incoming.length), this.incoming);
					if (incoming.length > this.incoming.length) {
						const delta = incoming.length - this.incoming.length;
						REPL.write(" ".repeat(delta),
								   backspace.repeat(delta));
					}
					this.position = this.incoming.length;
				}
				else
				if ((68 === c) && this.incoming) {		// left arrow
					if (this.position > 0) {
						this.position -= 1;
						REPL.write(backspace);
					}
				}
				else if ((67 === c) && (this.position < this.incoming.length)) {		// right arrow
					REPL.write(this.incoming[this.position]);
					this.position += 1;
				}
				continue;
			}

			if (c < 32) {
				if ((10 === c) || (13 === c))
					break;

				if (3 === c) {
					REPL.write("^C", newline);
					this.prompt();
				}

				if (27 === c)
					this.escapeState = 1;
				else if (8 === c)
					this.delete();	// backspace
				continue;
			}

			if (127 === c) {			// delete
				this.delete();
				continue;
			}

			// insert
			c = String.fromCharCode(c);
			REPL.write(c);
			if (this.position === this.incoming.length)
				this.incoming += c;
			else {
				REPL.write(this.incoming.slice(this.position), backspace.repeat(this.incoming.length - this.position));
				this.incoming = this.incoming.slice(0, this.position) + c + this.incoming.slice(this.position);
			}
			this.position += 1;
		} while (true);

		REPL.write(newline);
		if (this.incoming) {
			this.history.push(this.incoming);
			while (this.history.length > 20)
				this.history.shift();

			try {
				const result = REPL.eval(this.incoming);
				this.print(result);
			}
			catch (e) {
				REPL.write(e.toString(), newline);
			}
		}
		this.prompt();
	}
	delete() {
		if (this.incoming.length && this.position) {
			if (this.position === this.incoming.length) {
				REPL.write(backspace, " ", backspace);
				this.incoming = this.incoming.slice(0, -1);
			}
			else {
				let start = this.incoming.slice(0, this.position - 1);
				let end = this.incoming.slice(this.position);
				this.incoming = start + end
				REPL.write(backspace, end, " ", backspace.repeat(end.length + 1));
			}
			this.position -= 1;
		}
	}
	short(value) {
		if (null === value)
			return "null";

		switch (typeof value) {
			case "undefined":
				return "undefined";

			case "boolean":
			case "number":
			case "symbol":
				return value.toString();
				break;

			case "string":
				return '"' + value.toString() + '"';

			case "bigint":
				return value.toString() + "n";

			case "function": {
				const body = /* value.toString().includes("[native code]") ? "{ [native code] }" : */ "{}";
				const args = [];
				for (let i = 0; i < value.length; i++)
					args.push(String.fromCharCode(97 + i));
				return `${value.prototype?.constructor ? "class" : "ƒ"} ${value.name} (${args.join(", ")}) ${body}`;
				}

			case "object":
				let type = Object.prototype.toString.call(value).slice(8, -1);
				if ("RegExp" === type)
					return value.toString();
				return type;
		}
	}
	print(value) {
		if ((("object" !== typeof value) && ("function" !== typeof value)) || (null === value)) {
			REPL.write(this.short(value) , newline);
			return;
		}

		let type = Object.prototype.toString.call(value).slice(8, -1);
		if (primitives.includes(type))
			type = "Object";

		switch (type) {
			case "RegExp":
				REPL.write(value.toString(), newline);
				break;

			case "Error":
				REPL.write(value.stack.toString(), newline);
				break;

			case "Array":
			case "Atomics":
			case "Math":
			case "Object":
			case "Reflect":
			case "global":
				REPL.write(type, newline);
				const keys = Object.getOwnPropertyNames(value);
				for (let key of keys)
					REPL.write("  ", key, ": ", this.short(value[key]), newline);
				break;

			case "Set":
			case "Int8Array":
			case "Uint8Array":
			case "Uint8ClampedArray":
			case "Int16Array":
			case "Uint16Array":
			case "Int32Array":
			case "Uint32Array":
			case "Int64Array":
			case "Uint64Array":
			case "Float32Array":
			case "Float64Array":
			case "BigInt64Array":
			case "BigUint64Array":
				REPL.write(type, newline);
				for (let v of value)
					REPL.write("  ", this.short(v), newline);
				break;

			case "Map":
				REPL.write(type, newline);
				for (let [k, v] of value)
					REPL.write("  ", this.short(k), ": ", this.short(v), newline);
				break;

			case "ArrayBuffer":
			case "SharedArrayBuffer": {
				REPL.write(type, ", byteLength ", value.byteLength, newline);
				for (let v of (new Uint8Array(value)))
					REPL.write("  ", this.short(v), newline);
				} break;

			case "WeakMap":
			case "WeakRef":
			case "WeakSet":
				REPL.write(type, newline);
				break;

			case "Function":
				if (!value.prototype?.constructor)
					REPL.write(this.short(value) , newline);
				else {
					REPL.write("class ", value.name, " {", newline);

					let keys = Object.getOwnPropertyNames(value);
					for (let key of keys) {
						if (("length" === key) || ("name" === key) || ("prototype" === key))
							continue;

						let v = value[key];
						if ("function" === typeof v)
							REPL.write("  ", `static ƒ ${key}() {}`, newline);
						else
							REPL.write("  ", "static ", key, ": ", this.short(v), newline);
					}

					keys = Object.getOwnPropertyNames(value.prototype);
					for (let key of keys) {
						let v = value.prototype[key];
						if ("function" === typeof v)
							REPL.write("  ", `ƒ ${key}() {}`, newline);
						else
							REPL.write("  ", key, ": ", this.short(v), newline);
					}

					REPL.write("}", newline);
				}
				break;;

			default:
				REPL.write(type, ": ", value.toString(), newline);
				break;
		}
	}
}
Object.freeze(REPL.prototype);

global.console = {
	log: function (...str) {
		REPL.write(...str, newline);
	}
}
Object.freeze(global.console);

export default REPL;
