/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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

import Control from "control";
export default class extends Control {
	#onPush;
	#onTurn;
	#onPushAndTurn;
	#target;
	#value;
	constructor(options) {
		super(options);
		this.#onPush = options.onPush ?? this.onPush;
		this.#onTurn = options.onTurn ?? this.onTurn;
		this.#onPushAndTurn = options.onPushAndTurn ?? this.onPushAndTurn ?? this.#onTurn;
		this.#target = options.target ?? this;
	}
	onJSON(json) {
		const jogdial = json.jogdial;
		if (jogdial) {
			if (jogdial.turn) {
				if (jogdial.push)
					this.#onPushAndTurn.call(this.#target, jogdial.turn);
				else
					this.#onTurn.call(this.#target, jogdial.turn);
			}
			else if (jogdial.push == 1) {
				this.#onPush.call(this.#target, 1);
			}
			else if (jogdial.push == 0) {
				this.#onPush.call(this.#target, 0);
			}
			
		}
	}
};
