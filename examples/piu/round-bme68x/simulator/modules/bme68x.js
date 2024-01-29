/*
 * Copyright (c) 2023-2024 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import structuredClone from "structuredClone";
import Control from "control";
export default class extends Control {
	#value;
	constructor(options) {
		super(options);
		this.#value = {
		   thermometer: {
			  temperature: 33.68404006958008
		   },
		   hygrometer: {
			  humidity: 0.268742733001709
		   },
		   barometer: {
			  pressure: 100989.90625
		   }
		};	
	}
	onJSON(json) {
		const bme68x = json.bme68x;
		if (bme68x) {
			this.#value = bme68x;
		}
	}
	sample() {
		return structuredClone(this.#value);
	}
};
