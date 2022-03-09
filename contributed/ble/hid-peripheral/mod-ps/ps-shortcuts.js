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

import buttonData from "assets";

const TOPMARGIN = 46;
const LEFTMARGIN = 7;
const BOTTOMMARGIN = 6
const RIGHTMARGIN = 8;
const BUTTON_BOUNCE_BACK = 400;
const BUTTON_WIDTH = 56;
const BUTTON_HEIGHT = 54;

const ButtonSkin = Skin.template({ fill: ["#505050", "#cccccc"] });
const DividerSkin = Skin.template({fill: "#cccccc"});

const TitleStyle = Style.template({ font: "24px Open Sans", color: "#cccccc" });
const MessageStyle = Style.template({ font: "20px Open Sans", color: "#cccccc" });

class ButtonBehavior extends Behavior {
    onCreate(container, data) {
        this.data = data;
    }
    onTouchBegan(container) {
        container.state = 1;
        container.first.state = 1;
        trace(`down ${this.data.x}, ${this.data.y}\n`);
    }
    onTouchEnded(container) {
        container.duration = BUTTON_BOUNCE_BACK;
        container.time = 0;
        container.start();
        container.delegate("doPress");

    }
	onTimeChanged(container) {
        container.state = 1 - Math.quadEaseOut(container.fraction);
        container.first.state = 1 - Math.quadEaseOut(container.fraction);
	}
	onFinished(container) {
		container.state = 0;
        container.first.state = 0;
        container.stop();
	}
    doPress(container) {
        const string = this.data.shortcut;

        if (string == "?") {
            trace(`shortcut not set for position ${this.data.x}, ${this.data.y}`);
            return;
        }

        for (let i = 0; i < string.length; i++) {
            const c = string.charAt(i);
            container.bubble("doKeyTap", {character: c});
        }
    }
}

const HDivider = Content.template($ => ({
    active: false, visible: true, height: 2, Skin: DividerSkin, top: $.top, left: LEFTMARGIN - 1, right: RIGHTMARGIN
}));

const VDivider = Content.template($ => ({
    active: false, visible: true, top: TOPMARGIN, bottom: BOTTOMMARGIN - 1, Skin: DividerSkin, left: $.left, width: 2
}));


const PSButton = Container.template($ => ({
	active: true, visible: true, left: $.left, top: $.top, 
    width: BUTTON_WIDTH, height: BUTTON_HEIGHT, Skin: ButtonSkin,
	contents: [
		Content($, {Skin: $.buttonSkin, state: 0})
	],
	Behavior: ButtonBehavior
}));


class ControllerBehavior extends Behavior {
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
	onCreate(container, data) {
        this.data = data;

        for (const x of buttonData) {
            const left = x.x * BUTTON_WIDTH + LEFTMARGIN;
            const top = x.y * BUTTON_HEIGHT + TOPMARGIN;
        
            const button = new PSButton({...x, left, top});
            this.data["CONTROLS"].add(button);
        }

        for (let i = TOPMARGIN - 1; i < 320; i += BUTTON_HEIGHT)
            this.data["CONTROLS"].add(new HDivider({top: i}));

        for (let i = LEFTMARGIN - 1; i < 240; i += BUTTON_WIDTH)
            this.data["CONTROLS"].add(new VDivider({left: i}));
        
	}
}

const PSController = Container.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0,
	skin: new Skin({fill: "#505050"}),
	contents: [
        Container($, {
            left: 0, right: 0, top: 0, bottom: 0, anchor: "NOTPAIRED",
            contents: [
                Text($, {
                    left: 0, right: 0, Style: MessageStyle,
                    string: "Not connected to a computer.\nConnect using your operating system's Bluetooth settings."
                })
            ]
        }),
		Container($, {
            left: 0, right: 0, top: 0, bottom: 0, anchor: "CONTROLS", visible: false,
            contents: [
                Label($, { height: TOPMARGIN, left: 0, right: 0, top: 0, string: "Photoshop Tools", Style: TitleStyle})
            ]
		})
	],
	Behavior: ControllerBehavior
}));

export default {container: PSController}
