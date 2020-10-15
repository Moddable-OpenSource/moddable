/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
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

const backgroundSkin = new Skin({
	texture: new Texture("island.png"),
	width: 240, height: 320,
});

const dimSkin = new Skin({
	texture: new Texture("led-dim.png"),
	color: "white",
	width: 40, height: 40,
});

const brightSkin = new Skin({
	texture: new Texture("led-bright.png"),
	color: "white",
	width: 48, height: 48,
});

const outlineSkin = new Skin({
	fill: "transparent", stroke: "white",
	borders: {top: 3, bottom: 3, left: 3, right: 3}
});

const transparentWhiteSkin = new Skin({
	fill: "#FFFFFFCC"
});

class DimmingBehavior extends Behavior {
	onDisplaying(content) {
		this.backlight = new Host.Backlight
		this.adjustBrightness(content);
	}
	onTouchBegan(content, id, x, y) {
		this.onTouchMoved(content, id, x, y);
	}
	onTouchMoved(content, id, x, y) {
		const bounds = content.bounds;
		y = Math.max(bounds.y, y);
		y = Math.min(y, bounds.y + bounds.height);
		content.first.height = (bounds.y + bounds.height) - y;
		this.adjustBrightness(content);
	}
	adjustBrightness(content) {
		const fraction = content.first.height / content.height;
		this.backlight.write(fraction * 100);
	}
};

const ScreenDimmingApplication = Application.template($ => ({
	skin: backgroundSkin,
	contents: [
		Content($, {
			skin: brightSkin, top: 5
		}),
		Container($, {
			width: 100, height: 200, skin: outlineSkin,
			active: true, Behavior: DimmingBehavior,
			contents: [
				Content($, {
					left: 0, right: 0, bottom: 0, height: 100,
					skin: transparentWhiteSkin
				}),
			],
		}),
		Content($, {
			skin: dimSkin, bottom: 10
		}),
	]
}));

export default new ScreenDimmingApplication(null, { displayListLength:4096, touchCount:1 });
