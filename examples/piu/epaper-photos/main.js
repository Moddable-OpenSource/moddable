/*
 * Copyright (c) 2016-2022 Moddable Tech, Inc.
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
import config from "mc/config";

class PhotoPortBehavior extends Behavior {
	onCreate(port, data) {
		this.data = data;
		this.index = 0;
		this.textures = [1, 2, 3, 4, 5].map(i => new Texture({ path: `${i}.png` }));

		if (globalThis.device?.peripheral?.button?.A) {
			new device.peripheral.button.A({
				onPush() {
					if (this.pressed) {
						port.bubble("onTimeChanged", port);
						port.stop();
						port.time = 0;
						port.start();
					}
				}
			});
		}
	}
	onDisplaying(port) {
		port.interval = 5000;
		port.start();
	}
	onTimeChanged(port) {
		this.index += 1;
		if (this.index >= this.textures.length)
			this.index = 0;
		port.invalidate();
	}
	onDraw(port) {
		let texture = this.textures[this.index];
		port.drawTexture(texture, "black", 0, 0, 0, 0, texture.width, texture.height);
	}
}

const M5PaperApp = Application.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0,
	contents: [
		Port($, { 
			top: 0, bottom: 0, left: 0, right: 0,
			Behavior: PhotoPortBehavior
		})
	]
}));

export default function() {
	if (config.updateMode)
		screen.configure({updateMode: config.updateMode});
	return new M5PaperApp({});
}
