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

export default function() {
	const backgroundTexture = new Texture("island.png");
	const backgroundSkin = new Skin({
		texture: backgroundTexture,
		width:240, height:320,
	});

	const dimTexture = new Texture("led-dim.png");
	const dimSkin = new Skin({
		texture: dimTexture,
		color: "white",
		width:40, height:40,
	});

	const brightTexture = new Texture("led-bright.png");
	const brightSkin = new Skin({
		texture: brightTexture,
		color: "white",
		width:48, height:48,
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
		onTouchBegan(content, id, x, y, ticks) {
			let bar = content.next;
			bar.height = 180-y+bar.coordinates.bottom;
			this.adjustBrightness(content);
		}
		onTouchMoved(content, id, x, y, ticks) {
			if (y < 71) return;
			let bar = content.next;
			bar.height = 180-y+bar.coordinates.bottom;
			this.adjustBrightness(content);
		}
		adjustBrightness(content) {
			let bar = content.next;
			let fraction = bar.height/178;
			if (fraction < 0) fraction = 0;
			else if (fraction > 1) fraction = 1;
			this.backlight.write(fraction * 100);
		}
	};

	const ScreenDimmingApplication = Application.template($ => ({
		skin: backgroundSkin,
		contents: [
			Content($, {
				skin: brightSkin, top: 5
			}),
			Content($, {
				width: 100, height: 200, skin: outlineSkin,
				active: true, Behavior: DimmingBehavior
			}),
			Content($, {
				left: 80, right: 80, bottom: 71, height: 178 / 2,
				skin: transparentWhiteSkin
			}),
			Content($, {
				skin: dimSkin, bottom: 10
			}),
		]
	}));

	new ScreenDimmingApplication(null, { displayListLength:4096, touchCount:1 });
}

if (!globalThis.Host || !Host.Backlight)
	throw new Error("backlight unsupported");
