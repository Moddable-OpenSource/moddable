/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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
import {HorizontalExpandingKeyboard} from "keyboard";
import {KeyboardField} from "common/keyboard";
import ASSETS from "assets";

class VerticalScrollerBehavior extends Behavior {
	onTouchBegan(scroller, id, x, y) {
		this.anchor = scroller.scroll.y;
		this.y = y;
		this.waiting = true;
	}
	onTouchMoved(scroller, id, x, y, ticks) {
		let delta = y - this.y;
		if (this.waiting) {
			if (Math.abs(delta) < 8)
				return;
			this.waiting = false;
			scroller.captureTouch(id, x, y, ticks);
		}
		scroller.scrollTo(0, this.anchor - delta);
	}
}

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
			if (network.variant <= 1) 
				continue addNetworks;
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
			left: 0, right: 0, top: 0, bottom: 0, 
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

const PASSWORDMODE = false;	//Set to true to replace input with asterisks, false for clear text. 
const TRANSITION = true;	//Set to true to transition keyboard in and out. 

const KeyboardContainer = Column.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, active: true,
	contents:[
		KeyboardField($, {
			anchor: "FIELD", password: PASSWORDMODE, left: 32, right: 0, top: 0, bottom: 0,
			Skin: ASSETS.WhiteSkin, Style: ASSETS.OpenSans20, visible: false
		}),
		Container($, {
			anchor: "KEYBOARD", left:0, right:0, bottom:0, height:164, 
			Skin:ASSETS.WhiteSkin
		}),
	],
	Behavior: class extends Behavior {
		onCreate(column, data){
			this.data = data;
			this.addKeyboard();
		}
		onTouchEnded(column){
			if (1 != this.data["KEYBOARD"].length)
				this.addKeyboard();
		}
		addKeyboard() {
			this.data["KEYBOARD"].add(HorizontalExpandingKeyboard(this.data, {
				style:new ASSETS.OpenSans20(), target:this.data["FIELD"], doTransition:TRANSITION
			}));
		}
	}
}));

export const LoginScreen = Column.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, Skin: ASSETS.WhiteSkin,
	contents: [
		new ASSETS.Header({ title: $.ssid, backArrowBehavior: BackArrowBehavior }),
		KeyboardContainer($),
	],
	Behavior: class extends Behavior {
		onCreate(column, data) {
			this.ssid = $.ssid;
			this.data = data;
		}
		onKeyboardRowsContracted(column) {
			// keyboard rows contracted back to 1x view
		}
		onKeyboardRowsExpanded(column) {
			// keyboard rows expanded
		}
		onKeyboardOK(column, string) {
			trace(`String is: ${string}\n`);
			this.password = string;
		}
		onKeyboardTransitionFinished(column, out) {
			if (out) {
				let data = { ssid: this.ssid, password: this.password };
				application.delegate("doNext", "CONNECTING", data);
			}
			else {
				this.data["FIELD"].visible = true;
			}
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
			string: "Please confirm password is correct"
		})
	],
}));
