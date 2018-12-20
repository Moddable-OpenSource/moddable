/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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

/* note: this code assumes "RGB" format neopixels
 */

import NeoPixel from "neopixel";
import Time from "time";
import Timer from "timer";
import Timeline from "piu/Timeline";

function randomInt(min, max) {
    let span = max - min;
    return Math.floor(Math.random() * Math.floor(span) + min);
}

const TwoPI = Math.PI * 2;

export class NeoStrandEffect {
	constructor(dictionary) {
		this.strand = dictionary.strand;
		this.start = dictionary.start ? dictionary.start : 0;
		this.end = dictionary.end ? dictionary.end : this.strand.length;
		this.reverse = dictionary.reverse ? dictionary.reverse : 0;
		this.dur = dictionary.duration ? dictionary.duration : 1000;
		this.pos = dictionary.position ? dictionary.position : 0;
		this.speed = dictionary.speed ? dictionary.speed : 1;
		this.size = dictionary.size ? dictionary.size : this.end - this.start;
		this.loop = dictionary.loop ? dictionary.loop : 0;
		this.tickle = dictionary.tickle ? dictionary.tickle : 50;
		this.onComplete = dictionary.onComplete;

		this.lastTickle = 0;
		let timeline = this.timeline = new Timeline();
	}
	
	reset(effect) {
		effect.duration = effect.timeline.duration;
		effect.time = 0;
		effect.startTicks = Time.ticks;
		if (this.loop)
			effect.loopPrepare(effect);
	}

	loopPrepare(effect) { }

	activate(effect) {
		xsTrace("an effect needs an activate function\n");
	}

	idle(effect, ticks) {
		if (ticks > (effect.lastTickle + effect.tickle)) {
			effect.lastTickle = ticks;
			if (effect.reverse == 1)
				effect.time = effect.duration - ((ticks - effect.startTicks) % effect.duration);
			else
				effect.time = ticks - effect.startTicks;
			
			if (effect.time > effect.duration) {
//@ maybe loop == -1 is back and forth
				if (effect.loop) {
					effect.loopPrepare(effect);
					effect.startTicks = ticks;
					effect.time = effect.time % effect.duration;
					effect.timeline.seekTo(effect.time);
				}
				else if (undefined !== effect.onComplete)
					effect.onComplete(effect);
			}
			else {
				effect.timeline.seekTo(effect.time);
			}
		}
	}

}

class HueSpan extends NeoStrandEffect {
	constructor(dictionary) {
		super(dictionary);
		this.name = "HueSpan";
		this.loop = dictionary.loop ? dictionary.loop : 1;
		this.inc = (1 / this.size) * this.speed;
		this.hsv = { hue: 0 };
		this.hsv.sat = dictionary.saturation ? dictionary.saturation : 1;
		this.hsv.val = dictionary.value ? dictionary.value : 1;
	}

	activate(effect) {
//@ maybe add hueStart and hueEnd
		effect.timeline.on(effect, { hue: [0, 1] }, effect.dur, null, 0);
		effect.reset(effect);
	}

	set hue(h) {
		for (let i=this.start; i<this.end; i++) {
			h = (h + this.inc + this.pos) % 1;
			this.strand.set(i, this.strand.hsvToRgb(h, this.hsv.sat, this.hsv.val));
		}
	}
}

class Sine extends NeoStrandEffect {
	constructor(dictionary) {
		super(dictionary);
		this.name = "Sine";
		this.loop = dictionary.loop ? dictionary.loop : 1;
		this.amplitude = dictionary.amplitude ? dictionary.amplitude : 1;
		this.offset = dictionary.offset ? dictionary.offset : 0;
		this.vary = dictionary.vary ? dictionary.vary : "b";

		this.inc = (TwoPI / this.size) * this.speed;

		if (this.vary == "r" || this.vary == "g" || this.vary == "b")
			this.amplitude *= 255;
	}

	activate(effect) {
		effect.timeline.on(effect, { effectValue: [ 0, TwoPI ] }, effect.dur, null, 0);
		effect.reset(effect);
	}

