/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

import {
	Die,
	Transition,
} from "piu/MC";

const directions = {
	horizontal: false,
	vertical: true,
};
Object.freeze(directions);

export default class CombTransition extends Transition {
	constructor(duration, easing, direction, count) {
		super(duration);
		this.easing = easing;
		this.direction = (direction && (direction in directions)) ? directions[direction] : false;
		this.count = ((2 <= count) && (count <= 16)) ? count : 8;
	}
	onBegin(container, former, current) {
		container.add(current);
		this.container = container;
		this.die = new Die(null, {});
		this.die.attach(current);
		this.width = this.die.width;
		this.height = this.die.height;
		this.step = (this.direction) ? Math.ceil(this.width / this.count) : Math.ceil(this.height / this.count);
	}
	onEnd(container, former, current) {
		this.die.detach();
		container.remove(former);
	}
	onStep(fraction) {
		let easing = this.easing;
		if (easing)
			fraction = easing.call(Math, fraction);
		let c = this.count;
		let die = this.die;
		die.empty();
		if (this.direction) {
			let height = this.height;
			let dx = this.step;
			let x = 0;
			let dy = Math.round(height * fraction);
			let y = height - dy;
			for (let i = 0; i < c; i++) {
				if (i & 1) 
					die.or(x, 0, dx, dy);
				else
					die.or(x, y, dx, dy);
				x += dx;
			}
		}
		else {
			let width = this.width;
			let dx = Math.round(width * fraction);
			let x = width - dx;
			let dy = this.step;
			let y = 0;
			for (let i = 0; i < c; i++) {
				if (i & 1) 
					die.or(0, y, dx, dy);
				else
					die.or(x, y, dx, dy);
				y += dy;
			}
		}
		die.cut();
	}
}
Object.freeze(CombTransition.prototype);
