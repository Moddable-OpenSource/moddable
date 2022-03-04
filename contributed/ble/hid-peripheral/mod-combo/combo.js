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

const WHITE = "white";

const OpenSans20 = Style.template({ font: "20px Open Sans", color: "black" });
const ButtonSkin = Skin.template({ fill: ["#0033cc", "#668cff"] });

const volumeDownTexture = new Texture({ path: "down.png" });
const volumeDownSkin = new Skin({
	texture: volumeDownTexture, color: WHITE,
	height: 50, width: 50
});

const volumeUpTexture = new Texture({ path: "up.png" });
const volumeUpSkin = new Skin({
	texture: volumeUpTexture, color: WHITE,
	height: 50, width: 50
});

const playPauseTexture = new Texture({ path: "playpause.png" });
const playPauseSkin = new Skin({
	texture: playPauseTexture, color: WHITE,
	height: 50, width: 50
});

const muteTexture = new Texture({ path: "mute.png" });
const muteSkin = new Skin({
	texture: muteTexture, color: WHITE,
	height: 50, width: 50
});

const forwardTexture = new Texture({ path: "forward.png" });
const forwardSkin = new Skin({
	texture: forwardTexture, color: WHITE,
	height: 50, width: 50
});

const backTexture = new Texture({ path: "back.png" });
const backSkin = new Skin({
	texture: backTexture, color: WHITE,
	height: 50, width: 50
});

const BackgroundSkin = Skin.template(Object.freeze({ fill:"white" }));
const KeyboardStyle = Style.template(Object.freeze({ font:"18px Roboto", color:"black"  }));

const KEY_CODES = {
    VOLUME_DOWN: 1,
    VOLUME_UP: 2,
    MUTE: 3,
    SHUFFLE: 4,
    PLAYPAUSE: 5,
    FORWARD: 6,
    BACK: 7
}

class ButtonBehavior extends Behavior {
	onCreate(button, data) {
		this.data = data;
	}
	onTouchBegan(button) {
		button.state = 1;
		if (this.data.event)
            button.bubble("onMediaKeyDown", this.data.event);
	}
	onTouchEnded(button) {
		button.state = 0;
		if (this.data.event)
			button.bubble("onMediaKeyUp", this.data.event);
	}
}

const Button = Container.template($ => ({
	active: true, width: 50, height: 50, Skin: ButtonSkin,
	contents: [
		Content($, {skin: $.skin})
	],
	Behavior: ButtonBehavior
}));

const KeyboardContainer = Column.template($ => ({
	left:0, right:0, top:0, bottom:0, active:true, visible: false,
	contents: [
		Container($,  {
			left: 0, right: 0, top: 0, height: 135,
			contents: [
				Button({event: KEY_CODES.VOLUME_DOWN, skin: volumeDownSkin}, {
					top: 10, left: 15
				}),
				Button({event: KEY_CODES.VOLUME_UP, skin: volumeUpSkin}, {
					top: 10, right: 15
				}),
				Button({event: KEY_CODES.MUTE, skin: muteSkin}, {
					top: 10, left: 95
				}),
				Button({event: KEY_CODES.PLAYPAUSE, skin: playPauseSkin}, {
					top: 75, left: 95
				}),
				Button({event: KEY_CODES.FORWARD, skin: forwardSkin}, {
					top: 75, right: 15
				}),
				Button({event: KEY_CODES.BACK, skin: backSkin}, {
					top: 75, left: 15
				})
			]
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
	translateKey(container, code) {
        let hid = -1;
        const hidKeys = this.data.hidKeys;

        switch (code) {
            case KEY_CODES.VOLUME_DOWN:
                hid = hidKeys.VOLUME_DOWN.HID;
                break;
            case KEY_CODES.VOLUME_UP:
                hid = hidKeys.VOLUME_UP.HID;
                break;
            case KEY_CODES.MUTE:
                hid = hidKeys.MUTE.HID;
                break;
            case KEY_CODES.SHUFFLE:
                hid = hidKeys.SHUFFLE.HID;
                break;
            case KEY_CODES.PLAYPAUSE:
                hid = hidKeys.PLAYPAUSE.HID;
                break;
            case KEY_CODES.FORWARD:
                hid = hidKeys.FORWARD.HID;
                break;
            case KEY_CODES.BACK:
                hid = hidKeys.BACK.HID;
                break;
            default:
                trace("invalid key\n");
        }
        return hid;
    }
    onMediaKeyDown(container, code) {
        const hid = this.translateKey(container, code);
        if (hid != -1)
            application.delegate("doKeyDown", {hidCode: hid});
    }
    onMediaKeyUp(container, code) {
        const hid = this.translateKey(container, code);
        if (hid != -1)
            application.delegate("doKeyUp", {hidCode: hid});
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