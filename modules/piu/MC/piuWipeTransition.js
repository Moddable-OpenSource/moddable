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

const wipeCenter = 0;
const wipeLeft = 1;
const wipeRight = 2;
const horizontals = {
	center: wipeCenter,
	left: wipeLeft,
	right: wipeRight,
};
Object.freeze(horizontals);
const wipeMiddle = 0;
const wipeTop = 1;
const wipeBottom = 2;
const verticals = {
	middle: wipeMiddle,
	top: wipeTop,
	bottom: wipeBottom,
};
Object.freeze(verticals);
const wipeNone = 3;

export default class WipeTransition extends Transition {
	constructor(duration, easing, first, last) {
		super(duration);
		this.easing = easing;
		this.horizontal = wipeNone;
		this.vertical = wipeNone;
		if (first) {
			if (first in horizontals)
				this.horizontal = horizontals[first];
			else if (first in verticals)
				this.vertical = verticals[first];
		}
		if (last) {
			if (last in horizontals)
				this.horizontal = horizontals[last];
			else if (last in verticals)
				this.vertical = verticals[last];
		}
	}
	onBegin(container, former, current) {
		container.add(current);
		this.container = container;
		this.die = new Die(null, {});
		this.die.attach(current);
		this.width = this.die.width;
		this.height = this.die.height;
	}
	onEnd(container, former, current) {
		this.die.detach();
		container.remove(former);
		delete this.die;
		delete this.container;
	}
	onStep(fraction) {
		let easing = this.easing;
		if (easing)
			fraction = easing.call(Math, fraction);
		let left, right, top, bottom;
		switch (this.horizontal) {
		case wipeCenter:
			left = Math.round((this.width >> 1) * (1 - fraction));
			right = this.width - left;
			break;
		case wipeLeft:
			left = 0;
			right = Math.round(this.width * fraction);
			break;
		case wipeRight:
			left = Math.round(this.width * (1 - fraction));
			right = this.width;
			break;
		default:
			left = 0;
			right = this.width;
			break;
		}
		switch (this.vertical) {
		case wipeMiddle:
			top = Math.round((this.height >> 1) * (1 - fraction));
			bottom = this.height - top;
			break;
		case wipeTop:
			top = 0;
			bottom = Math.round(this.height * fraction);
			break;
		case wipeBottom:
			top = Math.round(this.height * (1 - fraction));
			bottom = this.height;
			break;
		default:
			top = 0;
			bottom = this.height;
			break;
		}
		this.die.set(left, top, right - left, bottom - top);
		this.die.cut();
	}
}
Object.freeze(WipeTransition.prototype);
