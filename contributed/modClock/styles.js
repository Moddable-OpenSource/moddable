/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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
import config from "mc/config";
import Time from "time";
import SetOfNiceColors from "nice-colors";

import html_content from "web";

const TwoPI = (Math.PI * 2);
const RAD2DEG = (180 / Math.PI);

	// name
	// tag				4 chars used to identify options, html, etc.
	// op_head			called before frame is drawn (for setup, whatever)
	// op_tail			called after frame is prepared, before it is drawn
	// op_set(p,v,d)	pixel, variable, digit
	// op_clear(p,v)
	// options_html		should return a chunk of html
	// options_response	should parse the options from response array options["key"]
	// base_options_html	"global" style options
	// base_options_response

export class ClockStyle {
	constructor(display, dict) {
		this.display = display;
		this.moving = (undefined !== dict.moving) ? dict.moving : 0;
	
		this.tag = (undefined !== dict.tag) ? dict.tag : "none";

        this.color1 = (undefined !== dict.color1) ? this.color1 : 0x52948b;
        this.bgRGB = (undefined !== dict.bgRGB) ? this.bgRGB : 0x000000;
        this.sat = (undefined !== dict.sat) ? dict.sat : 1.0;   // saturation
        this.val = (undefined !== dict.val) ? dict.val : 0.8;   // value

		if (undefined === this.op_set)
			this.op_set = (undefined !== dict.op_set) ? dict.op_set : function(p) { this.display.set(p, this.color1); };
		if (undefined === this.op_clear)
			this.op_clear = (undefined !== dict.op_clear) ? dict.op_clear : function(p) { this.display.set(p, 0); };
		if (undefined === this.op_setColon)
			this.op_setColon = (undefined !== dict.op_setColon) ? dict.op_setColon : function(p) { this.display.set(p, this.color1); };
		if (undefined === this.op_clearColon)
			this.op_clearColon = (undefined !== dict.op_clearColon) ? dict.op_clearColon : function(p) { this.display.set(p, 0); };

		if (undefined === this.options_html)
			this.options_html = function() { return ""; }

		this.xDest = this.xCenter = this.display.width / 2;
		this.yDest = this.yCenter = this.display.height / 2;

		this.speed = (undefined !== dict.speed) ? dict.speed : 50;

		this.lastMovedMS = 0;		// make sure it triggers before drawing

		this.blinkTimeMS = 1000;
		this.lastBlinkChangeMS = -this.blinkTimeMS;

		this.colonKinds = [ "Solid", "Blink", "None", "Blink Alternate", "Pulse", "Pulse Alternate" ];
		this.colonKind = 0;
	}

	get speed() { return this._speed; }
	set speed(v) {
		this._speed = parseInt(v); 
		this.movingMS = ((100-this._speed)*70)+200;
		if (undefined !== this.updateVars)
			this.updateVars();
	}

	get base_prefs() {
		return `"colonKind":"${this.colonKind}", "speed":"${this._speed}"`;
	}

	set base_prefs(val) {
		if (undefined !== val.colonKind) this.colonKind = parseInt(val.colonKind);
		if (undefined !== val.speed) this.speed = parseInt(val.speed);
	}

	get prefsJson() {
		let p = this.base_prefs;
		if (this.style_prefs)
			p += `, ${this.style_prefs}`;
		return `{${p}}`;
	}

	set prefsJson(val) {
		try {
			val = JSON.parse(val);
			this.base_prefs = val;
			if (this.style_prefs)
				this.style_prefs = val;
		}
		catch(e) {
			trace(e);
		}
	}

	// base options always show
	base_options_html() {
		let body = `<p> <div><label>Colon:</label><select name="COLN_kind">`;
		for (let i=0; i<this.colonKinds.length; i++) {
			body += `<option value=${i}`;
			if (this.colonKind == i)
				body += " selected";
			body += `>${this.colonKinds[i]}</option>`;
		}
		body += `</select></div><p>`;
		return body;
	}

	_base_options_response(kv) {
		if (undefined !== kv.COLN_kind)
			this.colonKind = parseInt(kv.COLN_kind);
		if (undefined !== kv[`${this.tag}_sped`])
			this.speed = parseInt(kv[`${this.tag}_sped`]);
	}

	options_response(kv) {
		this._base_options_response(kv);
		if (undefined !== this._options_response)
			this._options_response(kv);
	}

	options_html() {
		let body = "";
		if (undefined !== this._options_html)
			body += this._options_html();
		body += `<div><label>Speed:</label>`;
		body += html_content.slider(`${this.tag}_sped`, this.speed, 0, 100);
		body += `</div>`;
	
		return body;
	}

