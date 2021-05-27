/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
import {HorizontalExpandingKeyboard} from "keyboard";
import {KeyboardField} from "common/keyboard";

const PASSWORDMODE = false;	//Set to true to replace input with asterisks, false for clear text. 
const TRANSITION = true;	//Set to true to transition keyboard in and out. 

const HeaderSkin = Skin.template(Object.freeze({fill:"#0082ff"}));
const FieldSkin = Skin.template(Object.freeze({fill:"white"}));
const BackgroundSkin = Skin.template(Object.freeze({ fill:"white" }));

const KeyboardStyle = Style.template(Object.freeze({ font: "18px Roboto", color: "black" }));
const FieldStyle = Style.template(Object.freeze({ font: "20px Open Sans", color: "black", horizontal:"left", vertical:"middle" }));
const HeaderStyle = Style.template(Object.freeze({ font: "20px Open Sans", color: "white", horizontal:"left", vertical:"middle" }));

const ModdableLogoTexture = Texture.template(Object.freeze({ path:"moddable-logo-mask.png" }));
const ModdableLogoSkin = Skin.template(Object.freeze({ Texture:ModdableLogoTexture, x:0, y:0, width:88, height:34, color:"white" }));

const Header = Container.template($ => ({
	contents: [
		Content($, { left:116, top:0, bottom:0, Skin:ModdableLogoSkin }),
	]
}));

const KeyboardContainer = Column.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true,
	contents:[
		Header($, {
			anchor: "HEADER", left:0, right:0, height:40, 
			Style:HeaderStyle, Skin:HeaderSkin
		}),
		KeyboardField($, {
			anchor:"FIELD", password:PASSWORDMODE, left:32, right:0, top:0, bottom:0,
			Skin:FieldSkin, Style:FieldStyle, visible:false
		}),
		Container($, {
			anchor: "KEYBOARD", left:0, right:0, bottom:0, height:164, 
			Skin:BackgroundSkin
		}),
	],
	Behavior: class extends Behavior {
		onCreate(column, data){
			this.data = data;
			this.addKeyboard();
		}
		onTouchEnded(column){
			if (1 != this.data.KEYBOARD.length)
				this.addKeyboard();
		}
		addKeyboard() {
			this.data.KEYBOARD.add(HorizontalExpandingKeyboard(this.data, {
				style:new KeyboardStyle(), target:this.data.FIELD, doTransition:TRANSITION
			}));
		}
	}
}));

class KeyboardAppBehavior extends Behavior {
	onCreate(application, data) {
		this.data = data;
	}
	onKeyboardRowsContracted(application) {
		// keyboard rows contracted back to 1x view
	}
	onKeyboardRowsExpanded(application) {
		// keyboard rows expanded
	}
	onKeyboardOK(application, string) {
		trace(`String is: ${string}\n`);
		this.data.FIELD.visible = false;
	}
	onKeyboardTransitionFinished(application, out) {
		if (out) {
			let keyboard = this.data.KEYBOARD;
			keyboard.remove(keyboard.first);
		}
		else {
			this.data.FIELD.visible = true;
		}
	}
}
Object.freeze(KeyboardAppBehavior.prototype);

const KeyboardApp = Application.template($ => ({
	active:true, Skin:BackgroundSkin,
	Behavior:KeyboardAppBehavior,
	contents: [
		KeyboardContainer($),
	],
}));

export default function() {
	new KeyboardApp({}, { commandListLength:2448, displayListLength:2600, touchCount:1 });
}
