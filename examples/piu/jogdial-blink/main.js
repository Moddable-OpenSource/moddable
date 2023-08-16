/*
 * Copyright (c) 2016-2023 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */
import {} from "piu/MC";

const BLACK = "black";
const WHITE = "white";

const whiteSkin = new Skin({ fill: WHITE });
const bigTextStyle = new Style({ color: [BLACK, WHITE], font: "semibold 36px Open Sans"});

class BlinkAppBehavior extends Behavior {
	onCreate(column, data) {
		this.data = data;
		this.count = 0;
	}
	onDisplaying(column) {
		this.state = 0;
		this.editing = false;
		column.interval = 250;
		this.data["VALUE"].string = column.interval + "ms";
		column.start();
		let last = 1;
		this.jogDial = new Host.JogDial({
			onTurn(delta) {
				column.delegate("onTurn", delta);
			},
			onPush(value) {
				if (value !== last) {
					if (value)
						column.delegate("onClick");
					last = value;
				}
			}
		});
		this.led = new Host.LED.Default;
	}
	onTimeChanged(column) {
		this.state = !this.state;
		this.led.write(this.state ? 1 : 0);
	}
	onClick(column) {
		this.editing = !this.editing;
		this.data["VALUE"].delegate("onUpdate", this.editing);
	}
	onTurn(column, delta) {
		if (this.editing) {
			this.count += delta;
			if (this.count >= 4) {
				delta = 10;
				this.count = 0;
			} else if (this.count <= -4) {
				delta = -10;
				this.count = 0;
			}
			else 
				return;
			let interval = column.interval;
			interval += delta;
			if (interval < 10) interval = 10;
			column.interval = interval;
			this.data["VALUE"].string = Math.round(interval) + "ms";
		}
	}
}

class BlinkingTextBehavior extends Behavior {
	onCreate(text) {
		text.interval = 250;
	}
	onUpdate(text, blink) {
		if (blink)
			text.start();
		else {
			text.state = 0;
			text.stop();
		}
	}
	onTimeChanged(text) {
		text.state = !text.state;
	}
}

const BlinkApp = Application.template($ => ({ 
	top: 0, bottom: 0, left: 0, right: 0,
	skin: whiteSkin,
	contents: [
		new Column($, {
			contents: [
				new Label($, {
					anchor: "VALUE", style: bigTextStyle,
					Behavior: BlinkingTextBehavior
				})
			],
			Behavior: BlinkAppBehavior
		}),
	],
}));

export default new BlinkApp({});

trace(`
The LED blinks at the rate shown on screen (e.g. 250ms).

Press the jog dial button to adjust the rate the LED blinks.

The interval shown on screen blinks when in adjusting mode

Turn the jogdial to adjust the interval.

Press the jog dial again to exist adjusting mode.
`);
