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

import ASSETS from "assets";

export const ConnectionErrorScreen = Column.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, Skin: ASSETS.WhiteSkin, Style: ASSETS.OpenSans20, contents: [
		new ASSETS.Header({ title: "Networks", 
			backArrowBehavior: class extends Behavior {
				onCreate(content) {
					this.data = $;
				}
				onTouchBegan(content, id, x, y, ticks) {
					content.state = 1;
				}
				onTouchEnded(content, id, x, y, ticks) {
					content.state = 0;
					if ("password" in this.data) application.delegate("doNext", "LOGIN", this.data);
					else application.delegate("doNext", "NETWORK_LIST_SCAN");
				}
			} 
		}),
		Label($, {
			top: 12, height: 24, left: 0, right: 0, Style: ASSETS.CenterStyle,
			string: "Unable to join network:"
		}),
		Label($, {
			top: 0, height: 24, left: 0, right: 0, Style: ASSETS.CenterStyle,
			string: $.ssid			// For some reason this didn't work with a text object...
		}),
		Content($, { top: 20, Skin: ASSETS.ErrorSkin }),
		Text($, {
			top: 20, height: 32, left: 0, right: 0, Style: ASSETS.OpenSans16,
			string: "Please confirm that password\nis correct"
		})
	],
}));
