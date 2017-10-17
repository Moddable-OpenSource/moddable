/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

export class Tween {
	constructor(target, properties, duration, easing = null, delay = 0) {
		this.target = target;
		this.properties = properties;
		this.delay = delay;
		this.duration = duration;
		this.easing = easing;
		this.time = undefined;
	}
	static to(target, toProperties, fromProperties, duration, easing, delay) {
		let properties = [], name
		for (name in toProperties)
			properties.push({ name, from:fromProperties[name], to:toProperties[name] });
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
			let fraction = time / duration;
			let easing = this.easing;
			if (easing)
				fraction = easing.call(Math, fraction);
			let target = this.target;
			let properties = this.properties;
			let c = properties.length;
			for (let i = 0; i < c; i++) {
				let property = properties[i];
				let from = property.from;
				let to = property.to;
				target[property.name] = from + (fraction * (to - from));
			}
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
	to(target, toProperties, fromProperties, duration, easing = null, delay = 0) {
		let it = delay + this.duration;	
		this.duration += delay + duration;
		this.tweens.push(Tween.to(target, toProperties, fromProperties, duration, easing, it));
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
}
Object.freeze(Timeline.prototype);
