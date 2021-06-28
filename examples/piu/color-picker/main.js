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

import ColorPicker from "color-picker";

const BLACK = "black";
const textStyle = new Style({ color: BLACK, font: "light 42px Open Sans" });
const backgroundSkin = new Skin({ fill: BLACK });

class AppBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;
	}
    onColorPicked(app, color){
    	let r = this.toHex(color.red);
    	let g = this.toHex(color.green);
    	let b = this.toHex(color.blue);
    	let hexColor = `#${r}${g}${b}`;

    	let data = this.data;
		data["COLOR_SAMPLE"].skin = new Skin({ fill: hexColor });
		data["COLOR_SAMPLE"].string = hexColor;
    }
    toHex(val) {
    	return val.toString(16).padStart(2, 0).toUpperCase();
    }
}

const ColorPickerApp = Application.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, skin: backgroundSkin,
	contents: [
		new ColorPicker($, { top: 100 }),
		Label($, {
			anchor: "COLOR_SAMPLE", top: 0, height: 50, left: 0, right: 0,
			skin: new Skin({ fill: "#00FC39" }), string: "#00FC39", style: textStyle
		})
	],
	Behavior: AppBehavior
}));

export default new ColorPickerApp({});
