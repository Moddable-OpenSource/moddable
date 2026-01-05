/*
 * Copyright (c) 2024-2025 Moddable Tech, Inc.
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

class JSONBase64Parser extends Native("JSONBase64Parser_destructor") {
	constructor(target, data, step, minimum) { super(); native("JSONBase64Parser_constructor").call(this, target, data, step, minimum); }
	read(buffer) { return native("JSONBase64Parser_read").call(this, buffer); }
	reset() { return native("JSONBase64Parser_reset").call(this); }
	
	push(current) {
		if (this.stack)
			this.stack.push(this.current);
		else
			this.stack = [ this.current ];
		this.current = current;
	}
	pop() {
		this.current = this.stack.pop();
	}
	setName(name) {
		this.name = name;
	}
	setValue(value) {
		const current = this.current;
		if (current === undefined)
			this.current = this.result = value;
		else if (Array.isArray(current))
			current.push(value);
		else {
			current[this.name] = value;
			this.name = undefined;
		}
	}
	test() {
		const current = this.current;
		if (current === undefined)
			return 0;
		if (Array.isArray(current))
			return 1;
		return -1;
	}
	wait(head, size, space) {
		let tail = Atomics.load(this.barrier, 0);
		while (((tail - (head + 1)) & (size - 1)) < space) {
// 			trace(`WAIT ${ tail } ${ (tail - (head + 1)) & (size - 1) } ${ space } \n`);
			let result = Atomics.wait(this.barrier, 0, tail);
// 			trace(`-- ${ result } \n`);
			tail = Atomics.load(this.barrier, 0);
		}
	}
	
	// separate binary data
	copy(data) { return native("JSONBase64Parser_copy").call(this, data); }
	done() { return native("JSONBase64Parser_done").call(this); }
}

export default JSONBase64Parser