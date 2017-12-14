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

import {
	VerticalScrollerBehavior
} from "scroller";

import WiFi from "wifi";

const Separator = Content.template($ => ({
	top: 0, height: 1, left: 0, right: 0, Skin: ASSETS.SeparatorGraySkin
}))

class ListItemBehavior extends Behavior {
	onCreate(port, data) {
		this.data = {
			ssid: data.ssid,
			yOffset: (data.variant < 0)? 0: 27,
			xOffset: Math.abs(data.variant) * 28,
			state: 0,
		}
	}
	onDraw(port) {
		port.fillColor((this.data.state)? ASSETS.LIGHTEST_GRAY: ASSETS.WHITE, 0, 0, port.width, port.height);
		port.drawString(this.data.ssid, new ASSETS.OpenSans20, ASSETS.BLACK, 32, 8, 150, port.height);
		port.drawTexture(new ASSETS.WiFiStripTexture, ASSETS.BLACK, 260, 6, this.data.xOffset, this.data.yOffset, 28, 28);
	}
	onTouchBegan(port, id, x, y, ticks) {
		this.data.state = 1;
		port.invalidate();
		this.startY = y;
	}
	onTouchCancelled(port) {
		this.data.state = 0;
		port.invalidate();
	}
	onTouchEnded(port, id, x, y, ticks) {
		this.data.state = 0;
		port.invalidate();
		if (this.data.yOffset > 0) application.delegate("doNext", "LOGIN", { ssid: this.data.ssid});
		else application.delegate("doNext", "CONNECTING", { ssid: this.data.ssid });
	}
}

const ListItemTemplate = Port.template($ => ({
	active: true, left: 0, right: 0, height: 40, 
	Behavior: ListItemBehavior
}));

 export const NetworkListScreen = Scroller.template($ => ({ 
	left: 0, right: 0, top: 0, bottom: 0, Skin: ASSETS.WhiteSkin,
	active: true, backgroundTouch: true, Behavior: VerticalScrollerBehavior,
	contents: [
		Column($, {
			left: 0, right: 0, top: 40, clip: true,
			Behavior: class extends Behavior {
				onCreate(column) {
					let network = $.networks;
					delete $.networks;
					for ( ; network; network = network.next) {
						if (network.ssid != $.ssid) column.add(new ListItemTemplate(network));
						else {
							if (column.length > 0) column.insert(new Separator, column.first); 
							else column.add(new Separator);
							column.insert(new ListItemTemplate(network), column.first);
						}
					}
					column.add(Content(null, {height:40}))
				}
			}
		}),
		new ASSETS.Header({ title: "Networks" }),
	]
}));
