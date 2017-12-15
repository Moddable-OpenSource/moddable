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

import WiFi from "wifi";
import { Keyboard, BACKSPACE, SUBMIT } from "keyboard";
import ASSETS from "assets";
import { VerticalScrollerBehavior } from "scroller";

/* -=============================================- */
/* -============ Network list screen ============- */
/* -=============================================- */

const Separator = Content.template($ => ({
	name: "SEPARATOR", top: 0, height: 1, left: 0, right: 0, Skin: ASSETS.SeparatorGraySkin
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
		port.drawString(this.data.ssid, new ASSETS.OpenSans20, ASSETS.BLACK, 0, 8, 150, port.height);
		port.drawTexture(new ASSETS.WiFiStripTexture, ASSETS.BLACK, 228, 6, this.data.xOffset, this.data.yOffset, 28, 28);
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

class NetworkListScreenColumnBehavior extends Behavior {
	onCreate(column, data) {
		this.data = data;
		this.onUpdateNetworkList(column, data.networks);
	}
	onUpdateNetworkList(column, network) {
		let currNetwork = this.data.ssid || "";
		column.empty();
addNetworks:
		for ( ; network; network = network.next) {
			if (network.ssid == currNetwork) {
				if (column.first) {
					column.insert(new Separator, column.first);
					column.insert(new ListItemTemplate(network), column.first);
				} else {
					column.add(new ListItemTemplate(network));
					column.add(new Separator);
				}
				continue addNetworks;
			}
			let ssid = network.ssid.toLowerCase();
			let first;
			let separator = column.content("SEPARATOR");
			if (separator) first = separator.next;
			else first = column.first;
			if (first) {
				for (let item = first; item; item = item.next) {
					if (item.behavior.data.ssid.toLowerCase() > ssid) {
						column.insert(new ListItemTemplate(network), item);
						continue addNetworks;
					}
				}
			}
			column.add(new ListItemTemplate(network));
		}
		column.add(Content(null, {height:40}));
		column.duration = 3000;
		column.time = 0;
		column.start();
	}
	onFinished(column) {
		application.delegate("scan");
	}
}
Object.freeze(NetworkListScreenColumnBehavior.prototype);

export const NetworkListScreen = Container.template($ => ({ 
	left: 0, right: 0, top: 0, bottom: 0, Skin: ASSETS.WhiteSkin,
	contents: [
		Scroller($, {
			left: 32, right: 32, top: 0, bottom: 0, 
			active: true, backgroundTouch: true, clip: true, Behavior: VerticalScrollerBehavior,
			contents:[
				Column($, {
					left: 0, right: 0, top: 40, clip: true,
					Behavior: NetworkListScreenColumnBehavior,
				}),
			],
		}),
		new ASSETS.Header({ title: "Networks" }),
	],
}));

/* -=============================================- */
/* -================ Login screen ===============- */
/* -=============================================- */

class BackArrowBehavior extends Behavior {
	onCreate(content, data) {
		this.data = data;
	}
	onTouchBegan(content, id, x, y, ticks) {
		content.state = 1;
	}
	onTouchEnded(content, id, x, y, ticks) {
		content.state = 0;
		application.delegate("doNext", "NETWORK_LIST_SCAN");
	}
}

export const LoginScreen = Column.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, Skin: ASSETS.WhiteSkin,
	contents: [
		new ASSETS.Header({ title: $.ssid, backArrowBehavior: BackArrowBehavior }),
		Label($, {
			name: "passwordString", left: 0, right: 0, top: 0, height: 32, 
			string: "", Style: ASSETS.OpenSans20
		}),
		Container($, {
			left: 0, right: 0, top: 0, bottom: 0, contents: [
				Keyboard($, {style: new ASSETS.OpenSans18()})
			]
		}),
	],
	Behavior: class extends Behavior {
		onCreate(column, data) {
			this.ssid = $.ssid;
		}
		onPasswordEntered(column, password) {
			let data = { ssid: this.ssid, password };
			application.delegate("doNext", "CONNECTING", data);
		}
		onKeyUp(column, key){
			let str = application.first.first.next.string || "";
			if (key == BACKSPACE) {
				if (str.length > 0) str = str.slice(0, -1);
				else str = "";
			} else if (key == SUBMIT) {
				application.first.delegate("onPasswordEntered", str);
			} else {
				str += key;
			}
			application.first.first.next.string = str;
		}
	}
}));

/* -=============================================- */
/* -========== Connection error screen ==========- */
/* -=============================================- */

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