	doColon(p,v,x) {				// pixel, val, which colon pixel (0-1)
		if (this.display.ticks > this.lastBlinkChangeMS + this.blinkTimeMS) {
			this.blinkState = !this.blinkState;
			this.lastBlinkChangeMS = this.display.ticks;
		}

		switch (this.colonKind) {
			case 0:			// Solid
				this.op_set(p, v);
				break;
			case 1:			// Blink
				this.blinkState ? this.op_set(p, v) : this.op_clear(p, v);
				break;
			case 2:			// None
				this.op_clear(p, v);
				break;
			case 3:			// Blink Alternate
				if (x)
					this.blinkState ? this.op_set(p, v) : this.op_clear(p, v);
				else
					(!this.blinkState) ? this.op_set(p, v) : this.op_clear(p, v);
				break;
			case 4:			// Pulse
			case 5:			// Pulse Alternate
				this.op_set(p, v);
				let c = this.display.pixels[p];
				let m = (Math.sin(v*this.display.incVal)/2)+0.5;
				if (x)
					c = this.display.scaleColor(c, 4 === this.colonKind ? m : 1-m);
				else
					c = this.display.scaleColor(c, m);
				this.display.set(p, c);
				break;
		}
	}

	op_prep(v) {
		// moving center
		let offset = 0;
		if (this.display.timeVal < 2000) {		// account for size of initial digits
			if (this.display.timeVal < 1000) {
				if (this.display.timeVal < 200)
					offset = 5;
				else
					offset = 3;
			}
			else
				offset = 2;
		}
		this.digitsWidth = this.display.width - offset;

		if (this.display.ticks > (this.lastMovedMS + this.movingMS)) {
			this.lastMovedMS = this.display.ticks;
			this.xCenter = this.xDest;
			this.yCenter = this.yDest;
			this.xDest = Math.random() * this.digitsWidth;
			this.yDest = Math.random() * this.display.height;
			this.xInc = (this.xDest - this.xCenter) / this.movingMS;
			this.yInc = (this.yDest - this.yCenter) / this.movingMS;
		}

		// hsv value
		this.hue = (v * this.display.incHue) % 1;
		this.hsv = this.hsvToRgb(this.hue, this.sat, this.val);
	}

	// shuffle array
	// https://github.com/coolaj86/knuth-shuffle
	// https://bost.ocks.org/mike/shuffle/
	shuffle(array) {
		var m = array.length, t, i;
		// While there remain elements to shuffle…
		while (m) {
			// Pick a remaining element…
			i = Math.floor(Math.random() * m--);
			// And swap it with the current element.
			t = array[m];
			array[m] = array[i];
			array[i] = t;
		}
		return array;
	}

	mergeAndScale(amt, color1, color2) @ "xs_mergeAndScale";

	hsvToRgb(h, s, v) @ "xs_hsvtorgb";

	rgbToHex(c) {
		return (0x1000000 + c).toString(16).slice(1);
	}

	hexToRgb(h) {
		let r = parseInt(h.substring(3,5),16);
		let g = parseInt(h.substring(5,7),16);
		let b = parseInt(h.substring(7,9),16);
		return (r << 16 | g << 8 | b);
	}

	colorPickerHTML(tag, label, color, num=1) {
		let body = `<div style="float:left"><label>${label}</label>
<input name="${tag}_color${num}" id="${tag}_color${num}" value="${this.rgbToHex(color)}">
<br>
<canvas id="${tag}_ColorPicker${num}"></canvas>
<script>
new KellyColorPicker({place:'${tag}_ColorPicker${num}',input:'${tag}_color${num}'});
</script></div>`;

		return body;
	}

}


/*****************/
/* Single color  */

class Style_OneColor extends ClockStyle {
	constructor(display, dict) {
		super(display, dict);
		this.name = "One Color";
		this.tag = "ONEC";
        this.color1 = (undefined !== dict.color1) ? dict.color1 : 0xb84223;

		this.kind = (undefined !== dict.kind) ? dict.kind : 0;
		this.kinds = [ "LED", "Scan", "Twinkle" ];

		this.scanLine = 0;
		this.pulseMod = 20;

		this.lastSetMS = 0;
	}

	_options_html() {
		let body = `<div><label>Kind:</label> <select name="ONEC_kind">`;
		for (let i=0; i<this.kinds.length; i++) {
			body += `<option value=${i}`;
			if (this.kind == i)
				body += " selected";
			body += `>${this.kinds[i]}</option>`;
		}
		body += `</select></div><p> <div style="display: table-row">`;
		body += this.colorPickerHTML("ONEC", "Color: ", this.color1);
		body += `</div>`;
		return body;
	}

	_options_response(kv) {
		if (undefined !== kv.ONEC_color1)
			this.color1 = this.hexToRgb(kv.ONEC_color1);
		if (undefined !== kv.ONEC_kind)
			this.kind = parseInt(kv.ONEC_kind);
	}

	get style_prefs() { return `"color1":"${this.color1}", "kind":"${this.kind}"`; }

	set style_prefs(val) {
		if (undefined !== val.color1) this.color1 = parseInt(val.color1);
		if (undefined !== val.kind) this.kind = parseInt(val.kind);
	}

