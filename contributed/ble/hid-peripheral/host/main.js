/*
 * Copyright (c) 2022 Moddable Tech, Inc.
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

import {default as KeyboardService, KEYINFO, HID_MODIFIERS} from "keyboardService";
import Modules from "modules";

const WHITE = "white";
const BLACK = "black";

const BackgroundSkin = Skin.template({ fill: WHITE });
const OpenSans24 = Style.template({ font: "24px Open Sans", color: BLACK });

class MediaBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;

		if (Modules.has("UI")) {
			globalThis.modExport = Modules.importNow("UI");
			this.UI = new modExport.container({hidKeys: KEYINFO, modifiers: HID_MODIFIERS})
			application.add(this.UI);
		} else {
			application.add(new NoModUI());
		}

		this.ble = new KeyboardService({
			onKeyboardBound: () => {
				if (this.UI !== undefined)
					this.UI.delegate("onKeyboardBound");
			},
			onKeyboardUnbound: () => {
				if (this.UI !== undefined)
					this.UI.delegate("onKeyboardUnbound");
			}
		});
	}
	doKeyDown(application, options){
		this.ble.onKeyDown(options);
	}
	doKeyUp(application, options) {
		this.ble.onKeyUp(options);
	}
	doKeyTap(application, options) {
		this.ble.onKeyTap(options);
	}
	
}

const NoModUI = Container.template($ => ({
	Skin: BackgroundSkin, left: 0, right: 0, top: 0, bottom: 0,
	contents: [
		Text($, {
			height: 200, top: 50, left: 0, right: 0, Style: OpenSans24,
			string: "Please install a UI mod from the Moddable Store." 
		})
	]
}));

const MediaController = Application.template($ => ({
	Skin: BackgroundSkin,
	Behavior: MediaBehavior
}));

export default function () {
	return new MediaController({  }, { commandListLength: 2448, displayListLength: 3072, touchCount: 1 });
}