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

import CLI from "cli";
import Modules from "modules";

const backspace = String.fromCharCode(8);
const newline = "\r\n";

const primitives = Object.freeze(["Number", "Boolean", "BigInt", "String"]);

class REPL @ "xs_repl_destructor" {
	static eval() @ "xs_repl_eval"
	static get xsVersion() @ "xs_repl_get_xsVersion"

	constructor() {
		this.history = [];
		globalThis._ = undefined;
		globalThis.Modules = Modules;
		globalThis.require = Modules.importNow;

		globalThis.console = {
			log: (...str) => this.write(...str, newline)
		};
		globalThis.trace = (...str) => this.write(...str);
	}
	ready() {
		this.write(newline, "Moddable JavaScript REPL v0.0.5", newline);
		this.write(`  XS engine v${REPL.xsVersion.join(".")}`, newline);
		this.write(`  ? for help`, newline);
		this.write(newline);

		const archive = Modules.archive;
		if (archive.includes("setup"))
			Modules.importNow("setup");

		for (let i = 0; i < archive.length; i++) {
			if (archive[i].startsWith("cli/"))
				Modules.importNow(archive[i]);
		}

		this.prompt();
	}
	receive() @ "xs_repl_receive"
	write() @ "xs_repl_write"
	suspend(cancel) {
		this.suspended = cancel ?? true;
	}
	resume() {
		if (!this.suspended) return;

		delete this.suspended;
		this.prompt();
	}
	prompt() {
		delete this.historyPosition;
		delete this.postHistory;
		this.position = 0;
		this.escapeState = 0;
		this.incoming = "";
		if (!this.suspended)
			this.write("> ");
	}
	poll() {
		do {
			let c = this.receive();
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
					this.write(backspace.repeat(incoming.length), this.incoming);
					if (incoming.length > this.incoming.length) {
						const delta = incoming.length - this.incoming.length;
						this.write(" ".repeat(delta),
								   backspace.repeat(delta));
					}
					this.position = this.incoming.length;
				}
				else
				if ((68 === c) && this.incoming) {		// left arrow
					if (this.position > 0) {
						this.position -= 1;
						this.write(backspace);
					}
				}
				else if ((67 === c) && (this.position < this.incoming.length)) {		// right arrow
					this.write(this.incoming[this.position]);
					this.position += 1;
				}
				continue;
			}

			if (this.suspended) {
				if (3 === c) {
					if (true !== this.suspended)
						this.suspended();
					delete this.suspended;
					this.incoming = "";
					this.write("^C", newline);
					this.prompt();
				}
				continue;
			}

			if (c < 32) {
				if ((10 === c) || (13 === c))
					break;

				if (3 === c) {
					this.write("^C", newline);
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
			this.write(c);
			if (this.position === this.incoming.length)
				this.incoming += c;
			else {
				this.write(this.incoming.slice(this.position), backspace.repeat(this.incoming.length - this.position));
				this.incoming = this.incoming.slice(0, this.position) + c + this.incoming.slice(this.position);
			}
			this.position += 1;
		} while (true);

		this.write(newline);
		if (this.incoming) {
			this.history.push(this.incoming);
			while (this.history.length > 20)
				this.history.shift();

			try {
				if (this.incoming.startsWith(".")) {
					CLI.execute.call(this, this.incoming.slice(1));
				}
				else if (this.incoming.startsWith("?")) {
					this.line(`  "globalThis" lists JavaScript global variables`);
					this.line(`  lines that start with "." are commands`);
					this.line(`  ".help" lists all commands`);
					this.line(`  "Modules.host" lists built-in modules`);
					this.line(`  "Modules.archive" lists modules from mod`);
					this.line(`  global "_" is result of last REPL JavaScript operation`);
					this.line(`  lines that start with "?" display this help message`);
				}
				else {
					globalThis._ = REPL.eval(this.incoming);
					this.print(_);
				}
			}
			catch (e) {
				this.write(e.toString(), newline);
			}
		}

		this.prompt();
	}
	delete() {
		if (this.incoming.length && this.position) {
			if (this.position === this.incoming.length) {
				this.write(backspace, " ", backspace);
				this.incoming = this.incoming.slice(0, -1);
			}
			else {
				const start = this.incoming.slice(0, this.position - 1);
				const end = this.incoming.slice(this.position);
				this.incoming = start + end
				this.write(backspace, end, " ", backspace.repeat(end.length + 1));
			}
			this.position -= 1;
		}
	}
	line(...args) {
		this.write(...args, newline);
	}
	short(value) {
		if (null === value)
			return "null";

		let type = typeof value;
		if (globalThis === value)
			return "globals";

		switch (type) {
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
				return `${value.prototype?.constructor ? "class" : "ƒ"} ${value.name ?? ""} (${args.join(", ")}) ${body}`;
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
			this.write(this.short(value) , newline);
			return;
		}

		let type = Object.prototype.toString.call(value).slice(8, -1);
		if (primitives.includes(type))
			type = "Object";

		if (value === globalThis)
			type = "global";

		let keys;
		switch (type) {
			case "RegExp":
				this.write(value.toString(), newline);
				break;

			case "Error":
				this.write(value.stack.toString(), newline);
				break;

			case "Array":
			case "Atomics":
			case "Math":
			case "Object":
			case "Reflect":
				this.write(type, newline);
				keys = Object.getOwnPropertyNames(value);
				for (let key of keys)
					this.write("  ", key, ": ", this.short(value[key]), newline);
				break;

			case "global":
				this.write(type, newline);
				keys = Object.getOwnPropertyNames(value).concat(Object.getOwnPropertyNames(Object.getPrototypeOf(value))).sort();
				for (let key of keys)
					this.write("  ", key, ": ", this.short(value[key]), newline);
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
				this.write(type, newline);
				for (let v of value)
					this.write("  ", this.short(v), newline);
				break;

			case "Map":
				this.write(type, newline);
				for (let [k, v] of value)
					this.write("  ", this.short(k), ": ", this.short(v), newline);
				break;

			case "ArrayBuffer":
			case "SharedArrayBuffer": {
				this.write(type, ", byteLength ", value.byteLength, newline);
				for (let v of (new Uint8Array(value)))
					this.write("  ", this.short(v), newline);
				} break;

			case "Module":
			case "WeakMap":
			case "WeakRef":
			case "WeakSet":
				this.write(type, newline);
				break;

			case "Function":
				if (!value.prototype?.constructor)
					this.write(this.short(value) , newline);
				else {
					this.write("class", " " + (value.name ?? ""), " {", newline);

					let keys = Object.getOwnPropertyNames(value);
					for (let key of keys) {
						if (("length" === key) || ("name" === key) || ("prototype" === key))
							continue;

						let v = value[key];
						if ("function" === typeof v)
							this.write("  ", `static ƒ ${key}() {}`, newline);
						else
							this.write("  ", "static ", key, ": ", this.short(v), newline);
					}

					keys = Object.getOwnPropertyNames(value.prototype);
					for (let key of keys) {
						let d = Object.getOwnPropertyDescriptor(value.prototype, key);
						if (d.get || d.set) {
							if (d.get)
								this.write("  ", `ƒ get ${key}() {}`, newline);
							if (d.set)
								this.write("  ", `ƒ set ${key}() {}`, newline);
						}
						else {
							let v  = d.value;
							if ("function" === typeof v)
								this.write("  ", `ƒ ${key}() {}`, newline);
							else
								this.write("  ", key, ": ", this.short(v), newline);
						}
					}

					this.write("}", newline);
				}
				break;

			default:
				this.write(type, ": ", value.toString(), newline);
				break;
		}
	}
}

export default REPL;