	op_head(v) {
		if (1 == this.kind) {
			if (this.display.ticks > this.lastSetMS + (4 * (100-this.speed))) {
				this.lastSetMS = this.display.ticks;
				if (this.scanLine > this.display.height + 1)
					this.scanLine = 0;
				else
					this.scanLine++;
			}
			if (undefined === this.scanFollower)
				this.scanFollower = [];
		}
	}

	op_set(p) {
		if (0 == this.kind) {
			this.display.set(p, this.color1);
		}
		else if (1 == this.kind) {
			let xy = this.display.pixelToXY(p);
			if (xy.y == this.scanLine)
				this.display.set(p, this.color1);
		}
		else if (2 == this.kind) {
			if (this.display.ticks > this.lastSetMS + (100-this.speed))
				if (0.10 > Math.random()) this.display.set(p, this.color1);
		}
	}

	op_tail(v) {
		if (1 == this.kind)
			this.display.dim(0, this.display.length, this.speed/10);
		else if (2 == this.kind) {
			if (this.display.ticks > this.lastSetMS + (100-this.speed)) {
				this.lastSetMS = this.display.ticks;

				this.display.dim(0, this.display.length, this.speed/20);
			}
		}
	}

	op_extra(v) {
		let x = this.display.extra_start;
		let l = this.display.extra;

		if (1 == this.kind) {
			if (this.scanLine == this.display.height) {
				this.scanFollower.push({lastMS:this.display.ticks, pixel:this.display.extra_start-1});
			}

			for (var k = 0; k < this.scanFollower.length; k++) {
				let follower = this.scanFollower[k];
				if (this.display.ticks > follower.lastMS + (4 * (100-this.speed))) {
					follower.lastMS = this.display.ticks;
					follower.pixel = follower.pixel + 1;
					if (follower.pixel >= this.display.length)
						this.scanFollower.pop();
					else
						this.display.set(follower.pixel, this.color1);
				}
			}
		}
		else {
			for (let i=x; i<x+l; i++) {
				if (0 == this.kind)
					this.display.set(i, this.color1);
				else if (2 == this.kind) {
					if (this.display.ticks > this.lastSetMS + (100-this.speed)*2) {
						if (0.10 > Math.random()) this.display.set(i, this.color1);
					}
				}
			}
		}
	}
}


/*****************/
/* Two color  */

class Style_TwoColor extends ClockStyle {
	constructor(display, dict) {
		super(display, dict);
		this.name = "Two Color";
		this.tag = "TWOC";
        this.color1 = (undefined !== dict.color1) ? dict.color1 : 0x52948b;
        this.color2 = (undefined !== dict.color2) ? dict.color2 : 0x128277;

		this.kind = (undefined !== dict.kind) ? dict.kind : 0;
		this.kinds = [ "Still", "Fade", "Waves", "Sparkle" ];

		this.decayAmt = 5;
		this.pulseMod = 20;
	}

	_options_html() {
		let body = `<div><label>Kind:</label> <select name="TWOC_kind">`;
		for (let i=0; i<this.kinds.length; i++) {
			body += `<option value=${i}`;
			if (this.kind == i)
				body += " selected";
			body += `>${this.kinds[i]}</option>`;
		}
		body += `</select></div><p> <div style="display: table-row">`;
		body += this.colorPickerHTML("TWOC", "Color 1: ", this.color1);
		body += this.colorPickerHTML("TWOC", "Color 2: ", this.color2, 2);
		body += `</div>`;
		return body;
	}

	_options_response(kv) {
		if (undefined !== kv.TWOC_color1)
			this.color1 = this.hexToRgb(kv.TWOC_color1);
		if (undefined !== kv.TWOC_color2)
			this.color2 = this.hexToRgb(kv.TWOC_color2);
		if (undefined !== kv.TWOC_kind)
			this.kind = parseInt(kv.TWOC_kind);
	}

	get style_prefs() { return `"color1":"${this.color1}", "color2":"${this.color2}", "kind":"${this.kind}"`; }

	set style_prefs(val) {
		if (undefined !== val.kind) this.kind = parseInt(val.kind);
		if (undefined !== val.color1) this.color1 = parseInt(val.color1);
		if (undefined !== val.color2) this.color2 = parseInt(val.color2);
	}

	op_set(p, v) {
		let xy = this.display.pixelToXY(p);
		if (0 == this.kind) {						// still
			this.display.set(p, xy.y > this.display.height/2 ? this.color2 : this.color1);
		}
		else if (1 == this.kind) {				// Fade
			let x = this.display.incVal * ((this.speed / 100) + 0.3);
			let a = Math.sin(v*x);
			a = (a/2)+0.5;
			this.display.set(p, this.mergeAndScale(a, this.color1, this.color2));
			}
		else if (2 == this.kind) {				//  Waves
			let speed = this.speed / 3;
			let x = (this.display.incVal / 150) * (100-speed)/2;
			let a = Math.sin(v*x*this.speed/4);
			a = (a/2)+0.5;
			let b = (xy.y+1) / this.display.height;		// b is midpoint

			this.display.set(p, a >= (b-0.1) ? this.color1 : this.color2);
		}
		else if (3 == this.kind) {					// Sparkle
			let speed = this.speed / 400;
			this.display.set(p, Math.random() > speed ? this.color1 : this.color2);
		}
	}

