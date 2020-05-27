/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Poco from "commodetto/Poco";
import BufferOut from "commodetto/BufferOut";
import Resource from "Resource";
import parseBMP from "commodetto/parseBMP";

const BLACK = "black";
const WHITE = "white";

const textStyle = new Style({ color: BLACK, font: "light 42px Open Sans" });
const backgroundSkin = new Skin({ fill: BLACK });

const colorIndicatorTexture = new Texture({ path: "color-indicator.png" });
const colorIndicatorSkin = new Skin({
  texture: colorIndicatorTexture, color: WHITE,
  x: 0, y: 0, width: 36, height: 36 
});

const colorCircleTexture = new Texture({ path: "color-color.bmp" });
const colorCircleSkin = new Skin({
  texture: colorCircleTexture,
  x: 0, y: 0, width: 152, height: 152 
});

const colorLimiterTexture = new Texture({ path: "color-limiter.png" });
const colorLimiterSkin = new Skin({
  texture: colorLimiterTexture, color: WHITE,
  x: 0, y: 0, width: 185, height: 185 
});

class ColorWheelBehavior extends Behavior {
	onCreate(wheel, data) {
		this.data = data;
	}
	onTouchBegan(wheel, id, x, y, ticks) {
		this.update(wheel, x, y);
	}
	onTouchMoved(wheel, id, x, y, ticks) {
		this.update(wheel, x, y);
	}
	onTouchEnded(wheel, id, x, y, ticks) {
		this.update(wheel, x, y);
	}
	update(wheel, x, y) {
		let newx = x-wheel.x-92;
		let newy = y-wheel.y-92;
		let z = newx*newx + newy*newy;
		if (z > 5184) {
			let j = 72 / Math.sqrt(z);
			newx *= j;
			newy *= j;
		}
		newx += wheel.x+92;
		newy += wheel.y+92;
		let color = getColor(newx-wheel.x-18, newy-wheel.y-18);
		this.data["COLOR_INDICATOR"].position = {x: newx-18, y: newy-18};
		if (color != "#000000") application.delegate("changeColor", color);
	}
}

class AppBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;
	}
	changeColor(application, color) {
		let data = this.data;
		data["COLOR_SAMPLE"].skin = new Skin({ fill: color });
		data["COLOR_SAMPLE"].string = color;
	}
}
Object.freeze(AppBehavior.prototype);

const ColorPickerApp = Application.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, skin: backgroundSkin,
	contents: [
		Content($, {
			top: 100, skin: colorCircleSkin
		}),
		Content($, {
			top: 85, skin: colorLimiterSkin,
			active: true, Behavior: ColorWheelBehavior
		}),
		Content($, {
			anchor: "COLOR_INDICATOR", top: 136, height: 36, left: 56, width: 36, skin: colorIndicatorSkin
		}),
		Label($, {
			anchor: "COLOR_SAMPLE", top: 0, height: 50, left: 0, right: 0,
			skin: new Skin({ fill: "#00FC39" }), string: "#00FC39", style: textStyle
		})
	],
	Behavior: AppBehavior
}));

let wheel = parseBMP(new Resource("color-color.bmp"));
let poco = new Poco(screen);
let offscreen = new BufferOut({width: 1, height: 1, pixelFormat: poco.pixelsOut.pixelFormat});
let pocoOff = new Poco(offscreen);

function getColor(x, y) {
	pocoOff.begin();
		pocoOff.drawBitmap(wheel, 0, 0, x, y, 1, 1);
	pocoOff.end();

	let buff = pocoOff.pixelsOut.buffer;
	let view = new DataView(buff);
	let testcolor = view.getUint8(1) << 8 | view.getUint8();

	let r = ((testcolor >> 11) & 0x1F);
	let g = ((testcolor >> 5) & 0x3F);
	let b = (testcolor & 0x1F);
	r = r << 3 | r >> 2;
	g = g << 2 | g >> 6;
	b = b << 3 | b >> 2;

	return "#" + r.toString(16).padStart(2, 0) + g.toString(16).padStart(2, 0)  + b.toString(16).padStart(2, 0) 
}

export default new ColorPickerApp({});
