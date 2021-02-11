/*
 * Copyright (c) 2019-2021  Moddable Tech, Inc.
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

/*
	16 pin GPIO expander MCP23017

	to do:
		rising OR falling interrupts
		track in use pins... single instance
*/

class ExpanderI2C {
	#io;

	constructor(options) {
		this.#io =  new (options.io)(options);
	}
	close() {
		this.#io.close?.();
		this.#io = undefined;
	}
	write(...args) {
		this.#io.write(Uint8Array.from(args));
	}
	read(count) {
		return new Uint8Array(this.#io.read(count));
	}
}

class Expander {
	#i2c;

	constructor(options) {
		let o = options.i2c;
		const i2c = this.#i2c = new ExpanderI2C({
			io: o.io,
			data: o.data,
			clock: o.clock,
			hz: o.hz ?? 1_000_000,
			address: o.address ?? 0x20
		});

		i2c.inputs = 0b1111111111111111;		// set bit to indicate pin is an input
		i2c.write(0x00, 0b11111111, 0b11111111);

		i2c.pullups = 0b0000000000000000;		// set bit to indicate pin is pulled up
		i2c.write(0x06, 0b00000000, 0b00000000);

		i2c.output = 0b0000000000000000;		// each bit represents the last set value of an output
		i2c.write(0x12, 0b00000000, 0b00000000);

		i2c.readers = [];
		if (options.interrupt) {
			o = options.interrupt;
			i2c.interrupt = new (o.io)({
				pin: o.pin,
				mode: o.io.InputPullUp,
				edge: o.io.Rising | o.io.Falling,
				onReadable() {
					const i2c = this.i2c;
					i2c.write(0x0E);		// INTF
					let pins = i2c.read(2);
					pins = pins[0] | (pins[1] << 8);
					for (let i = 0, readers = i2c.readers, length = readers.length; i < length; i++) {
						if (readers[i].pins & pins)
							readers[i].onReadable(readers[i].pins & pins);
					}
				}
			});
			i2c.interrupt.i2c = i2c;

			i2c.write(0x0A);		// IOCON
			let value = i2c.read(2);
			value = value[0] | (value[1] << 8)
			value = (value & ~0x4646) | 0x40 | 0 | 0 | 0x4000 | 0 | 0;	// mirroring ON, open drain OFF, polarity active low
			i2c.write(0x0A, value & 255, value >> 8);

			i2c.write(0x08, 0, 0);		// INTCON
			i2c.write(0x06, 0, 0);		// DEFVAL
		}
		i2c.write(0x04, 0, 0);		// GPINTEN

		this.Digital = class extends Digital {
			constructor(options) {
				super(options, i2c, true);
			}
		}
		this.DigitalBank = class extends Digital {
			constructor(options) {
				super(options, i2c);
			}
		}
	}
	close() {
		this.#i2c.close();
		this.#i2c.interrupt.close?.();
		this.#i2c = undefined;
		delete this.Digital;
		delete this.DigitalBank;
	}
}

class Digital {
	constructor(options, i2c, flag) {
		if (flag) {
			options = {...options, pins: 1 << options.pin, i2c};
			if ((options.pin < 0) || (options.pin > 15))
				throw new RangeError("invalid pin");
			delete options.pin;
			if (options.edge) {
				if (Digital.Rising & options.edge)
					options.rises = options.pins;
				if (Digital.Falling & options.edge)
					options.falls = options.pins;
				delete options.edge;
			}
		}
		else {
			options = {...options, i2c};
			if (!options.pins)
				throw new RangeError("invalid pins");
		}

		switch (options.mode) {
			case Digital.Input:
			case Digital.InputPullUp:
				return flag ? new Input(options) : new InputBank(options);
			case Digital.Output:
				return flag ? new Output(options) : new OutputBank(options);
			default:
				throw new RangeError("invalid mode");
		}
	}
}

class IO {
	close() {
		//@@ anything to do here?
	}
	get format() {
		return "number";
	}
	set format(value) {
		if ("number" !== value)
			throw new Error;
	}
}

class InputBank extends IO {
	#i2c;
	#pins;
	constructor(options) {
		super(options);
		const i2c = options.i2c;
		const pins = options.pins;

		if (undefined !== options.target)
			this.target = options.target;

		this.#pins = pins;
		this.#i2c = i2c;

		i2c.pullups = (Digital.InputPullUp === options.mode) ? (i2c.pullups | pins) : (i2c.pullups & ~pins);
		i2c.write(0x06, i2c.pullups & 255, i2c.pullups >> 8);

		i2c.inputs |= pins;
		i2c.write(0x00, i2c.inputs & 255, i2c.inputs >> 8);

		if (options.rises || options.falls) {
			const onReadable = options.onReadable;
			if (!onReadable)
				throw new Error("onReadable required");

			this.#interrupt(~0);
			i2c.readers.push({
				onReadable,
				pins
			});
		}
	}
	close() {
		for (let i = 0, readers = this.#i2c.readers, length = readers.length; i < length; i++) {
			if (readers[i].pins === this.#pins) {	// requires pins no re-used across instances
				readers.splice(i, 1);
				this.#interrupt(0);
				break;
			}
		}
		super.close();
	}
	read() {
		this.#i2c.write(0x12);
		const result = this.#i2c.read(2);
		return (result[0] | (result[1] << 8)) & this.#pins;
	}
	#interrupt(enable) {
		const i2c = this.#i2c;
		i2c.write(0x04);		// read GPINTEN
		let value = i2c.read(2);
		value = (value[0] | (value[1] << 8)) & ~this.#pins;
		value |= enable & this.#pins;
		i2c.write(0x04, value & 255, value >> 8);		// write GPINTEN
	}
}

class Input extends InputBank {
	read() {
		return super.read() ? 1 : 0;
	}
}

class OutputBank extends IO {
	#i2c;
	#pins;
	constructor(options) {
		super(options);
		const i2c = options.i2c;
		const pins = options.pins;

		if (undefined !== options.target)
			this.target = options.target;

		this.#pins = pins;
		this.#i2c = i2c;

		i2c.inputs &= ~pins;
		i2c.write(0, i2c.inputs & 255, i2c.inputs >> 8);
	}
	write(value) {
		const i2c = this.#i2c;
		const output = (i2c.output & ~this.#pins) | (value & this.#pins);
		i2c.output = output;

		i2c.write(0x12, output & 255, output >> 8)
	}
}

class Output extends OutputBank {
	write(value) {
		super.write(value ? ~0 : 0);
	}
}

Digital.Input = 0;
Digital.InputPullUp = 1;

Digital.Output = 8;

Digital.Rising = 1;
Digital.Falling = 2;

export default Expander;
