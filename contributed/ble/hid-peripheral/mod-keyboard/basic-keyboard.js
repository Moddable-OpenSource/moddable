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

import {} from "piu/MC";
import {VerticalExpandingKeyboard} from "keyboard";

const OpenSans20 = Style.template({ font: "20px Open Sans", color: "black" });

const HeaderSkin = Skin.template(Object.freeze({fill:"#0082ff"}));
const BackgroundSkin = Skin.template(Object.freeze({ fill:"white" }));

const KeyboardStyle = Style.template(Object.freeze({ font:"18px Roboto", color:"black"  }));
const HeaderStyle = Style.template(Object.freeze({ font:"20px Open Sans", color: "white", horizontal:"left", vertical:"middle" }));

const ModdableLogoTexture = Texture.template(Object.freeze({ path:"moddable-logo-mask.png" }));
const ModdableLogoSkin = Skin.template(Object.freeze({ Texture:ModdableLogoTexture, x:0, y:0, width:88, height:34, color:"white" }));

const Header = Container.template($ => ({
	contents: [
		Content($, { left:76, top:0, bottom:0, Skin:ModdableLogoSkin }),
	]
}));

const KeyboardContainer = Column.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, visible: false,
	contents: [
		Header($, {
			left:0, right:0, height:40, 
			Style:HeaderStyle, Skin:HeaderSkin
		}),
		Content($, {
			left: 0, right: 0, top: 0, bottom: 0
		}),
		Container($, {
			name: "KEYBOARD", left:0, right:0, bottom:0, height:185, 
			Skin:BackgroundSkin
		})
	],
	Behavior: class extends Behavior {
		onCreate(column, data){
			this.data = data;
			this.addKeyboard(column);
		}
		onTouchEnded(column){
			if (1 != column.content("KEYBOARD").length)
				this.addKeyboard(column);
		}
        onKeyUp(column, character) {
            column.bubble("doKeyTap", {character});
        }
		addKeyboard(column) {
			column.content("KEYBOARD").add(VerticalExpandingKeyboard(this.data, {
				style:new KeyboardStyle(), target:column
			}));
		}
	}
}));

class KeyboardUIBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onKeyboardBound(container) {
        let content = container.first;
        while (content) {
            content.visible = true;
            content = content.next;
        }
        
        this.data["NOTPAIRED"].visible = false;
    }
    onKeyboardUnbound(container) {
        let content = container.first;
        while (content) {
            content.visible = false;
            content = content.next;
        }

        this.data["NOTPAIRED"].visible = true;
    }
}

const KeyboardUI = Container.template($ => ({
	Skin:BackgroundSkin,
    left: 0, right: 0, top: 0, bottom: 0,
	Behavior:KeyboardUIBehavior,
	contents: [
		Container($, {
            left: 0, right: 0, top: 0, bottom: 0, anchor: "NOTPAIRED",
            contents: [
                Text($, {
                    left: 0, right: 0, Style: OpenSans20,
                    string: "Not connected to a computer.\nConnect using your operating system's Bluetooth settings."
                })
            ]
        }),
		KeyboardContainer($),
	],
}));

export default {container: KeyboardUI}