	op_extra(v) {
		let x = this.display.extra_start;
		let l = this.display.extra;
		for (let i=x; i<x+l; i++) {
			let a = (Math.sin(v*this.display.incVal*2 + i*0.01)/2);
			if (0 == this.kind) {						// still
				this.display.set(i, i % 2 ? this.color1 : this.color2);
			}
			else if (1 == this.kind) {					// Fade
				a += 0.5;
				this.display.set(i, this.mergeAndScale(a, this.color1, this.color2));
			}
			else if (2 == this.kind) {					// Waves
				let center = x + (l >> 1);
				a += 0.52;
				let b = (i - x) / l;
				if (Math.abs(b-a) < 0.1)
					this.display.set(i, this.mergeAndScale(a, this.color2, this.color1));
				else
					this.display.set(i, a >= b ? this.color1 : this.color2);
			}
			else if (3 == this.kind) {					// Sparkle
				let speed = this.speed / 400;
				this.display.set(i, Math.random() > speed ? this.color2 : this.color1);
			}
		}
	}
}

/*****************/
/* Rainbow       */

class Style_Rainbow extends ClockStyle {
	constructor(display, dict) {
		super(display, dict);
		this.name = "Rainbow";
		this.tag = "RNBW";
		this.kind = (undefined !== dict.kind) ? dict.kind : 0;
		this.kinds = [ "Sunrise", "Span", "Scan", "Disc", "Moving Disc", "Target", "Moving Target" ];

		this.rainbowSpeed = (undefined !== dict.rainbowSpeed) ? dict.rainbowSpeed : 50;
		this.direction = 0;
	}

	_options_html() {
		let body = `<div><label>Kind:</label>`;
		body += `<select name="RNBW_kind">`;
		for (let i=0; i<this.kinds.length; i++) {
			body += `<option value=${i}`;
			if (this.kind == i)
				body += " selected";
			body += `>${this.kinds[i]}</option>`;
		}
		body += `</select><p><label>Rainbow Speed:</label>`;
		body += html_content.slider("RNBW_speed", this.rainbowSpeed, 0, 100);
		body += `</div>`;
		return body;
	}

	_options_response(kv) {
		if (undefined !== kv.RNBW_kind)
			this.kind = parseInt(kv.RNBW_kind);
		if (undefined !== kv.RNBW_speed)
			this.rainbowSpeed = parseInt(kv.RNBW_speed);
	}

	get style_prefs() { return `"kind":"${this.kind}","rSpeed":"${this.rainbowSpeed}"`; }

	set style_prefs(val) {
		if (undefined !== val.kind) this.kind = parseInt(val.kind);
		if (undefined !== val.rSpeed) this.rainbowSpeed = parseInt(val.rSpeed);
	}

	op_head(v) {
		this._hueInc = (v * this.display.incHue) * (this.rainbowSpeed / 20);
	}

	op_set(p, v) {
		let x = this.display.width/2;
		let y = this.display.height/2;
		let xy, a;
		let elapsed = this.display.ticks - this.lastMovedMS;
		switch (this.kind) {
			case 0:						// sunrise
				this.hueTarget(p, v, x, this.display.height, 1);
				break;
			case 1:						// span
				this.display.set(p, this.hsv);
				break;
			case 2:						// scan
				xy = this.display.pixelToXY(p);
				a = this._hueInc;
				switch (this.direction) {
					case 0: a += xy.y / this.display.height; break;
					case 1: a -= xy.y / this.display.height; break;
					case 2: a -= xy.x / this.display.width; break;
					default: a += xy.x / this.display.width; break;
				}
				if (a < 0) a = 1 - a;
				a %= 1;
				this.display.set(p, this.hsvToRgb(a, this.sat, this.val));
				break;
			case 3:						// disc
				this.hueDiscCenter(p, v, x, y);
				break;
			case 4:						// moving disc
				x = this.xCenter + (this.xInc * elapsed);
				y = this.yCenter + (this.yInc * elapsed);
				this.hueDiscCenter(p, v, x, y);
			case 5:						// target
			case 6:						// moving target
				if (this.kind == 6) {
					x = this.xCenter + (this.xInc * elapsed);
					y = this.yCenter + (this.yInc * elapsed);
				}
				this.hueTarget(p, v, x, y, this.direction);
				break;
		}
	}

