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

import Preference from "preference";
import config from "mc/config";

const PREF_WIFI = "wifi";
const PREF_CLOCK = "clock";
const PREF_STYLE = "style";

const DEFAULT_LAYOUT = 0;
const EXTRA_PIXELS_DEFAULT = 50;

const PREF_KEY_NAME = "name";
const PREF_KEY_SSID = "ssid";
const PREF_KEY_PASS = "password";
const PREF_KEY_TZ 	= "tz";
const PREF_KEY_LAYOUT = "layout";
const PREF_KEY_ZERO = "zero";
const PREF_KEY_PIN = "pin";
const PREF_KEY_DST = "dst";
const PREF_KEY_BRIGHTNESS = "brightness";
const PREF_KEY_TWELVE = "twelve";
const PREF_KEY_STYLE = "style";
const PREF_KEY_EXTRA = "extra";
const PREF_KEY_TAIL_ON = "tail_on";
const PREF_KEY_TAIL_ONLY = "tail_only";
const PREF_KEY_TAIL_ORDER = "tail_order";
const PREF_KEY_TAIL_BRIGHTNESS = "tail_brightness";
const PREF_KEY_TAIL_SCHEDULE = "tail_sched";
const PREF_KEY_TAIL_TIME_ON = "tail_time_on";
const PREF_KEY_TAIL_TIME_OFF = "tail_time_off";
const PREF_KEY_BUTTON_A = "buttonA";
const PREF_KEY_BUTTON_B = "buttonB";

export class ClockPrefs {
	constructor() {
		this.neopixel_pins = [ 22, 23 ];
		this.button_pins = [ "none", 0, 32, 34, 35 ];
		this.dst_types = [ "Off", "On", "Auto" ];
		this._storedName = this.getPref(PREF_CLOCK, PREF_KEY_NAME, "clock");
		this._name = this._storedName;
		this._ssid = this.getPref(PREF_WIFI, PREF_KEY_SSID);
		this._pass = this.getPref(PREF_WIFI, PREF_KEY_PASS);
		this._tz = this.getPref(PREF_CLOCK, PREF_KEY_TZ, 3)|0;
		this._layout = this.getPref(PREF_CLOCK, PREF_KEY_LAYOUT, DEFAULT_LAYOUT)|0;
		this._zero = this.getPref(PREF_CLOCK, PREF_KEY_ZERO, 0)|0;
		this._pin = this.getPref(PREF_CLOCK, PREF_KEY_PIN, config.seven_segments[this._layout].pin)|0;
		this._dst = this.getPref(PREF_CLOCK, PREF_KEY_DST, 0)|0;
		this._brightness = this.getPref(PREF_CLOCK, PREF_KEY_BRIGHTNESS, 36)|0;
		this._twelve = this.getPref(PREF_CLOCK, PREF_KEY_TWELVE, 1)|0;
		this._style = this.getPref(PREF_CLOCK, PREF_KEY_STYLE, "ONEC");
		this._extra = this.getPref(PREF_CLOCK, PREF_KEY_EXTRA, EXTRA_PIXELS_DEFAULT)|0;
		this._tail_on = this.getPref(PREF_CLOCK, PREF_KEY_TAIL_ON, 1);
		this._tail_only = this.getPref(PREF_CLOCK, PREF_KEY_TAIL_ONLY, 0);
		this._tail_order = this.getPref(PREF_CLOCK, PREF_KEY_TAIL_ORDER, "RGB");
		this._tail_brightness = this.getPref(PREF_CLOCK, PREF_KEY_TAIL_BRIGHTNESS, 20)|0;
		this._tail_sched = this.getPref(PREF_CLOCK, PREF_KEY_TAIL_SCHEDULE, 0);
		this._tail_time_on = this.getPref(PREF_CLOCK, "tail_time_on", "1700");
		this._tail_time_off = this.getPref(PREF_CLOCK, "tail_time_off", "0100");
		this._buttonA = this.getPref(PREF_CLOCK, "buttonA", 34);
		this._buttonB = this.getPref(PREF_CLOCK, "buttonB", 35);
	}

