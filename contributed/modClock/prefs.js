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

const EXTRA_PIXELS_DEFAULT = 5;

export class ClockPrefs {
	constructor() {
		this.available_pins = [ 22, 23 ];
		this._storedName = this.getPref(PREF_CLOCK, "name", "clock");
		this._name = this._storedName;
		this._ssid = this.getPref(PREF_WIFI, "ssid");
		this._pass = this.getPref(PREF_WIFI, "password");
		this._tz = this.getPref(PREF_CLOCK, "tz", 3)|0;
		this._layout = this.getPref(PREF_CLOCK, "layout", 0)|0;
		this._pin = this.getPref(PREF_CLOCK, "pin", config.seven_segments[this._layout].pin)|0;
		this._dst = this.getPref(PREF_CLOCK, "dst", 0)|0;
		this._brightness = this.getPref(PREF_CLOCK, "brightness", 36)|0;
		this._twelve = this.getPref(PREF_CLOCK, "twelve", 0)|0;
		this._style = this.getPref(PREF_CLOCK, "style", "ONEC");
		this._extra = this.getPref(PREF_CLOCK, "extra", EXTRA_PIXELS_DEFAULT)|0;
		this._tail_order = this.getPref(PREF_CLOCK, "tail_order", "GRB");
		this._tail_brightness = this.getPref(PREF_CLOCK, "tail_brightness", 20)|0;
	}

    reset() {
        Preference.delete(PREF_WIFI, "ssid");
        Preference.delete(PREF_WIFI, "password");
		Preference.delete(PREF_CLOCK, "name");
		Preference.delete(PREF_CLOCK, "tz");
		Preference.delete(PREF_CLOCK, "pin");
		Preference.delete(PREF_CLOCK, "dst");
		Preference.delete(PREF_CLOCK, "brightness");
		Preference.delete(PREF_CLOCK, "twelve");
		Preference.delete(PREF_CLOCK, "layout");
		Preference.delete(PREF_CLOCK, "style");
		Preference.delete(PREF_CLOCK, "extra");
		Preference.delete(PREF_CLOCK, "tail_order");
		Preference.delete(PREF_CLOCK, "tail_brightness");
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