	hueDiscCenter(p, v, x, y) {
		let xy = this.display.pixelToXY(p);
		let rad = Math.atan2(xy.x - x, xy.y - y);
		let hue = 1 + (rad * RAD2DEG)/360;
		hue += this._hueInc;
		hue %= 1;
		this.display.set(p, this.hsvToRgb(hue, this.sat, this.val));
	}

	hueTarget(p, v, x, y, direction) {
		let xy = this.display.pixelToXY(p);
		let dist = Math.sqrt((Math.pow(xy.x - x, 2))+(Math.pow(xy.y-y,2)));
		let hue = dist / this.display.width;
		if (direction == 0) {
			hue += this._hueInc;
			hue %= 1;
		}
		else if (direction == 1) {
			hue -= this._hueInc;
			let sub = Math.abs(hue) | 0;
			if (hue < 0)
				hue = sub + 1 + hue;
		}
		
		this.display.set(p, this.hsvToRgb(hue, this.sat, this.val));
	}

	op_extra(v) {
		let x = this.display.extra_start;
		let l = this.display.extra;
		let center = x + (l >> 1);
		for (let i=x; i<x+l; i++) {
			let dist = Math.sqrt(Math.pow(center - i, 2));
			let hue = dist / l;
			if (this.direction == 0) {
				hue += this._hueInc;
				hue %= 1;
			}
			else if (this.direction == 1) {
				hue -= this._hueInc;
				let sub = Math.abs(hue) | 0;
				if (hue < 0)
					hue = sub + 1 + hue;
			}
			this.display.set(i, this.hsvToRgb(hue, this.sat, this.val));
		}
	}
	
}

/*****************/
/* Special       */

class Style_Special extends ClockStyle {
	constructor(display, dict) {
		super(display, dict);
		this.name = "Special";
		this.tag = "SPCL";

		this.kinds = [ "America", "Fire", "Random", "Random decay" ];
		this.kind = 0;

		this.setTimeMS = 5000;
		this.lastSetMS = -this.setTimeMS;

		this.decayTimeMS = 200;
		this.lastDecayMS = -this.decayTimeMS;
	}

	_options_html() {
		let body = `<div><label>Kind:</label>`;
		body += `<select name="SPCL_kind">`;
		for (let i=0; i<this.kinds.length; i++) {
			body += `<option value=${i}`;
			if (this.kind == i)
				body += " selected";
			body += `>${this.kinds[i]}</option>`;
		}
		body += "</select></div>";
		return body;
	}

	_options_response(kv) {
		if (undefined !== kv.SPCL_kind)
			this.kind = parseInt(kv.SPCL_kind);
	}

	get style_prefs() { return `"kind":"${this.kind}"`; }

	set style_prefs(val) {
		if (undefined !== val.kind) this.kind = parseInt(val.kind);
	}

	op_head(v) {
		if (1 == this.kind) {				// Fire
			const COOLING = 5; // 55;
			const SPARKING = 120; // 120;

			let cooldown, i, j, x, l;
			l = this.display.extra + this.display.extra_start;
			if (l < this.display.width * this.display.height)
				l = this.display.width * this.display.height;
				
			if (undefined === this.heat)
				this.heat = new Uint8Array(l);

			for (j=0; j<this.display.width; j++) {
				for (i=0; i<this.display.height; i++) {
					x = j * this.display.height + i;
					cooldown = Math.random()*(((COOLING*10)/this.display.height)+2)

					if (cooldown > this.heat[x])
						this.heat[x] = 0;
					else
						this.heat[x] = this.heat[x] - cooldown;
				}
			}

			for (j=0; j<this.display.width; j++) {
				for (i=this.display.height-1; i>=2; i--) {
					x = j * this.display.height + i;
					this.heat[x] = (this.heat[x-1] + this.heat[x-2] + this.heat[x-2])/3;
				}
			}

			for (j=0; j<this.display.width; j++) {
				if (Math.random()*255 < SPARKING) {
					i = (Math.random() * 7) | 0;
					x = j * this.display.height + i;
					this.heat[x] = this.heat[x] + (Math.random() * (255-160)) + 160;
				}
			}
		}
	}

	op_set(p, v) {
		let xy = this.display.pixelToXY(p);

		if (this.kind == 0) {				// America
			if ((xy.x > 7) && (xy.y < 4)) 		// blue field with stars
				this.display.set(p, Math.random() > 0.9 ? 0xffffff : 0x0000ff);
			else 								// red or white bar
				this.display.set(p, xy.y % 2 ? 0xffffff : 0xff0000);
		}
		else if (this.kind == 1) {				// Fire
			let x = xy.x * this.display.height + xy.y;
			let c;

			let t192 = Math.round((this.heat[x]/255.0)*191);
			let heatramp = t192 & 0x3F;
			heatramp <<= 2;

			if (t192 > 0x80)
				c = 0xffff00 | heatramp;
			else if (t192 > 0x40)
				c = 0xff0000 | (heatramp << 8);
			else
				c = heatramp << 16;
			this.display.set(p, c);
		}
		else if ((this.kind == 2) || (this.kind == 3))  {	// Random color
			if (this.display.ticks > this.lastSetMS + this.setTimeMS) {
				let r = (Math.random() * 255) | 0;
				let g = (Math.random() * 255) | 0;
				let b = (Math.random() * 255) | 0;
				let c = (r << 16) | (g << 8) | b;
				if (this.kind == 2) {
					if (0.3 > Math.random())
						this.display.set(p, c);
				}
				else
					this.display.set(p, c);
			}
		}
	}