	set effectValue(value) {
		for (let i=this.start; i<this.end; i++) {
			let v = Math.sin(value+((i-this.start) * this.inc)+this.pos);
			v += this.offset;
			if (v < 0) v = 0;
			else if (v >= 1) v = 1;

			let p = this.strand.pixels[i];
			if (this.vary == "h" || this.vary == "s" || this.vary == "v") {
				let hsv = this.strand.rgbToHsv ((p&0xff00000)>>16, (p&0xff00)>>8, p&0xff);
				v += (amplitude * v);
				if (this.vary == "h")
					hsv.h = v;
				else if (this.vary == "s")
					hsv.s = v;
				else if (this.vary == "v")
					hsv.v = v;
				this.strand.pixels[i] = this.strand.hsvToRgb(hsv.h, hsv.s, hsv.v);
			}
			else if (this.vary == "r" || this.vary == "g" || this.vary == "b") {
				let c;
				let x = (v * this.amplitude);
				if (x > 0xff) x = 0xff;
				if (x < 0) x = 0;
				if (this.vary == "r") {
					x = x << 16;
					c = x | (p&0x00FFFF);
				}
				else if (this.vary == "g") {
					x = x << 8;
					c = x | (p&0xFF00FF);
				}
				else if (this.vary == "b") {
					c = x | (p&0xFFFF00);
				}
				this.strand.pixels[i] = c;
			}
		}
	}
}

class Marquee extends NeoStrandEffect {
	constructor(dictionary) {
		super(dictionary);
		this.name = "Marquee";
		this.loop = dictionary.loop ? dictionary.loop : 1;
		this.sizeA = dictionary.sizeA ? dictionary.sizeA : 1;
		this.sizeB = dictionary.sizeB ? dictionary.sizeB : 2;
		this.size = this.sizeA + this.sizeB;

		if (dictionary.rgbA) {
			let c = dictionary.rgbA;
			this.rgbA = this.strand.makeRGB(c.r, c.g, c.b);
		}
		else
			this.rgbA = this.strand.makeRGB(0, 0, 0x80);

		if (dictionary.rgbB) {
			let c = dictionary.rgbB;
			this.rgbB = this.strand.makeRGB(c.r, c.g, c.b);
		}
		else
			this.rgbB = this.strand.makeRGB(0, 0, 0);
		
	}

	activate(effect) {
		effect.timeline.on(effect, { step: [0, effect.sizeA + effect.sizeB] }, effect.dur, null, 0);
		effect.reset(effect);
	}

	set step(s) {
		let x;
		s = Math.floor(s);

		s += this.pos;
		for (let i=this.start; i<this.end; i++) {
			x = (s + i) % this.size;
			if (x < this.sizeA)
				this.strand.set(i, this.rgbA, this.start, this.end);
			else
				this.strand.set(i, this.rgbB, this.start, this.end);
		}
	}
}

class Pulse extends NeoStrandEffect {
	constructor(dictionary) {
		super(dictionary);
		this.name = "Pulse";
		this.loop = dictionary.loop ? dictionary.loop : 1;
		this.dir = dictionary.direction ? dictionary.direction : 0;
		this.mode = dictionary.mode ? dictionary.mode : 0;
		this.fade = dictionary.fade ? dictionary.fade : 0.05;
		this.size = dictionary.size ? dictionary.size : 3;
		this.dur = dictionary.duration ? dictionary.duration : 3000;
		this.pos = dictionary.position ? dictionary.position : "random";

		this.fadeRgb = this.strand.makeRGB(255 * this.fade, 255 * this.fade, 255 * this.fade);

		if (dictionary.rgb) {
			let c = dictionary.rgb;
			this.rgb = this.strand.makeRGB(c.r, c.g, c.b);
		}
		else
			this.rgb = this.strand.makeRGB(0x30, 0x30, 0x30);
	}

	loopPrepare(effect) {
		if (this.pos == "random")
			this.loc = randomInt(effect.start, effect.end);
		else
			this.loc = this.pos;
	}

	activate(effect) {
		effect.timeline.on(effect, { pulseLoc: [ effect.start-effect.size, effect.end+effect.size ] }, effect.dur, null, 0);
		effect.reset(effect);
	}

	set pulseLoc(px) {
		let did;
		let f, b;
		if (-1 == this.dir)
			f = Math.floor(this.loc - px);
		else if (0 == this.dir) {
			f = Math.floor(this.loc + px); 
			b = Math.floor(this.loc - px);
		}
		else
			f = Math.floor(this.loc + px); 

		if ((this.lastF == f) && (this.lastB == b))
			return;

		this.lastF = f; this.lastB = b;

		for (let i=this.start; i<this.end; i++) {
			did = 0;
			if (i >= f && i < f + this.size) {
				this.strand.op(i, this.rgb, this.mode, this.start, this.end);
				did = 1;
			}
			else if (this.dir == 0) {
				if (i >= b && i < b + this.size) {
					this.strand.op(i, this.rgb, this.mode, this.start, this.end);
					did = 1;
				}
			}
			if (!did) {
				this.strand.sub(i, this.fadeRgb, this.start, this.end);
			}
		}
	}
}

