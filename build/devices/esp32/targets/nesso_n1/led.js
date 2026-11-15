/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

import Expander from "NessoN1Expander";
import LED from "led/digital";

class LEDOutput {
	#expander;
	#io;

	constructor() {
		const expander = new Expander(1);
		try {
			this.#io = new expander.Digital({pin: 7, mode: expander.Digital.Output, initial: 1});
		}
		catch (error) {
			expander.close();
			throw error;
		}
		this.#expander = expander;
	}

	write(value) {
		this.#io.write(value);
	}

	close() {
		this.#io.close();
		this.#expander.close();
	}
}

export default class extends LED {
	constructor(options) {
		super({...options, io: LEDOutput, invert: true});
	}
}