	op_tail(v) {
		if ((this.kind == 2) || (this.kind == 3)) {			// Random color fade
			if (this.display.ticks > this.lastSetMS + this.setTimeMS)
				this.lastSetMS = this.display.ticks;

			if (this.kind == 3)	{ 							// decay
				if (this.display.ticks > this.lastDecayMS + this.decayTimeMS) {
					this.lastDecayMS = this.display.ticks;
					this.display.dim(0, this.display.length, 5);
				}
			}
		}
	}

	op_extra(v) {
		let x = this.display.extra_start;
		let l = this.display.extra;
		for (let i=x; i<x+l; i++) {
			if (this.kind == 0) {							// America
					switch (i%3) {
						case 0: this.display.set(i, 0x00ff0000); break;
						case 1: this.display.set(i, 0x00ffffff); break;
						case 2: this.display.set(i, 0x000000ff); break;
					}
			}
			else if (this.kind == 1) {						// fire
				let n = i - x;
				let c;
	
				let t192 = Math.round((this.heat[n]/255.0)*191);
				let heatramp = t192 & 0x3F;
				heatramp <<= 2;
	
				if (t192 > 0x80)
					c = 0xffff00 | heatramp;
				else if (t192 > 0x40)
					c = 0xff0000 | (heatramp << 8);
				else
					c = heatramp << 16;
				this.display.set(i, c);
			}
			else if (this.kind == 2 || this.kind == 3) {	// random
				this.op_set(i, v);
			}
		}
	}
}

/*****************/
/* Nice Colors   */

class Style_NiceColors extends ClockStyle {
	constructor(display, dict) {
		super(display, dict);
		this.name = "Nice Colors";
		this.tag = "NICE";
		this.stepMS = (undefined !== dict.stepMS) ? dict.stepMS : 20000;
		this.div = this.stepMS / 4;
		this.lastColorChangeMS = -1;	// make sure it triggers once before drawing
		this.niceColor = 0;

		this.kind = (undefined !== dict.kind) ? dict.kind : 0;
		this.kinds = [ "Still", "Step", "Quad", "Quad step", "Moving" ];
	}

	get style_prefs() { return `"kind":"${this.kind}"`; }

	set style_prefs(val) {
		if (undefined !== val.kind) this.kind = parseInt(val.kind);
	}

	_options_html() {
		let body = `<div><label>Kind:</label>`;
		body += `<select name="NICE_kind">`;
		for (let i=0; i<this.kinds.length; i++) {
			body += `<option value=${i}`;
			if (this.kind == i)
				body += " selected";
			body += `>${this.kinds[i]}</option>`;
		}
		body += "</select></div>";
		return body;
	}

	_options_response(kv) {
		if (undefined !== kv.NICE_kind)
			this.kind = parseInt(kv.NICE_kind);
	}

	updateVars() {
		this.stepMS = (100 - this.speed) * 600;
		this.lastColorChangeMS = -1;
		this.div = this.stepMS / 4;
	}

	op_set(p, v, d) {
		let step = ((this.display.ticks - this.lastColorChangeMS) / this.div)|0;
		step %= 4;
		if (undefined === d)
			d = 2;
		switch (this.kind) {
			case 0:				// static color set - one per digit
				this.display.set(p, this.niceColors[d]);
				break;
			case 1:				// step
				this.display.set(p, this.niceColors[(d+step)%4]);
				break;
			case 2:				// quad
				step = 0;
				// don't break
			case 3:				// quad step
				this.quad(p, step);
				break;
			case 4:				// moving
				this.moveCenter(p);
				break;
		}
	}

	quad(p, step) {
		let xy = this.display.pixelToXY(p);
		let c;
		if (xy.x < (this.display.width/2)-1) {
			if (xy.y < this.display.height/2)
				c = this.niceColors[(step+0)%4];
			else
				c = this.niceColors[(step+1)%4];
		}
		else {
			if (xy.y < this.display.height/2)
				c = this.niceColors[(step+3)%4];
			else
				c = this.niceColors[(step+2)%4];
		}
		this.display.set(p, c);
	}

