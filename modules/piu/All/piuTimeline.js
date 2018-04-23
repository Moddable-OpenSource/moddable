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

class TweenProperty {
	constructor(name, from, to) {
		if (name == "x")
			this.tweenValue = this.tweenX;
		else if (name == "y")
			this.tweenValue = this.tweenY;
		else if (name == "state")
			this.tweenValue = this.tweenState;
		else {
			this.tweenValue = this.tweenName;
			this.name = name;
		}
		this.from = from;
		this.to = to;
	}
	tween(target, fraction) {
		this.tweenValue(target, fraction, this.from, this.to);
	}
	tweenName(target, fraction, from, to) {
		target[this.name] = from + (fraction * (to - from));
	}
	tweenState(target, fraction, from, to) {
		target.state = from + (fraction * (to - from));
	}
	tweenX(target, fraction, from, to) {
		target.x = from + (fraction * (to - from));
	}
	tweenY(target, fraction, from, to) {
		target.y = from + (fraction * (to - from));
	}
}

class TweenOnProperty extends TweenProperty {
	constructor(name, values) {
		let length = values.length - 1;
		super(name, values[0], values[length]);
		this.length = length;
		this.values = values;
	}
	tween(target, fraction) {
		let values = this.values;
		let length = this.length;
		let index = length * fraction;
		let from = Math.floor(index);
		let to = Math.min(from + 1, length);
		this.tweenValue(target, index - from, values[from], values[to]);
	}
}

export class Tween {
	constructor(target, properties, duration, easing = null, delay = 0) {
		this.target = target;
		this.properties = properties;
		this.delay = delay;
		this.duration = duration;
		this.easing = easing;
		this.time = undefined;
	}
	static from(target, fromProperties, duration, easing, delay) {
		let properties = [], name
		for (name in fromProperties) {
			let from = fromProperties[name];
			properties.push(new TweenProperty(name, from, target[name]));
		}
		return new Tween(target, properties, duration, easing, delay);
	}
	static on(target, onProperties, duration, easing, delay) {
		let properties = [], name
		for (name in onProperties) {
			let values = onProperties[name];
			properties.push(new TweenOnProperty(name, values));
		}
		return new Tween(target, properties, duration, easing, delay);
	}
	static to(target, toProperties, duration, easing, delay) {
		let properties = [], name
		for (name in toProperties) {
			let to = toProperties[name];
			properties.push(new TweenProperty(name, target[name], to));
		}
		return new Tween(target, properties, duration, easing, delay);
	}
	seekTo(time) {
		let duration = this.duration;
		time -= this.delay;
		if (time < 0)
			time = 0;
		else if (time > duration)
			time = duration;
		if (this.time !== time) {
			this.time = time;
			let target = this.target;
			let fraction = time / duration;
			let easing = this.easing;
			if (easing)
				fraction = easing.call(Math, fraction);
			let properties = this.properties;
			let c = properties.length;
			for (let i = 0; i < c; i++)
				properties[i].tween(target, fraction)
		}
	}
}
Object.freeze(Tween.prototype);

export default class Timeline {
	constructor() {
		this.tweens = [];
		this.delay = 0;
		this.duration = 0;
		this.time = undefined;
	}
	add(tween, when = this.duration) {
		when += tween.delay;
		tween.delay = when;
		this.duration = Math.max(when + tween.duration, this.duration);
		this.tweens.push(tween);
		return this;
	}
	from(target, fromProperties, duration, easing = null, delay = 0, when = this.duration) {
		when += delay;
		this.duration = Math.max(when + duration, this.duration);
		this.tweens.push(Tween.from(target, fromProperties, duration, easing, when));
		return this;
	}
	on(target, onProperties, duration, easing = null, delay = 0, when = this.duration) {
		when += delay;
		this.duration = Math.max(when + duration, this.duration);
		this.tweens.push(Tween.on(target, onProperties, duration, easing, when));
		return this;
	}
	to(target, toProperties, duration, easing = null, delay = 0, when = this.duration) {
		when += delay;
		this.duration = Math.max(when + duration, this.duration);
		this.tweens.push(Tween.to(target, toProperties, duration, easing, when));
		return this;
	}
	seekTo(time) {
		let duration = this.duration;
		time -= this.delay;
		if (time < 0)
			time = 0;
		else if (time > duration)
			time = duration;
		if (this.time !== time) {
			this.time = time;
			let tweens = this.tweens;
			let c = tweens.length;
			for (let i = 0; i < c; i++) {
				tweens[i].seekTo(time);
			}
		}
	}
	get fraction() {
		return this.time / this.duration;
	}
	set fraction(it) {
		this.seekTo(it * this.duration);
	}
}
Object.freeze(Timeline.prototype);
