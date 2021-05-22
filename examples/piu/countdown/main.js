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

import {} from "piu/MC";
import Preference from "preference";

const digitsTexture = new Texture("digits.png");

class PortBehavior extends Behavior {
	onCreate(port, delta) {
		let string = Preference.get("config", "date") 
		if (!string) {
			const date = new Date();
			string = date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + (date.getDate() + 1); // tomorrow
		}
		this.date = new Date(string);
		this.hue0 = 0;
		this.hue1 = 90;
		this.hue2 = 180;
		this.hue3 = 270;
	}
	onDisplaying(port) {
		port.interval = 50;
		port.start();
	}
	onDraw(port, x, y, w, h) {
		let time = (this.date.valueOf() - Date.now()) / 1000;
		if (time < 0) time = 0;
		let seconds = Math.floor(time % 60);
		time = Math.floor(time / 60);
		let minutes = Math.floor(time % 60);
		time = Math.floor(time / 60);
		let hours = Math.floor(time % 24);
		time = Math.floor(time / 24);
		let days = Math.floor(time);
		if (days > 99) days = 99;
		
		let color0 = hsl(this.hue0, 1, 0.75);
		let color1 = hsl(this.hue1, 1, 0.75);
		let color2 = hsl(this.hue2, 1, 0.75);
		let color3 = hsl(this.hue3, 1, 0.75);
		
		port.drawTexture(digitsTexture, color0, 0, 0, Math.floor(days / 10) * 32, 0, 32, 48);
		port.drawTexture(digitsTexture, color0, 32, 0, (days % 10) * 32, 0, 32, 48);
		port.drawTexture(digitsTexture, color0, 0, 48, 0, 48, 64, 16);

		port.drawTexture(digitsTexture, color1, 64, 0, Math.floor(hours / 10) * 32, 0, 32, 48);
		port.drawTexture(digitsTexture, color1, 96, 0, (hours % 10) * 32, 0, 32, 48);
		port.drawTexture(digitsTexture, color1, 64, 48, 64, 48, 64, 16);

		port.drawTexture(digitsTexture, color2, 0, 64, Math.floor(minutes / 10) * 32, 0, 32, 48);
		port.drawTexture(digitsTexture, color2, 32, 64, (minutes % 10) * 32, 0, 32, 48);
		port.drawTexture(digitsTexture, color2, 0, 112, 128, 48, 64, 16);

		port.drawTexture(digitsTexture, color3, 64, 64, Math.floor(seconds / 10) * 32, 0, 32, 48);
		port.drawTexture(digitsTexture, color3, 96, 64, (seconds % 10) * 32, 0, 32, 48);
		port.drawTexture(digitsTexture, color3, 64, 112, 192, 48, 64, 16);

		this.hue0 += 1;
		this.hue1 += 1;
		this.hue2 += 1;
		this.hue3 += 1;
	}
	onTimeChanged(port) {
		port.invalidate();
	}
};

let CountdownApplication = Application.template($ => ({
	skin: new Skin({ fill:"black" }),
	contents: [
		Port($, { width:128, height:128, Behavior:PortBehavior }),
	]
}));

export default new CountdownApplication(null, { commandListLength:4096, displayListLength:4096, touchCount:0 });
