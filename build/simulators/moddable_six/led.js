import Control from "control";

function HSVtoRGB(h, s, v) {
    var r, g, b, i, f, p, q, t;
    if (arguments.length === 1) {
        s = h.s, v = h.v, h = h.h;
    }
    i = Math.floor(h * 6);
    f = h * 6 - i;
    p = v * (1 - s);
    q = v * (1 - f * s);
    t = v * (1 - (1 - f) * s);
    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }
    return {
        r: Math.round(r * 255),
        g: Math.round(g * 255),
        b: Math.round(b * 255)
    };
}

function RGBtoHSV(r, g, b) {
    if (arguments.length === 1) {
        g = r.g, b = r.b, r = r.r;
    }
    var max = Math.max(r, g, b), min = Math.min(r, g, b),
        d = max - min,
        h,
        s = (max === 0 ? 0 : d / max),
        v = max / 255;

    switch (max) {
        case min: h = 0; break;
        case r: h = (g - b) + d * (g < b ? 6: 0); h /= 6 * d; break;
        case g: h = (b - r) + d * 2; h /= 6 * d; break;
        case b: h = (r - g) + d * 4; h /= 6 * d; break;
    }

    return {
        h: h,
        s: s,
        v: v
    };
}

export default class extends Control {
	#color;
	constructor(options = {}) {
		super();
		this.#color = this.makeRGB(0, 0, 0);
	}
	getPixel(index) {
		return this.#color;
	}
	makeRGB(r, g, b) {
		return ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
	}
	makeHSB(h, s, v) {
		const { r, g, b } = HSVtoRGB(h / 360, s / 1000, v / 1000);
		return makeRGB(r, g, b);
	}
	off() {
		this.write(0);
	}
	on() {
		this.write(1);
	}
	read() {
		return this.#color ? 1 : 0;
	}
	setPixel(index, color) {
		this.#color = color;
	}
	update() {
		this.postJSON({ led:this.#color });
	}
	write(value) {
		value = value ? 255 : 0
		this.setPixel(0, this.makeRGB(value, value, value));
		this.update();
	}
	
	get brightness() {
		const color = this.#color;
		const r = (color & 0xFF0000) >> 16;
		const g = (color & 0x00FF00) >> 8;
		const b = (color & 0x0000FF);
		return RGBtoHSV(r, g, b).v * 1000;
	}
	set brightness(it) {
		const color = this.#color;
		let r = (color & 0xFF0000) >> 16;
		let g = (color & 0x00FF00) >> 8;
		let b = (color & 0x0000FF);
		const hsv = RGBtoHSV(r, g, b);
		hsv.v = it / 1000;
		const rgb = HSVtoRGB(hsv);
		this.#color = this.makeRGB(rgb.r, rgb.g, rgb.b);
	}
	get byteLength() {
		return 24;
	}
	get length() {
		return 1;
	}
}
