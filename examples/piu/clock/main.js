/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */
import {} from "piu/MC";

const WHITE = "white";
const BLACK = "black";

const backgroundSkin = new Skin({ fill: WHITE });
const textStyle = new Style({ font: "light 36px Open Sans", color: BLACK });

class ClockBehavior extends Behavior {
	onDisplaying(label) {
		this.colon = true;
		this.update(label);
		label.interval = 500;
		label.start();
	}
	onTimeChanged(label) {
		this.colon = !this.colon;
		this.update(label);
	}
	update(label) {
		let d = new Date();
		let h = d.getHours();
		if (h == 0) 
			h = 12;
		else if (h > 12)
			h -= 12;
		let m = d.getMinutes();
		m = m.toString(10).padStart(2, "0");
		label.string = `${h}${(this.colon)? ":" : " "}${m}`;
	}
}

let ClockApplication = Application.template($ => ({
	skin: backgroundSkin, style: textStyle,
	contents: [
		Label($, { 
			Behavior: ClockBehavior 
		})
	]
}));

export default new ClockApplication();