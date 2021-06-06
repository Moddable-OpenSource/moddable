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
import Timer from "timer";
import Time from "time";
import NeoPixel from "neopixel";
import ClockStyle from "styles";

import config from "mc/config";


// 50mA / pixel
const DEFAULT_NUM_PIXELS = 56;	// (2 pixels per 7 segments * 4 digits)
//const EXTRA_PIXELS_START = 58;

const DisplayWidth = 17;
const DisplayHeight = 7;

const REFRESH_RATE = 10;		// ms refresh rate

const TenPow = Object.freeze([ 1, 10, 100, 1000 ]);

const CYCLE_DURATION = (10000);

//    aa
//  f    b   
//  f    b c1
//    gg
//  e    c c2
//  e    c   
//    dd   dp
//
//     'a', 'b', 'c', 'd', 'e', 'f', 'g', dp
// bit  7    6    5    4    3    2    1    0
const sevenSegments = Object.freeze([
		0b11111100,		// 0 - abcdef
		0b01100000,		// 1 -  bc
		0b11011010,		// 2 - ab de g
		0b11110010,		// 3 - abcd  g
		0b01100110,		// 4 -  bc  fg
		0b10110110,		// 5 - a cd fg
		0b10111110,		// 6 - a cdefg
		0b11100000,		// 7 - abc    
		0b11111110,		// 8 - abcdefg
		0b11110110,		// 9 - abcd fg
]);

const letterSegments_ = [
	"?", 0b11001010,		// ? - ab  e g
	" ", 0b00000000,		// space is nothing
	"-", 0b00000010,		// - -       g
	"_", 0b00010000,		// _ -    d   
	"[", 0b10011100,		// [ - a  def 
	"]", 0b11110000,		// ] - abcd   
	"a", 0b11111010,		// a - abcde g
	"b", 0b00111110,		// b -   cdefg
	"c", 0b00011010,		// c -    de g
	"d", 0b01111010,		// d -  bcde g
	"e", 0b10011110,		// E - a  defg
	"f", 0b10001110,		// F - a   efg
	"g", 0b11110110,		// g - abcd fg
	"h", 0b01101110,		// H -  bc efg
	"i", 0b00100000,		// i -   c    
	"j", 0b01110000,		// J -  bcd   
	"k", 0b01101110,		// K -  bc efg  // like H
	"l", 0b00011100,		// L -    def 
	"m", 0b00101010,		// m -   c e g	// like n - no way to make m
	"n", 0b00101010,		// n -   c e g
	"o", 0b00111010,		// o -   cde g
	"p", 0b11001110,		// p - ab  efg
	"q", 0b11100110,		// q - abc  fg
	"r", 0b00001010,		// r -     e g
	"s", 0b10110110,		// s - a cd fg
	"t", 0b00011110,		// t -    defg
	"u", 0b00111000,		// u -   cde  
	"v", 0b00111000,		// v -   cde	// like u
	"w", 0b00111000,		// w -   cde    // like u
	"x", 0b01101110,		// X -  bc efg	// like H
	"y", 0b01100110,		// Y -  bc  fg
	"z", 0b11011010,		// Z - ab de g
];

// create 8-bit integer array for lookups that merge letters and numbers
const letterSegments = new Uint8Array(256);
letterSegments.fill(letterSegments_[1]);
for (let i=0; i <letterSegments_.length; i+=2)
	letterSegments[letterSegments_[i].charCodeAt()] = letterSegments_[i +1];
for (let i=0; i <10; i+=1)
	letterSegments[i + '0'.charCodeAt()] = sevenSegments[i];