class Pattern extends NeoStrandEffect {
	constructor(dictionary) {
		super(dictionary);
		this.name = "Pattern"
		this.pattern = dictionary.pattern ? dictionary.pattern
			: [ 0x003300, 0x003300, 0x003300, 0x131313, 0x131313 ];
		this.mode = dictionary.mode ? dictionary.mode : 0;
	}

	activate(effect) {
		effect.timeline.on(effect, { effectValue: [ 0, effect.dur ] }, effect.dur, null, 0);
		effect.reset(effect);
		effect.pattern_set = 0;
	}

	set effectValue(value) {
		if (0 == this.pattern_set) {
			let j;
			for (let i=this.start; i<this.end; i++) {
				j = i % this.pattern.length;
				this.strand.op(i, this.pattern[j], this.mode, this.start, this.end);
			}
			this.pattern_set = 1;
		}
	}
}

function doFade(strand, start, end, amt) {
	for (let i=start; i<end; i++) {
		let p = strand.pixels[i];
		let r = ((p & 0xff0000) >> 16) - amt;
		let g = ((p & 0x00ff00) >>  8) - amt;
		let b =  (p & 0x0000ff)        - amt;
		if (r < 0) r = 0;
		if (g < 0) g = 0;
		if (b < 0) b = 0;
		strand.set(i, strand.makeRGB(r, g, b));
	}
}

class Dim extends NeoStrandEffect {
	constructor(dictionary) {
		super(dictionary);
		this.name = "Fade";
		this.amount = 255 / this.dur;
	}

	loopPrepare(effect) {
		effect.lastValue = 1;
	}

	activate(effect) {
		effect.timeline.on(effect, { effectValue: [ 0, 1 ] }, effect.dur, null, 0);
		effect.reset(effect);
	}

	set effectValue(value) {
		let d = value - this.lastValue;

		if (d > 0)
			doFade(this.strand, this.start, this.end, d * 255);
		this.lastValue = value;
	}
}


class Ease extends NeoStrandEffect {
	constructor(dictionary) {
		super(dictionary);
		this.name = "Ease";
		this.loop = dictionary.loop ? dictionary.loop : 1;
		this.easing = dictionary.easing;
		this.size = dictionary.size ? dictionary.size : 3;
		this.dur += this.dur / 3;		// add a third for the bounce effect

		if (dictionary.rgb) {
			let c = dictionary.rgb;
			this.rgb = this.strand.makeRGB(c.r, c.g, c.b);
		}
		else
			this.rgb = this.strand.makeRGB(0, 0, 0x13);

		this.lastStep = this.start - 1;
	}

	activate(effect) {
		if (effect.dir < 0)
			effect.timeline.on(effect, { step: [effect.end, effect.start] }, effect.dur, this.easing, 0);
		else
			effect.timeline.on(effect, { step: [effect.start, effect.end] }, effect.dur, this.easing, 0);
		effect.reset(effect);
	}

	set step(s) {
		let x;
		s = Math.floor(s);

		if (s == this.lastStep)
			return;

		trace("clear at " + this.lastStep);	
		for (x=this.lastStep; x<this.lastStep + this.size; x++)
			this.strand.set(x, 0, this.start, this.end);
		this.lastStep = s;

		trace(" set at " + s + "\n");
		for (x=s; x<s+this.size; x++)
			this.strand.set(x, this.rgb, this.start, this.end);
	}
}


function doFadeHSV(strand, start, end, amt) {
	for (let i=start; i<end; i++) {
		let p = strand.pixels[i];
		let r = ((p & 0xff0000) >> 16);
		let g = ((p & 0x00ff00) >>  8);
		let b =  (p & 0x0000ff);
		let hsv = strand.rgbToHsv (r, g, b);
		if (hsv.v)
			hsv.v -= amt;
		if (hsv.v < 0) hsv.v = 0;
		strand.set(i, strand.hsvToRgb(hsv.h, hsv.s, hsv.v));
	}
}

export class NeoStrand extends NeoPixel {
	constructor(dictionary) {
		super(dictionary);
		this.pixels = new Uint32Array(this.length);
		this.running = 0;
		this.freq = dictionary.freq ? dictionary.freq : 10;
	}

	set(idx, color, start = 0, end = this.length) {
		idx = Math.floor(idx);
		if ((idx < start) || (idx > end))
			return;

		this.pixels[idx] = color;
	}

	add(idx, color, start = 0, end = this.length) {
		idx = Math.floor(idx);
		if ((idx < start) || (idx > end))
			return;
		this._add(idx, color);
	}

	_add(idx, color) {
		let p = this.pixels[idx];
		let r = ((p & 0xff0000) + (color & 0xff0000)) >> 16;
		let g = ((p & 0x00ff00) + (color & 0x00ff00)) >>  8;
		let b =  (p & 0x0000ff) + (color & 0x0000ff);
		if (r > 0xff) r = 0xff;
		if (g > 0xff) g = 0xff;
		if (b > 0xff) b = 0xff;
		this.pixels[idx] = this.makeRGB(r, g, b);
	}

