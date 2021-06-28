/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
/*
 *   This app should be built with rotation set to 90 degrees:
 *      mcconfig -d -m -p esp -r 90
 *
 *   For details on localization support, refer to the localization document:
 *      https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/localization.md 
 */

import {} from "piu/MC";
import WipeTransition from "piu/WipeTransition";

const BLACK = "black";
const BLUE = "blue";
const WHITE = "white";

global.locals = new Locals;
global.localize = function(it) {
    return locals.get(it);
}  

const backgroundSkin = new Skin({ fill:BLACK });
const languageButtonSkin = new Skin({ fill:BLUE, left:20, right:20, top:20, bottom:20 });
const settingsSkin = { texture:{ path:"settings.png" }, x:0, y:0, width:34, height:34 };
const textStyle = new Style({ font:"OpenSans-Regular-24", color:WHITE });

class LanguageButtonBehavior extends Behavior {
	onCreate(label, data) {
		this.data = data;
	}
	onTouchBegan(label, id, x, y) {
		application.distribute("onLanguageChange", this.data.code);
	}
}

const LanguageButton = Label.template($ => ({
	width:200, left:-10, right:-10, top:0, bottom:0, skin:languageButtonSkin, active:true, string:localize($.string),
	Behavior: LanguageButtonBehavior
}));

const LanguagesContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:backgroundSkin,
	contents: [
		Column($, {
			left:0, right:0, top:0, bottom:0,
			contents: [
				Row($, {
					left:0, right:0, top:0, bottom:0,
					contents: [
						LanguageButton({ string:"English", code:"en" }),
						LanguageButton({ string:"French", code:"fr" }),
					]
				}),
				Row($, {
					left:0, right:0, top:0, bottom:0,
					contents: [
						LanguageButton({ string:"German", code:"de" }),
						LanguageButton({ string:"Spanish", code:"es" }),
					]
				})
			]
		})
	],
}));

class SettingsButtonBehavior extends Behavior {
	onTouchBegan(label, id, x, y) {
		application.distribute("onSettings");
	}
}

const SettingsButton = Content.template($ => ({
	skin:settingsSkin, active:true,
	Behavior: SettingsButtonBehavior
}));

const InstructionsContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:backgroundSkin,
	contents: [
		SettingsButton($, { right:0, top:0 }),
		Text($, {
			left:10, right:10, string:localize("TAP_TO_CHANGE_LANGUAGE")
		})
	],
}));

class LocalizationBehavior extends Behavior {
	onLanguageChange(application, code) {
		locals.language = code;
		let transition = new WipeTransition(300, Math.quadEaseOut, "top");
		application.run(transition, application.first, new InstructionsContainer({}));
	}
	onSettings(application) {
		let transition = new WipeTransition(300, Math.quadEaseOut, "bottom");
		application.run(transition, application.first, new LanguagesContainer({}));
	}
}

let LocalizationApplication = Application.template($ => ({
	style:textStyle,
	Behavior:LocalizationBehavior,
	contents: [
		Container($, {
			left:0, right:0, top:0, bottom:0,
			contents: [
				InstructionsContainer({})
			]
		}),
	]
}));

export default function () {
	new LocalizationApplication(null, { touchCount:1, displayListLength: 2048 });
}