	moveCenter(p) {
		let x, y;
		let elapsed = this.display.ticks - this.lastMovedMS;
		x = this.xCenter + (this.xInc * elapsed);
		y = this.yCenter + (this.yInc * elapsed);
		let xy = this.display.pixelToXY(p);
		if (xy.x < x) {
			if (xy.y < y)
				this.display.set(p, this.niceColors[0]);
			else
				this.display.set(p, this.niceColors[1]);
		}
		else if (xy.y < y)
			this.display.set(p, this.niceColors[2]);
		else
			this.display.set(p, this.niceColors[3]);
	}

	op_head(v) {
		if (this.display.ticks > this.lastColorChangeMS + this.stepMS) {
			this.lastColorChangeMS = this.display.ticks;
			this.niceColor += 1;
			if (this.niceColor >= SetOfNiceColors.length)
				this.niceColor = 0;
			this.niceColors = SetOfNiceColors[this.niceColor].slice(0,4);
		}
	}

	op_extra(v) {
		let x = this.display.extra_start;
		let l = this.display.extra;
		let center = x + (l >> 1);
		for (let i=x; i<x+l; i++) {
			let which = (((i-x)/4)%4)|0;
			this.display.set(i, this.niceColors[which]);
		}
	}
}

/*****************/
/* Color wheel   */

class Style_ColorWheel extends ClockStyle {
	constructor(display, dict) {
		super(display, dict);
		this.name = "Color Wheel";
		this.tag = "WHEL";

		this.stepMS = (undefined !== dict.stepMS) ? dict.stepMS : 20000;
		this.div = this.stepMS / 4;
		this.lastColorChangeMS = -1;	// make sure it triggers once before drawing

		this.kind = (undefined !== dict.kind) ? dict.kind : 0;
		this.kinds = [ "Still", "Step", "Quad", "Quad step", "Moving" ];

		this.pastels = dict.pastels|0;

		this.color = [ 0, 0, 0, 0 ];
		this.colorKind = (undefined !== dict.colorKind) ? dict.colorKind : 0;
		this.colorKinds = [ "Monochromatic", "Complementary", "Split Complementary", "Analogous", "Triadic", "Tetradic", "Warm", "Cool" ];
	}

	_options_html() {
		let body = `<div><label>Kind:</label>`;
		body += `<select name="WHEL_kind">`;
		for (let i=0; i<this.kinds.length; i++) {
			body += `<option value=${i}`;
			if (this.kind == i)
				body += " selected";
			body += `>${this.kinds[i]}</option>`;
		}
		body += "</select></div><p>";
		body += `<div><label>Color Kind:</label>`;
		body += `<select name="WHEL_colr">`;
		for (let i=0; i<this.colorKinds.length; i++) {
			body += `<option value=${i}`;
			if (this.colorKind == i)
				body += " selected";
			body += `>${this.colorKinds[i]}</option>`;
		}
		body += `</select></div><p><div><label>Pastels:</label>
<input type="checkbox" name="WHEL_pstl" ${(this.pastels ?" checked":"")} value="1"></div><p>`;

		return body;
	}

	_options_response(kv) {
		if (undefined !== kv.WHEL_kind)
			this.kind = parseInt(kv.WHEL_kind);
		this.pastels = parseInt(kv.WHEL_pstl);
		if (undefined !== kv.WHEL_colr) {
			this.colorKind = parseInt(kv.WHEL_colr);
			this.lastColorChangeMS = -1;
		}
		this.updateVars();
	}

	get style_prefs() { return `"colorKind":"${this.colorKind}", "kind":"${this.kind}", "pastels":"${this.pastels}"`; }

	set style_prefs(val) {
		if (undefined !== val.kind) this.kind = parseInt(val.kind);
		if (undefined !== val.colorKind) this.colorKind = parseInt(val.colorKind);
		if (undefined !== val.pastels) this.pastels = parseInt(val.pastels);

		this.updateVars();
	}

	updateVars() {
		if (this.pastels)
			this.sat = 0.60;
		else
			this.sat = 1;
		this.stepMS = (100 - this.speed) * 600;
		this.lastColorChangeMS = -1;
		this.div = this.stepMS / 4;
	}


	// warm - from 0 to 70 deg and 340 to 0
	warmHue() {
		let x;
		let min = 0;
		let max1 = 70 / 360;
		let max2 = 340 / 360;
		do {
			x = Math.random();
		} while (x > max1 && x < max2);
		return x;
	}
	
	// cool - 90 to 270
	coolHue() { 
		let x;
		let min = 90 / 360;
		let max = 270 / 360;
		do {
			x = Math.random();
		} while (x < min || x > max);
		return x;
	}