    reset() {
        Preference.delete(PREF_WIFI, "ssid");
        Preference.delete(PREF_WIFI, "password");
		Preference.delete(PREF_CLOCK, "name");
		Preference.delete(PREF_CLOCK, "tz");
		Preference.delete(PREF_CLOCK, "dst");
		Preference.delete(PREF_CLOCK, "brightness");
		Preference.delete(PREF_CLOCK, "twelve");
		Preference.delete(PREF_CLOCK, "layout");
		Preference.delete(PREF_CLOCK, "style");
		Preference.delete(PREF_CLOCK, "extra");
		Preference.delete(PREF_CLOCK, "tail_on");
		Preference.delete(PREF_CLOCK, "tail_only");
		Preference.delete(PREF_CLOCK, "tail_order");
		Preference.delete(PREF_CLOCK, "tail_brightness");
		Preference.delete(PREF_CLOCK, "tail_sched");
		Preference.delete(PREF_CLOCK, "tail_time_on");
		Preference.delete(PREF_CLOCK, "tail_time_off");
//		Preference.delete(PREF_CLOCK, "pin");
//		Preference.delete(PREF_CLOCK, "buttonA");
//		Preference.delete(PREF_CLOCK, "buttonB");
		if (undefined !== this.owner)
        	this.owner.styles.forEach(this.resetPref);
    }

	get name() { return this._name; }
	set name(v) {this._name = v; }

	get storedName() { return this._storedName; }
	set storedName(v) {this._storedName = v; this._name = v; Preference.set(PREF_CLOCK, "name", this._name); }

	get ssid() { return this._ssid; }
	set ssid(v) { this._ssid = v; Preference.set(PREF_WIFI, "ssid", this._ssid); }

	get pass() { return this._pass; }
	set pass(v) { this._pass = v; Preference.set(PREF_WIFI, "password", this._pass); }

	get tz() { return this._tz; }
	set tz(v) {
		if (this._tz != v|0) {
			this._tz = v|0;
			Preference.set(PREF_CLOCK, "tz", this._tz);
			if (undefined !== this.owner)
				this.owner.fetchTime();
		}
	}

	get dst() { return this._dst; }
	set dst(v) {
		if (this._dst != v|0) {
			this._dst = v|0;
			Preference.set(PREF_CLOCK, "dst", this._dst);
			if (undefined !== this.owner)
				this.owner.fetchTime();
		}
	}

	get pin() { return this._pin; }
	set pin(v) {
		if (this._pin != v|0) {
			this._pin = v|0;
			Preference.set(PREF_CLOCK, "pin", this._pin);
			if (undefined !== this.owner)
				this.owner.display.pin = this._pin;
		}
	}

	get zero() { return this._zero; }
	set zero(v) {
		if (this._zero != v|0) {
			this._zero = v|0;
			Preference.set(PREF_CLOCK, "zero", this._zero);
			if (undefined !== this.owner)
				this.owner.display.zero = this._zero;
		}
	}

	get layout() { return this._layout; }
	set layout(v) {
		if (this._layout != v|0) {
			this._layout = v|0;
			Preference.set(PREF_CLOCK, "layout", this._layout);
			if (undefined !== this.owner)
				this.owner.display.layout = this._layout;
		}
	}

	get twelve() { return this._twelve; }
	set twelve(v) {
		if (this._twelve != v|0) {
			this._twelve = v|0;
			Preference.set(PREF_CLOCK, "twelve", this._twelve);
			if (undefined !== this.owner)
				this.owner.display.twelve = this._twelve;
		}
	}

	get brightness() { return this._brightness; }
	set brightness(v) {
		if (this._brightness != v|0) {
			this._brightness = v|0;
			Preference.set(PREF_CLOCK, "brightness", this._brightness);
			if (undefined !== this.owner)
				this.owner.display.brightness = this._brightness;
		}
	}
		
	get tail_brightness() { return this._tail_brightness; }
	set tail_brightness(v) {
		if (this._tail_brightness != v|0) {
			this._tail_brightness = v|0;
			Preference.set(PREF_CLOCK, "tail_brightness", this._tail_brightness);
			if (undefined !== this.owner)
				this.owner.display.tail_brightness = this._tail_brightness;
		}
	}
	