	sub(idx, color, start = 0, end = this.length) {
		idx = Math.floor(idx);
		if ((idx < start) || (idx > end))
			return;
		this._sub(idx, color);
	}

	_sub(idx, color) {
		let p = this.pixels[idx];
		let r = ((p & 0xff0000) - (color & 0xff0000)) >> 16;
		let g = ((p & 0x00ff00) - (color & 0x00ff00)) >>  8;
		let b =  (p & 0x0000ff) - (color & 0x0000ff);
		if (r < 0) r = 0;
		if (g < 0) g = 0;
		if (b < 0) b = 0;
		this.pixels[idx] = this.makeRGB(r, g, b);
	}

	op(idx, rgb, mode, start, end) {
		idx = Math.floor(idx);
		if ((idx < start) || (idx > end))
			return;

		if (mode == 1)
			this._add(idx, rgb, start, end);
		else if (mode == -1)
			this._sub(idx, rgb, start, end);
		else
			this.pixels[idx] = rgb;
	}

	update() {
		for (let i=0; i<this.length; i++)
			this.setPixel(i, this.pixels[i]);
		super.update();
	}

	setScheme(scheme) {
		this.scheme = scheme;
		if (this.running) {
			for (let i=0; i<this.length; i++)
				this.pixels[i] = 0;
			Timer.clear(this.timer);
			this.start();
		}
	}

	start(ms = this.freq) {
		//trace("start - ms: " + ms + "\n");
		this.running = 1;
		this.fill(0);
		
		this.scheme.forEach( function(effect) {
			effect.activate(effect);
			trace(` activating: ${effect.name} [${effect.start}-${effect.end}]\n`);
			 } );
		this.timer = Timer.repeat(id => {
			let t = Time.ticks;
			this.scheme.forEach( function(effect) { effect.idle(effect, t); });
			this.update(this);

			//trace("effect cycle took" + (Time.ticks - t) + "ms\n");
		}, ms);
	}

	stop() {
		Timer.clear(this.timer);
		this.running = 0;
	}

/*
	hsvToRgb(h, s, v) {
		return this.fast_hsvToRgb(h * (6 * 256), s * 255, v * 255);
	}
	fast_hsvToRgb (h, s, v) @ "xs_fast_hsv2rgb";
*/
	// hsvToRgb adapted from
	// http://axonflux.com/handy-rgb-to-hsl-and-rgb-to-hsv-color-model-c
	// all parameters [0..1]
	hsvToRgb (h, s, v) {
		let r, g, b;

		let i = Math.floor(h * 6);
		let f = h * 6 - i;
		let p = v * (1 - s);
		let q = v * (1 - f * s);
		let t = v * (1 - (1 - f) * s);

		switch(i % 6) {
			case 0: r = v, g = t, b = p; break;
			case 1: r = q, g = v, b = p; break;
			case 2: r = p, g = v, b = t; break;
			case 3: r = p, g = q, b = v; break;
			case 4: r = t, g = p, b = v; break;
			case 5: r = v, g = p, b = q; break;
		}

    	return (((r * 255) & 0xff) << 16) + (((g * 255) & 0xff) << 8) + ((b * 255) & 0xff);
	}

	// rgbToHsv adapted from
	//     // http://axonflux.com/handy-rgb-to-hsl-and-rgb-to-hsv-color-model-c
    // all parameters [0-255]
	rgbToHsv (r, g, b) {
		r = r / 255; g = g / 255; b = b / 255;
		let max = Math.max(r, g, b), min = Math.min(r, g, b);
		let hsv = { v: max };
		let d = max - min;
		hsv.s = max == 0 ? 0 : d / max;
		if (max == min)
			hsv.h = 0;
		else {
			switch (max) {
				case r: hsv.h = (g - b) / d + (g < b ? 6 : 0); break;
				case g: hsv.h = (b - r) / d + 2; break;
				case b: hsv.h = (r - g) / d + 4; break;
			}
			hsv.h /= 6;
		}
		return hsv;
	}
}

NeoStrand.Marquee = Marquee;
NeoStrand.HueSpan = HueSpan;
NeoStrand.Pulse = Pulse;
NeoStrand.Sine = Sine;
NeoStrand.Pattern = Pattern;
NeoStrand.Dim = Dim;
//NeoStrand.Ease = Ease;

Object.freeze(NeoStrand.prototype);
Object.freeze(NeoStrandEffect.prototype);

export default { NeoStrand, NeoStrandEffect };

