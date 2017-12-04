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
import { Keyboard, BACKSPACE, SUBMIT } from "keyboard";

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

function gotKey(key){
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
				new Keyboard({style: new ASSETS.OpenSans18(), callback: gotKey})
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
	}
}));