export class SevenSegDisplay {
    constructor(dict) {
		this.supportedFormats = [ "RGB", "GRB", "RGBW" ];
		this.length = dict.length;
		this._pin = dict.pin;
		this.timing = dict.timing;
		this.order = "RGB"; // dict.order ? dict.order : "RGB";
		this._main_order = dict.order ?? "RGB";
		this._tail_on = dict.tail_on ?? 1;
		this._tail_order = dict.tail_order ?? "RGB";

		this.width = DisplayWidth;
		this.height = DisplayHeight;

		this.dur = dict.duration ?? CYCLE_DURATION;

		this.rgb = dict.rgb ?? 0x0000ff;
		this.bg_rgb = dict.bg_rgb ?? 0x000000;

		this.twelve = dict.twelve ?? 0;
		this._layout = dict.layout ?? 0;
			this.segment_pixels = config.seven_segments[this._layout].segments;
			this.colonSegments = config.seven_segments[this._layout].colon;
			this.setupPixels();
		this._zero = dict.zero ?? 0;
		this.tail_length = dict.tail ?? 0;
		this.tail_start = DEFAULT_NUM_PIXELS + this.colonSegments.length;
		this.length = this.tail_start + this.tail_length;

		this.setup_neopixels();

		this.tail_only = dict.tail_only ?? 0;

		this.tail_sched = dict.tail_sched ?? 0;
		this.tail_time_on = dict.tail_time_on ?? 0;
		this.tail_time_off = dict.tail_time_off ?? 0;

		this._brightness = dict.brightness ?? 96;
		this._tail_brightness = dict.tail_brightness ?? 32;
		this._style = dict.style ?? new ClockStyle.OneColor(this, {});

		this.lastMS = 0;
		this.blinkDisplayOn = 1;
		this.blinkSpeed = 500;
		this.blinkDigits = 0xf;			// all 4 digits

		this.height = DisplayHeight;
		this.incHue = 1 / this.dur;
		this.incVal = (2 * Math.PI) / this.dur;

		this.userValue = dict.value;			// if defined, show a fixed value

		this.start(REFRESH_RATE);
    }

	start(refreshMS) {
		if (undefined !== this.timer)
			Timer.clear(this.timer);

		this.timer = Timer.repeat(id => {
			this.effectValue = Time.ticks;
		}, refreshMS);
	}

	restart() @ "do_restart";

	setup_neopixels() {
		if (undefined !== this.neopixels) {
			let oldLength = this.neopixels.length;
			this.neopixels.fill(0);
        	this.neopixels.update();
			if (oldLength > this.length)		// if neopixels are longer, it's okay
				return;
		// otherwise, reboot
			this.restart();
		}

		this.neopixels = new NeoPixel(this);
		this.neopixels.fill(0);
		this.neopixels.brightness = 255;
		this.pixels = new Uint32Array(this.length);
	}

	get pin() { return this._pin; }
	set pin(val) {
		trace(`Changing pin from ${this._pin} to ${val}\n`);
		this._pin = val;
		this.setup_neopixels();
	}


/*
	get tail() { return this.tail_length; }
	set tail(v) { this.tail_length = v; }
	set tail(v) {
// delete neopixels if it already exists?
		this.tail_length = v;
		this.length = DEFAULT_NUM_PIXELS + this.colonSegments.length + this.tail_length;
//		if (this.colonSegments == 7)
//			this.length += 5;
		this.setup_neopixels();
	}
*/
	get extra() { return this.tail_length; }
	set extra(v) {
		this.tail_length = v;
		this.length = DEFAULT_NUM_PIXELS + this.colonSegments.length + this.tail_length;
		this.setup_neopixels();
	}

	get style() { return this._style; }
	set style(val) {
		this._style = val;
		for (let n=0; n<this.colonSegments.length; n++)
			this._style.op_clear(this.colonSegments[n], 0);
	}

	value(val) {
		if (undefined === val) {
			this.userValue = undefined;
			this.blinkSpeed = 0;
		}
		else
			this.userValue = ("    " + val.toString().toLowerCase()).slice(-4); 
		return this;
	}

	get timeShowing() { return this.userValue === undefined; }


	get brightness() { return this._brightness; }
	set brightness(val) { this._brightness = val; }

	get tail_brightness() { return this._tail_brightness; }
	set tail_brightness(val) { this._tail_brightness = val; }

	get tail_on() { return this._tail_on; }
	set tail_on(val) { this._tail_on = val; }

	get tail_sched() { return this._tail_sched; }
	set tail_sched(val) { this._tail_sched = val|0; }

	get tail_time_on() { return this._tail_time_on; }
	set tail_time_on(val) { this._tail_time_on = val|0; }

	get tail_time_off() { return this._tail_time_off; }
	set tail_time_off(val) { this._tail_time_off = val|0; }

	get shouldShowTail() {
		let ret = true;
		if (!this.tail_on)			// master off switch
			return false;
		if (!this.tail_sched)		// if not scheduled, then on.
			return true;
		if (this._tail_time_off < this._tail_time_on) {	// off in morning
			if ((this.timeVal24 >= this._tail_time_off)
				&& (this.timeVal24 < this._tail_time_on))
				return false;
		}
		else {
			if ((this.timeVal24 < this._tail_time_on)
				|| (this.timeVal24 >= this._tail_time_off))
				return false;
		}
		return true;
	}