	op_head(v) {
		let i, inc;
		this._step = ((this.display.ticks - this.lastColorChangeMS) / this.div)|0;
		this._step %= 4;
		if (this.display.ticks > this.lastSetMS + (4 * 100-this.speed)) {
			this.lastSetMS = this.display.ticks;
		}
		if ((this.lastColorChangeMS < 0) || (this.display.ticks > this.lastColorChangeMS + this.stepMS)) {
			this.lastColorChangeMS = this.display.ticks;
			this.hue = Math.random();
			switch (this.colorKind) {
				case 0:	//  "Monochromatic"
					inc = 0.25;
					for (i=0; i<4; i++) {
//trace(`${this.color[i]}: h:${this.hue}, s:${this.sat}, v:${1.0-(i*inc)}\n`);
						this.color[i] = this.hsvToRgb(this.hue, this.sat, 1.0-(i*inc));
					}
					break;
				case 1: //	"Complementary"
					this.color[0] = this.hsvToRgb(this.hue, this.sat, 1.0);
					this.color[1] = this.hsvToRgb((this.hue+.5)%1, this.sat, 1.0);
					this.color[2] = this.hsvToRgb(this.hue, this.sat, 0.7);
					this.color[3] = this.hsvToRgb((this.hue+.5)%1, this.sat, 0.7);
					break;
				case 2: //	"Split Complementary"
					this.color[0] = this.hsvToRgb(this.hue, this.sat, 1.0);
					this.color[1] = this.hsvToRgb((this.hue+.4)%1, this.sat, 1.0);
					this.color[2] = this.hsvToRgb((this.hue+.5)%1, this.sat, 1.0);
					this.color[3] = this.hsvToRgb((this.hue+.6)%1, this.sat, 1.0);
					break;
				case 3: //	"Analogous"
					inc = 0.05;
					for (i=0; i<4; i++)
						this.color[i] = this.hsvToRgb((this.hue+(i*inc))%1, this.sat, 1.0);
					break;
				case 4: //  "Triadic"
					inc = 0.3333;
					for (i=0; i<4; i++)
						this.color[i] = this.hsvToRgb((inc*i)%1, this.sat, 1.0);
					break;
				case 5: //	"Tetradic"
					inc = 0.25;
					for (i=0; i<4; i++)
						this.color[i] = this.hsvToRgb((inc*i)%1, this.sat, 1.0);
					break;
				case 6: //  "Warm"
					for (i=0; i<4; i++)
						this.color[i] = this.hsvToRgb(this.warmHue(), this.sat, 1.0);
					break;
				case 7: //  "Cool"
					for (i=0; i<4; i++)
						this.color[i] = this.hsvToRgb(this.coolHue(), this.sat, 1.0);
					break;
				
			}
//			trace(`colors: ${this.color[0]}, ${this.color[1]}, ${this.color[2]}, ${this.color[3]}\n`);
		}
	}

	op_set(p, v, d) {
		if (undefined === d)
			d = 2;
		switch (this.kind) {
			case 0:				// static color set - one per digit
				this.display.set(p, this.color[d]);
				break;
			case 1:				// step
				this.display.set(p, this.color[(d+this._step)%4]);
				break;
			case 2:				// quad
				this._step = 0;
				// don't break
			case 3:				// quad step
				this.quad(p, this._step);
				break;
			case 4:				// moving
				this.moveCenter(p);
				break;
		}
	}

	quad(p, step) {
		let xy = this.display.pixelToXY(p);
		let c;
		if (xy.x < (this.display.width/2)-1) {
			if (xy.y < this.display.height/2)
				c = this.color[(step+0)%4];
			else
				c = this.color[(step+1)%4];
		}
		else {
			if (xy.y < this.display.height/2)
				c = this.color[(step+3)%4];
			else
				c = this.color[(step+2)%4];
		}
		this.display.set(p, c);
	}

	moveCenter(p) {
		let x, y;
		let elapsed = this.display.ticks - this.lastMovedMS;
		x = this.xCenter + (this.xInc * elapsed);
		y = this.yCenter + (this.yInc * elapsed);
		let xy = this.display.pixelToXY(p);
		if (xy.x < x) {
			if (xy.y < y)
				this.display.set(p, this.color[0]);
			else
				this.display.set(p, this.color[1]);
		}
		else if (xy.y < y)
			this.display.set(p, this.color[2]);
		else
			this.display.set(p, this.color[3]);
	}

	op_extra(v) {
		let x = this.display.extra_start;
		let l = this.display.extra;
		let center = x + (l >> 1);
		for (let i=x; i<x+l; i++) {
			let which = ((((i-x)/4)+this._step)%4)|0;
			this.display.set(i, this.color[which]);
		}
	}
}

ClockStyle.OneColor = Style_OneColor;
ClockStyle.TwoColor = Style_TwoColor;
ClockStyle.Rainbow = Style_Rainbow;
ClockStyle.NiceColors = Style_NiceColors;
ClockStyle.Special = Style_Special;
ClockStyle.ColorWheel = Style_ColorWheel;


Object.freeze(Style_OneColor.prototype);
Object.freeze(Style_TwoColor.prototype);
Object.freeze(Style_Rainbow.prototype);
Object.freeze(Style_NiceColors.prototype);
Object.freeze(Style_Special.prototype);
Object.freeze(Style_ColorWheel.prototype);

Object.freeze(ClockStyle.prototype);

export default ClockStyle;

