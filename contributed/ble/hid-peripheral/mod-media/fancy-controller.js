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

import Timer from "timer";
import {WHITE, ORANGE, OpenSans20White, BkgSkin, BTSkin, VolumeDownSkin, VolumeUpSkin, PlayPauseSkin, ForwardSkin, BackSkin, CurvedLine, Shadow, Ticks} from "assets";

const BUTTON_BOUNCE_BACK = 700;
const DEFAULT_VOL = 5;
const ADDED_VOL = DEFAULT_VOL + DEFAULT_VOL;
const UPDATE_INTERVAL = 100;

const KEY_CODES = {
    VOLUME_DOWN: 1,
    VOLUME_UP: 2,
    MUTE: 3,
    SHUFFLE: 4,
    PLAYPAUSE: 5,
    FORWARD: 6,
    BACK: 7
}

// Volume Bar

class BarBehavior extends Behavior {
	onCreate(port) {
		this.curvedLine = new CurvedLine;
		this.shadow = new Shadow;
		this.ticks = new Ticks;
		port.interval = UPDATE_INTERVAL;
	}
	onDisplaying(port) {
		this.x = 120 - DEFAULT_VOL;
		this.width = ADDED_VOL;
		this.value = 0;
	}
	onDraw(port) {
		port.drawTexture(this.ticks, WHITE, 2, 9, 0, 0, 236, this.ticks.height);
		port.drawTexture(this.curvedLine, ORANGE, this.x, 9, this.x, 0, this.width, 47);
		port.drawTexture(this.shadow, WHITE, 0, 9, 0, 0, this.shadow.width, this.shadow.height);
		port.fillColor(WHITE, 119, 0, 2, 36);
	}
	updateCoordinates(port, x, width) {
		this.x = x;
		this.width = width;
		port.invalidate();
	}
	update(port, direction, decay=false) {
		if (!decay) 
			port.stop();

		this.value += direction;

		let x, width;
		if (this.value >= 0) {
			x = 120 - DEFAULT_VOL;
			width = 10 * this.value + ADDED_VOL;
		} else {
			x = (120 - DEFAULT_VOL) + (10 * this.value);
			width = -10 * this.value + ADDED_VOL;
		}
		port.delegate("updateCoordinates", x, width);
	}
	startDecay(port) {
		port.start();
	}
	onTimeChanged(port) {
		if (this.value == 0)
			port.stop();
		else if (this.value >= 0)
			port.delegate("update", -1, true);
		else
			port.delegate("update", 1, true);
	}
}

const Bar = Port.template($ => ({
	left: 0, right: 0, height: 55,
	Behavior: BarBehavior
}));

// Buttons

class ButtonBehavior extends Behavior {
	onCreate(button, data) {
		this.data = data;
	}
	onTouchBegan(button) {
		button.first.state = 1;
		if (this.data.event)
            button.bubble("onMediaKeyDown", this.data.event);
	}
	onTouchEnded(button) {
		button.duration = BUTTON_BOUNCE_BACK;
		button.time = 0;
		button.start();
		if (this.data.event)
			button.bubble("onMediaKeyUp", this.data.event);
	}
	onTimeChanged(button) {
		button.first.state = 1 - Math.quadEaseOut(button.fraction);
	}
	onFinished(button) {
		button.first.state = 0;
	}
}

const MediaButton = Container.template($ => ({
	active: true, visible: false,
	contents: [
		Content($, {Skin: $.buttonSkin, state: 0})
	],
	Behavior: ButtonBehavior
}));

// Main UI

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
    translateKey(container, code) {
        const hidKeys = this.data.hidKeys;

        switch (code) {
            case KEY_CODES.VOLUME_DOWN:
                return hidKeys.VOLUME_DOWN.HID;
            case KEY_CODES.VOLUME_UP:
                return hidKeys.VOLUME_UP.HID;
            case KEY_CODES.MUTE:
                return hidKeys.MUTE.HID;
            case KEY_CODES.SHUFFLE:
                return hidKeys.SHUFFLE.HID;
            case KEY_CODES.PLAYPAUSE:
                return hidKeys.PLAYPAUSE.HID;
            case KEY_CODES.FORWARD:
                return hidKeys.FORWARD.HID;
            case KEY_CODES.BACK:
                return hidKeys.BACK.HID;
        }
        throw new Error("Invalid key code");
    }
	updateVolumeBar(container, code) {
		if (code == KEY_CODES.VOLUME_DOWN)
			this.data.BAR.delegate("update", -1);
		else
			this.data.BAR.delegate("update", 1);
	}
    onMediaKeyDown(container, code) {
		if (code == KEY_CODES.VOLUME_DOWN || code == KEY_CODES.VOLUME_UP) {
			container.delegate("updateVolumeBar", code);

			if (this.volumeTimer !== undefined)
				Timer.clear(this.volumeTimer);

			this.volumeTimer = Timer.repeat(() => {
				container.delegate("updateVolumeBar", code);
			}, 100);
		}

        const hid = this.translateKey(container, code);
		application.delegate("doKeyDown", {hidCode: hid});
    }
    onMediaKeyUp(container, code) {
		if (code == KEY_CODES.VOLUME_DOWN || code == KEY_CODES.VOLUME_UP) {
			if (this.volumeTimer !== undefined)
				Timer.clear(this.volumeTimer);

			this.volumeTimer = Timer.set(() => {
				container.distribute("startDecay");
				this.volumeTimer = undefined;
			}, 3_000);
		}
			
		const hid = this.translateKey(container, code);
        application.delegate("doKeyUp", {hidCode: hid});
    }
	onCreate(container, data) {
		this.volume = 1;
		this.volumeTimer = undefined;
        this.data = data;
	}
}

const BasicMediaController = Container.template($ => ({
	Skin: BkgSkin, left: 0, right: 0, top: 0, bottom: 0,
	contents: [
        Container($, {
            left: 0, right: 0, top: 0, bottom: 0, anchor: "NOTPAIRED",
            contents: [
                Text($, {
                    left: 0, right: 0, Style: OpenSans20White,
                    string: "Not connected to a computer.\nConnect using your operating system's Bluetooth settings."
                })
            ]
        }),
		Container($, {
			height: 24, width: 18, left: 30, top: 7, visible: false, Skin: BTSkin
		}),
		Label($, {
			height: 20, Style: OpenSans20White, left: 53, top: 9, string: "Media Controller", visible: false
		}),
		Bar($, { left: 0, top: 38, anchor: "BAR", visible: false}),
		MediaButton({event: KEY_CODES.VOLUME_DOWN, buttonSkin: VolumeDownSkin}, {
			height: 62, top: 225, left: 52, width: 137
		}),
		MediaButton({event: KEY_CODES.VOLUME_UP, buttonSkin: VolumeUpSkin}, {
			height: 62, top: 87, left: 52, width: 137
		}),
		MediaButton({event: KEY_CODES.PLAYPAUSE, buttonSkin: PlayPauseSkin}, {
			height: 88, top: 143, left: 76, width: 88
		}),
		MediaButton({event: KEY_CODES.FORWARD, buttonSkin: ForwardSkin}, {
			height: 138, top: 119, left: 158, width: 62
		}),
		MediaButton({event: KEY_CODES.BACK, buttonSkin: BackSkin}, {
			height: 138, top: 119, left: 20, width: 62
		}),
	],
	Behavior: ControllerBehavior
}));

export default {container: BasicMediaController}