	get tail_only() { return this._tail_only; }
	set tail_only(val) {
		if (undefined === this._tail_only)
			this._tail_only = 0;

		if (this._tail_only != val) {
			this._tail_only = val;
			if (this._tail_only) {
				if (this.tail_start != 0) {
					this.tail_length += this.tail_start;
					this.tail_start = 0;
				}
			}
			else {
				if (this.tail_start != 0) {
					this.tail_start = DEFAULT_NUM_PIXELS + this.colonSegments.length;
					this.tail_length -= this.length - this.tail_start;
				}
			}
		}
	 }

	get tail_order() { return this._tail_order; }
	set tail_order(val) { this._tail_order = val; }

	get layout() { return this._layout; }
	set layout(val) {
		if (val != this._layout) {
			this._layout = val;
			trace(`Setting pixel layout: ${config.seven_segments[this._layout].name}\n`);
			this.segment_pixels = config.seven_segments[this._layout].segments;
			this.colonSegments = config.seven_segments[this._layout].colon;
			this.setupPixels();
		}
	}

	get zero() { return this._zero; }
	set zero(val) {
		if (val != this._zero)
			this._zero = val;
	}

    set effectValue(value) {
		let doColons = 1;
		let color, bgColor;
		let displayDigits = 0xF;		// show all 4 digits
		this.ticks = value;

		if (this.blinkSpeed) {
			if (this.ticks > (this.lastMS + this.blinkSpeed)) {
				this.lastMS = this.ticks;
				this.blinkDisplayOn = !this.blinkDisplayOn;
			}
			if (this.blinkDisplayOn)
				displayDigits = this.blinkDigits;
			else
				displayDigits = 0;
		}

		if (undefined != this.userValue) {		// like blinking 12:00
			doColons = 0;
		}
		else {
			let now = new Date();
			this.timeVal = now.getHours() * 100 + now.getMinutes();
			this.timeVal24 = this.timeVal;
			this.seconds = now.getSeconds();
//			trace(`this.timeVal: ${this.timeVal} - utcHours: ${now.getUTCHours()}\n`);
			if (this.twelve) {
				if (this.timeVal > 1300) this.timeVal -= 1200;
				if (this.timeVal < 100) this.timeVal += 1200;
			}
		}
	
		if (undefined !== this._style.op_prep)
			this._style.op_prep(value);
		if (undefined !== this._style.op_first)
			this._style.op_first(value);

		if (!this.tail_only) {
			let segment_pixels, pix, seg;
			for (let digits=0; digits<4; digits++) {
				let d;
				let skipDigit = 0;

				if (undefined !== this.userValue) {
					d = letterSegments[this.userValue[3-digits].charCodeAt()]
					if (!d)
						skipDigit = 1;
				}
				else {
					let v = ((this.timeVal / TenPow[digits]) | 0) % 10;
	
					if ((digits == 3) && (v == 0)) {
						if (!this._zero)
							skipDigit = 1;
					}
					d = sevenSegments[v];		// which 7 segments for this digit
				}
	
				if ( !((1 << digits) & displayDigits) || !this.blinkDisplayOn)
					skipDigit = 1;
	
				segment_pixels = this.segment_pixels[digits];
	
				for (seg=0; seg<7; seg++) {
					const pixels = segment_pixels[seg];
					const length = pixels.length;
					if (!skipDigit && (d & 0b10000000)) {	// segment that should be on
						for (pix=0; pix<length; pix++)
							this._style.op_set(pixels[pix], value, digits, seg);
					}
					else {
						for (pix=0; pix<length; pix++)
							this._style.op_clear(pixels[pix], value);
					}
					d <<= 1;
				}
			}
	
			// colons
			if (doColons)
				this._style.doColon(value);
			else {
				for (let n=0; n<this.colonSegments.length; n++)
					this._style.op_clear(this.colonSegments[n], value);
			}
		}

		if (this.tail_length > 0 && (undefined !== this._style.op_tail))
			if (this.tail_on)
				this._style.op_tail(value);
			else
				for (let i=this.tail_start; i<this.tail_start+this.tail_length; i++)
					this.set(i, 0);

		if (undefined !== this._style.op_last)
			this._style.op_last(value);

		this.update();
	}

	set(p, c) { this.pixels[p|0] = c; }

/*
	scaleColor(c, amt) {				// 0..1.0
		let r = (c & 0xff0000) >> 16;
		let g = (c & 0x00ff00) >>  8;
		let b = (c & 0x0000ff);
		r = (r * amt) & 0xff;
		g = (g * amt) & 0xff;
		b = (b * amt) & 0xff;
		return this.neopixels.makeRGB(r, g, b);
	}


	_dim(i, amt) {			// dim decreases the amount floored at 0
		let p = this.pixels[i];
		let r = ((p & 0xff0000) >> 16) - amt; if (r < 0) r = 0;
		let g = ((p & 0x00ff00) >>  8) - amt; if (g < 0) g = 0;
		let b =  (p & 0x0000ff)        - amt; if (b < 0) b = 0;
		this.pixels[i] = (r << 16) | (g << 8) | b;
	}
*/

