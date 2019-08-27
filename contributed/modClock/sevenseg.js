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
const DEFAULT_NUM_PIXELS = 58;
const EXTRA_PIXELS_START = 58;

const DisplayWidth = 17;
const DisplayHeight = 7;
const DigitWidth = 4;
const DigitHeight = 7;

const REFRESH_RATE = 10;		// ms refresh rate

const SAT = 1.0;
const TwoPI = (Math.PI * 2);
const RAD2DEG = (180 / Math.PI);
const TenPow = [ 1, 10, 100, 1000 ];

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
const sevenSegments = [
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
];

const letterSegments = [
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
	"i", 0b01100000,		// I -  bc    
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

export class SevenSegDisplay {
    constructor(dict) {
		this.supportedFormats = [ "RGB", "GRB", "RGBW" ];
		this.length = dict.length;
		this._pin = dict.pin;
		this.timing = dict.timing;
		this.order = "RGB"; // dict.order ? dict.order : "RGB";
		this._main_order = dict.order ? dict.order : "RGB";
		this._tail_order = dict.tail_order ? dict.tail_order : "RGB";

		this.width = DisplayWidth;
		this.height = DisplayHeight;

		this.dur = dict.duration ? dict.duration : CYCLE_DURATION;

		this.rgb = (undefined !== dict.rgb) ? dict.rgb : 0x0000ff;
		this.bg_rgb = (undefined !== dict.bg_rgb) ? dict.bg_rgb : 0x000000;

		
		this.twelve = dict.twelve ? dict.twelve : 0;
		this._layout = dict.layout !== undefined ? dict.layout : 0;
			this.segment_pixels = config.seven_segments[this._layout].segments;
			this.colonSegments = config.seven_segments[this._layout].colon;
			this.setupPixels();
		this.extra_start = EXTRA_PIXELS_START;
		this.extra = (undefined !== dict.extra) ? dict.extra : 0;
		// setting "extra" may cause the length to change, thus requiring
		// a new neopixel object - so do this before setting brightness

		this._brightness = (dict.brightness !== undefined) ? dict.brightness : 96;
		this._tail_brightness = (dict.tail_brightness !== undefined) ? dict.tail_brightness : 32;
		this._style = (undefined !== dict.style) ? dict.style : new ClockStyle.OneColor(this, {});

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

	setup_neopixels() {
		if (undefined !== this.neopixels)
			this.neopixels.close();
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


	get extra() { return this.extra_length; }
	set extra(v) {
// delete neopixels if it already exists?
		this.extra_length = v;
		this.length = DEFAULT_NUM_PIXELS + this.extra_length;
		this.setup_neopixels();
	}

	get style() { return this._style; }
	set style(val) { this._style = val; }

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

	findLetterSegment(letter) {
		for (let i=0; i<letterSegments.length; i+=2) {
			if (letterSegments[i] == letter)
				return letterSegments[i+1];
		}
		return letterSegment[1];
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
//			trace(`this.timeVal: ${this.timeVal} - utcHours: ${now.getUTCHours()}\n`);
			if (this.twelve) {
				if (this.timeVal > 1300) this.timeVal -= 1200;
				if (this.timeVal < 100) this.timeVal += 1200;
			}
		}
	
		if (undefined !== this._style.op_prep)
			this._style.op_prep(value);
		if (undefined !== this._style.op_head)
			this._style.op_head(value);

		let segment_pixels, pix, seg;
		for (let digits=0; digits<4; digits++) {
			let d;
			let skipDigit = 0;

			if (undefined !== this.userValue) {
				let v = this.userValue[3-digits];
				if (v >= "0" && v <= "9")
					d = sevenSegments[parseInt(v)];
				else if (v >= "a" && v <= "z")
					d = this.findLetterSegment(v);
				else
					skipDigit = 1;
			}
			else {
				let v = ((this.timeVal / TenPow[digits]) | 0) % 10;

				if ((digits == 3) && (v == 0)) {
					skipDigit = 1;
					v = 0;
				}
				d = sevenSegments[v];		// which 7 segments for this digit
			}

			if ( !((1 << digits) & displayDigits) || !this.blinkDisplayOn)
				skipDigit = 1;

			segment_pixels = this.segment_pixels[digits];

			for (seg=0; seg<7; seg++) {
				if (!skipDigit && (d & 0b10000000)) {	// segment that should be on
					for (pix=0; pix<segment_pixels[seg].length; pix++)
						this._style.op_set(segment_pixels[seg][pix], value, digits);
				}
				else {
					for (pix=0; pix<segment_pixels[seg].length; pix++)
						this._style.op_clear(segment_pixels[seg][pix], value);
				}
				d <<= 1;
			}
		}

		// colons
		if (doColons) {
			this._style.doColon(this.colonSegments[0], value, 0);
			this._style.doColon(this.colonSegments[1], value, 1);
		}
		else {
			this._style.op_clear(this.colonSegments[0], value);
			this._style.op_clear(this.colonSegments[1], value);
		}

		if (this.extra > 0 && (undefined !== this._style.op_extra))
			this._style.op_extra(value);

		if (undefined !== this._style.op_tail)
			this._style.op_tail(value);

		this.update();
	}

	set(p, c) { this.pixels[p|0] = c; }

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

	dim(start, end, amt) {
		for (let i=start; i<end; i++)
			this._dim(i, amt);
	}


	xyToPixel(x, y) {
		if (undefined === this.pixel_xy)
			return -1;
		return this.pixel_xy[x|0][y|0];
	}

	pixelToXY(p) {
		for (let x=0; x<DisplayWidth; x++)
			for (let y=0; y<DisplayHeight; y++)
				if (p == this.pixel_xy[x][y])
					return {x,y};
		return undefined;
	}

	setupPixels() {
		this.pixel_xy = [];
		this.pixel_xy[8] = [ -1, -1, this.colonSegments[0], -1, this.colonSegments[1], -1, -1 ];
		for (let d=0;d<4;d++) {
			let bx = d*4 + 1*(d>1);
			let segs = this.segment_pixels[d];
			this.pixel_xy[bx++] = [-1, segs[1][0], segs[1][1], -1, segs[2][0], segs[2][1], -1];
			this.pixel_xy[bx++] = [segs[0][1], -1, -1, segs[6][1], -1, -1, segs[3][1]];
			this.pixel_xy[bx++] = [segs[0][0], -1, -1, segs[6][0], -1, -1, segs[3][0]];
			this.pixel_xy[bx++] = [-1, segs[5][0], segs[5][1], -1, segs[4][0], segs[4][1], -1];
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
        for (let i=0; i<this.length; i++) {
			if (i < DEFAULT_NUM_PIXELS)
            	this.neopixels.setPixel(i, this.brightenAndConvert(this.pixels[i], this._brightness, this._main_order));
			else
            	this.neopixels.setPixel(i, this.brightenAndConvert(this.pixels[i], this._tail_brightness, this._tail_order));
		}

        this.neopixels.update();
    }

}

export default SevenSegDisplay;
Object.freeze(SevenSegDisplay.prototype);
