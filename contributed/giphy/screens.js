/*
 * Copyright (c) 2016-2021 Moddable Tech, Inc.
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

import Flash from "flash";
import {VerticalExpandingKeyboard} from "keyboard";
import {KeyboardField} from "common/keyboard";

const BLACK = "black";
const WHITE = "white";
const DARK_GRAY = "#666666";
const LIGHT_GRAY = "#e1e1e1";

const GiphyLogoTexture = Texture.template(Object.freeze({ path: "giphy-logo.png" }));
const GiphyLogoSkin = Skin.template(Object.freeze({
	Texture: GiphyLogoTexture,
	height: 32, width: 190
}));
const BackArrowTexture = Texture.template(Object.freeze({ path: "back-arrow.png" }));
const BackArrowSkin = Skin.template(Object.freeze({
	Texture: BackArrowTexture,
	color: WHITE,
	height: 24, width: 14
}));

const HeaderSkin = Skin.template(Object.freeze({fill: BLACK}));
const FieldSkin = Skin.template(Object.freeze({fill: WHITE}));
const BackgroundSkin = Skin.template(Object.freeze({fill: BLACK}));
const ProgressBarSkin = Skin.template(Object.freeze({ fill: [DARK_GRAY, LIGHT_GRAY]}));

const KeyboardStyle = Style.template(Object.freeze({font:"18px Open Sans", color: [BLACK, WHITE], horizontal: "center"}));
const FieldStyle = Style.template(Object.freeze({font:"light 42px Open Sans", color: BLACK, horizontal:"left", vertical:"middle"}));
const TitleStyle = Style.template(Object.freeze({font:"18px Open Sans", color: WHITE, horizontal: "right" }));

class KeyboardScreenBehavior extends Behavior {
	onCreate(column, data){
		this.data = data;
		this.addKeyboard();
	}
	onTouchEnded(column){
		if (1 != this.data.KEYBOARD.length)
			this.addKeyboard();
	}
	addKeyboard() {
		this.data.KEYBOARD.add(VerticalExpandingKeyboard(this.data, {
			style:new KeyboardStyle(), target:this.data.FIELD, doTransition: true
		}));
	}
	onKeyboardRowsContracted(column) {
		// keyboard rows contracted back to 1x view
	}
	onKeyboardRowsExpanded(column) {
		// keyboard rows expanded
	}
	onKeyboardOK(column, string) {
		this.data.FIELD.visible = false;
		application.delegate("onStringEntered", string);
	}
	onKeyboardTransitionFinished(column, out) {
		if (out) {
			let keyboard = this.data.KEYBOARD;
			keyboard.remove(keyboard.first);
		}
		else {
			this.data.FIELD.visible = true;
		}
	}
}

class CustomKeyboardFieldBehavior extends Behavior {
	onCreate(container, $, data) {
		container.duration = 500;
		this.string = "";
	}
	onDisplaying(container) {
		this.field = container.first;
		this.cursor = container.last;
	}
	onKeyUp(container, key) {
		let field = this.field;
		let cursor = this.cursor;
		let password = this.password;
		let style = container.style;
		let width;
		if ('\r' == key) {
			application.distribute("onKeyboardOK", this.string);
			this.string = "";
		}
		else if ('\b' == key)
			this.string = this.string.slice(0, -1);
		else
			this.string += key;
		let length = this.string.length;
		width = style.measure(this.string).width;
		cursor.x = field.x + width + 2;
		field.string = this.string;
		width = width + 2 + cursor.width;
		container.width = width;
		if (width > 180)
			container.x = 210 - width;
		else
			container.x = 30;
	}
}

const KeyboardScreen = Column.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0, Skin: BackgroundSkin,
	contents: [
		Label($, {
			top: 0, height: 36, left: 0, right: 0,
			Skin: HeaderSkin, Style: KeyboardStyle, state: 1, string: "Search"
		}),
		Container($, {
			top: 0, bottom: 0, left: 0, right: 0, Skin: FieldSkin,
			contents: [
				KeyboardField($, {
					anchor: "FIELD", password: false, 
					left: 30, width: 100, top: 10, bottom: 10,
					Skin: FieldSkin, Style: FieldStyle, visible: false,
					Behavior: CustomKeyboardFieldBehavior
				}),
			]
		}),
		Container($, {
			anchor: "KEYBOARD", left: 0, right: 0, bottom: 0, height: 185, 
			Skin: BackgroundSkin
		})
	],
	active: true, Behavior: KeyboardScreenBehavior
}));

class GIFImageBehavior extends Behavior {
	animate(image) {
		image.start();
	}
	onFinished(image) {
		image.time = 0;
		image.start();
	}
};

class GIFScreenBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
		this.downloading = true;
 	}
	updateTitle(container, title) {
		let data = this.data;
		let header = data["HEADER"];
		let index = title.indexOf("by");
		let style = container.style;
		if (index > -1) {
			let string = title.slice(0, index);
			let gifTitle = new Label(data, {
				top: 3, left: 26, right: 0, string
			});
			let author = new Label(data, {
				top: 3+style.measure(string).height, left: 26, right: 0, string: title.slice(index)
			});
			header.add(gifTitle);
			header.add(author);
		} else {
			let gifTitle = new Label(data, {
				top: 3, left: 26, right: 0, string: title
			});
			header.add(gifTitle);
		}
		header.height += 2;
	}
	onUpdateProgress(container, progress) {
		let background = this.data["PROGRESS_BAR"];
		let filler = background.first;
		filler.width = background.width * progress;
	}
	onError(container, error) {
		this.data["LOADING"].string = error;
	}
	showFirstFrame(container) {
		container.remove(this.data["LOADING"]);
		delete this.data["LOADING"]
		application.purge();
		let GIF = new GIFImage(this.data, { 
			anchor: "GIF", top: 65,
			buffer: (new Flash("xs")).map(), 
			Behavior:GIFImageBehavior 
		});
		container.insert(GIF, this.data["FOOTER"]);
	}
	onGIFDownloaded(container) {
		if (this.data["GIF"]) {
			container.remove(this.data["GIF"]);
		}
		let GIF = new GIFImage(this.data, { 
			anchor: "GIF", top: 65,
			buffer: (new Flash("xs")).map(), 
			Behavior:GIFImageBehavior 
		});
		container.insert(GIF, this.data["FOOTER"]);
		this.downloading = false;
		container.remove(this.data["PROGRESS_BAR"]);
		delete this.data["PROGRESS_BAR"]
		this.showControls = false;
		container.time = 0;
		container.duration = 200;
		container.start();
		this.data["GIF"].delegate("animate");
	}
	onTouchEnded(container, id, x, y, ticks) {
		if ((x <= 40) && (y <= 50)) {
			if (this.downloading) application.delegate("stopDownload");
			else application.delegate("stopGIF");
		} else {
			this.showControls = !this.showControls;
			container.time = 0;
			container.duration = 200;
			container.start();
		}
	}
	onTimeChanged(container) {
		let data = this.data;
		let fraction = Math.quadEaseOut(container.fraction);
		if (this.showControls) {
			data["HEADER"].y = -50 + fraction*50;
			data["FOOTER"].y = 320 - fraction*34;
		} else {
			data["HEADER"].y = 0 - fraction*50;
			data["FOOTER"].y = 286 + fraction*34;
		}
	}
	onFinished(container) {
		let data = this.data;
		if (this.showControls) {
			data["HEADER"].y = 0;
			data["FOOTER"].y = 286;
		} else {
			data["HEADER"].y = -50;
			data["FOOTER"].y = 320;
		}
	}
}

const GIFScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0, 
	Skin: BackgroundSkin, Style: TitleStyle,
	contents: [
		Container($, {
			anchor: "HEADER", height: 50, top: 0, left: 12, right: 12,
			contents: [
				Content($, {
					left: 0, top: 14, Skin: BackArrowSkin
				})
			]
		}),
		Container($, {
			anchor: "PROGRESS_BAR", left: 20, top: 56, height: 4, width: 200, Skin: ProgressBarSkin,
			contents: [
				Content($, {
					left: 0, height: 4, width: 0, Skin: ProgressBarSkin, state: 1
				})
			]		
		}),
		Text($, {
			anchor: "LOADING", left: 20, right: 20,
			Style: KeyboardStyle, state: 1, string: "Loading..."
		}),
		Content($, {
			anchor: "FOOTER", left: 25, top: 286, Skin: GiphyLogoSkin
		})
	],
	active: true, Behavior: GIFScreenBehavior
}))

export default {
	KeyboardScreen,
	GIFScreen
}