	scaleColor(c, amt) @ "xs_scaleColor";				// 0..1.0
	_dim(color, amt) @ "xs_dimColor";

	dim(start, end, amt) {
		for (let i=start; i<end; i++)
			this.pixels[i] = this._dim(this.pixels[i], amt);
	}

	xyToPixel(x, y) {
		if (undefined === this.pixel_loc)
			return -1;
		return this.pixel_loc[x|0][y|0];
	}

	pixelToXY(p) { return this.pixel_xy[p]; }

	setupPixels() {
		let numPix = DEFAULT_NUM_PIXELS + this.colonSegments.length;
		this.pixel_xy = [];
		this.pixel_loc = [];
		this.pixel_marqA = [];
		this.pixel_marqB = [];
		if (this.colonSegments.length == 7) {		// r6 uses a pixel bar
			this.pixel_loc[8] = [ this.colonSegments[0], this.colonSegments[1], this.colonSegments[2], this.colonSegments[3], this.colonSegments[4], this.colonSegments[5], this.colonSegments[6] ];
		}
		else
			this.pixel_loc[8] = [ -1, -1, this.colonSegments[0], -1, this.colonSegments[1], -1, -1 ];
		for (let d=0;d<4;d++) {
			let bx = d*4 + 1*(d>1);
			let segs = this.segment_pixels[d];
			this.pixel_loc[bx++] = [-1, segs[1][0], segs[1][1], -1, segs[2][0], segs[2][1], -1];
			this.pixel_loc[bx++] = [segs[0][1], -1, -1, segs[6][1], -1, -1, segs[3][1]];
			this.pixel_loc[bx++] = [segs[0][0], -1, -1, segs[6][0], -1, -1, segs[3][0]];
			this.pixel_loc[bx++] = [-1, segs[5][0], segs[5][1], -1, segs[4][0], segs[4][1], -1];
			this.pixel_marqA.push(segs[0][0], segs[1][0], segs[2][0], segs[3][1], segs[4][1], segs[5][1], segs[6][1]);
			this.pixel_marqB.push(segs[0][1], segs[1][1], segs[2][1], segs[3][0], segs[4][0], segs[5][0], segs[6][0]);
		}
		this.pixel_marqA.push(this.colonSegments[0]);
		this.pixel_marqB.push(this.colonSegments[1]);
		if (this.colonSegments.length == 7) {
			this.pixel_marqA.push(this.colonSegments[2]);
			this.pixel_marqB.push(this.colonSegments[3]);
			this.pixel_marqA.push(this.colonSegments[4]);
			this.pixel_marqB.push(this.colonSegments[5]);
			this.pixel_marqA.push(this.colonSegments[6]);
		}
		
		

		for (let p=0; p<numPix; p++) {
			for (let x=0; x<DisplayWidth; x++)
				for (let y=0; y<DisplayHeight; y++)
					if (p == this.pixel_loc[x][y])
						this.pixel_xy[p] = {x,y};
		}
	}

	showTime(style) {
		this.userValue = undefined;	// don't blink 12
		this.style = style;
		this.blinkSpeed = 0;
		this.blinkDisplayOn = 1;
	}

	blink(speed=500, digits=0xf) {
		this.blinkSpeed = speed;
		this.blinkDigits = digits;
	}

	brightenAndConvert(color, brightness, order) @ "xs_brightenAndConvert";

	update() {
		let brightness = this._brightness;
		let order = this._main_order;
		let clockPixels = DEFAULT_NUM_PIXELS + this.colonSegments.length;

		if (this.tail_only) {
			if (this.shouldShowTail)
				brightness = this._tail_brightness;
			else
				brightness = 0;
		}

        for (let i=0, length = this.length, pixels = this.pixels; i<length; i++) {
        	if (i === clockPixels) { // this.tail_start) { // DEFAULT_NUM_PIXELS) {
				if (this.shouldShowTail)
					brightness = this._tail_brightness;
				else
					brightness = 0;
				order = this._tail_order;
        	}
			this.neopixels.setPixel(i, this.brightenAndConvert(pixels[i], brightness, order));
		}

        this.neopixels.update();
    }

}

export default SevenSegDisplay;
Object.freeze(SevenSegDisplay.prototype);