	get tail_order() { return this._tail_order; }
	set tail_order(v) {
		this._tail_order = v;
		Preference.set(PREF_CLOCK, "tail_order", this._tail_order);
		if (undefined !== this.owner)
			this.owner.display.tail_order = this._tail_order;
	}
	
	get tail_on() { return this._tail_on; }
	set tail_on(v) {
		this._tail_on = v;
		Preference.set(PREF_CLOCK, "tail_on", this._tail_on);
		if (undefined !== this.owner)
			this.owner.display.tail_on = this._tail_on;
	}
	
	get tail_only() { return this._tail_only; }
	set tail_only(v) {
		if (this._tail_only != v|0) {
			this._tail_only = v|0;
			Preference.set(PREF_CLOCK, "tail_only", this._tail_only);
			if (undefined !== this.owner)
				this.owner.display.tail_only = this._tail_only;
		}
	}

	get tail_sched() { return this._tail_sched; }
	set tail_sched(v) {
		if (this._tail_sched != v|0) {
			this._tail_sched = v|0;
			Preference.set(PREF_CLOCK, "tail_sched", this._tail_sched);
			if (undefined !== this.owner)
				this.owner.display.tail_sched = this._tail_sched;
		}
	}
	
	get tail_time_on() { return this._tail_time_on; }
	set tail_time_on(v) {
		if (this._tail_time_on != v|0) {
			this._tail_time_on = v|0;
			Preference.set(PREF_CLOCK, "tail_time_on", this._tail_time_on);
			if (undefined !== this.owner)
				this.owner.display.tail_time_on = this._tail_time_on;
		}
	}
	
	get tail_time_off() { return this._tail_time_off; }
	set tail_time_off(v) {
		if (this._tail_time_off != v|0) {
			this._tail_time_off = v|0;
			Preference.set(PREF_CLOCK, "tail_time_off", this._tail_time_off);
			if (undefined !== this.owner)
				this.owner.display.tail_time_off = this._tail_time_off;
		}
	}
	
	get extra() { return this._extra; }
	set extra(v) {
		if (this._extra != v|0) {
			this._extra = v|0;
			Preference.set(PREF_CLOCK, "extra", this._extra);
			if (undefined !== this.owner)
				this.owner.display.extra = this._extra;
		}
	}
	
	get style() { return this._style; }
	set style(v) {
		if (v === parseInt(v)) {
			if (undefined !== this.owner) {
				if (undefined !== this.owner.styles[v]) {
					this._style = this.owner.styles[v].tag;
					Preference.set(PREF_CLOCK, "style", this._style);
					this.owner.currentStyle = this.owner.styles[v];
				}
			}
		}
		else if (this._style != v) {
			this._style = v;
			Preference.set(PREF_CLOCK, "style", this._style);
			if (undefined !== this.owner)
				this.owner.currentStyle = this.owner.styles.find(x => x.tag === this._style)
		}
	}

	get buttonA() { return this._buttonA; }
	set buttonA(v) {
		if (v !== "none")
			this._buttonA = parseInt(v);
		Preference.set(PREF_CLOCK, "buttonA", this._buttonA);
	}

	get buttonB() { return this._buttonB; }
	set buttonB(v) {
		if (v !== "none")
			this._buttonB = parseInt(v);
		Preference.set(PREF_CLOCK, "buttonB", this._buttonB);
	}

	getPref(domain, key, default_value) {
		let ret = Preference.get(domain, key);
		if (undefined === ret) ret = default_value;
		return ret;
	}

	loadPref(element,i,a) {
		let p = Preference.get(PREF_STYLE, element.tag);
		if (undefined !== p)
			element.prefsJson = p;
	}

	savePref(element,i,a) {
		Preference.set(PREF_STYLE, element.tag, element.prefsJson);
	}

	resetPref(element, i, a) {
		Preference.delete(PREF_STYLE, element.tag);
	}

};

export default ClockPrefs;

