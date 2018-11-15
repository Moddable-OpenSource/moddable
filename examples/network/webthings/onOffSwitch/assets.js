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
import Timeline from "piu/Timeline";

const WHITE = "#FFFFFF";
const MOZILLA_BLUE = "#4C9ACA";

const MozillaBlueSkin = Skin.template({ fill: MOZILLA_BLUE });

const OpenSans28 = Style.template({ color: [WHITE, MOZILLA_BLUE], font: "semibold 28px Open Sans", vertical: "middle" });

const CircleTexture = Texture.template({ path:"led_on-disc-mask.png" });
const CircleSkin = Skin.template({ 
	Texture: CircleTexture,
	color: [MOZILLA_BLUE, WHITE],
	x: 0, y: 0, width: 128, height: 128 
});
const SwitchOnTexture = Texture.template({ path:"switch_on-glyph-mask.png" });
const SwitchOnSkin = Skin.template({ 
	Texture: SwitchOnTexture,
	color: MOZILLA_BLUE,
	x: 0, y: 0, width: 128, height: 128 
});
const SwitchOffTexture = Texture.template({ path:"switch_off-mask.png" });
const SwitchOffSkin = Skin.template({ 
	Texture: SwitchOffTexture,
	color: WHITE,
	x: 0, y: 0, width: 128, height: 128 
});

const Switch = Container.template($ => ({ 
	top: 30, bottom: 0, left: 0, right: 0, active: true, Skin: CircleSkin, state: 1,
	contents: [
		Content($, {
			Skin: SwitchOnSkin, 
		})
	],
	Behavior: class extends Behavior {
		onCreate(container, data) {
			this.data = data;
			this.state = 1;
		}
		onTouchBegan(container) {
			application.distribute("stateChanged", !this.state);
		}
		onSwitchUpdate(container, state) {
			this.state = state;
			container.state = state;
			if (state) {
				container.first.skin = new SwitchOnSkin;
			} else {
				container.first.skin = new SwitchOffSkin;
			}
		}
	},
}));


class SwitchScreenBehavior extends Behavior {
	onDisplaying(container) {
		let timeline = this.timeline = new Timeline();
		timeline.from(container.first, { state: 1 }, 500, Math.quadEaseOut, 0);
		timeline.from(container.last, { state: 0 }, 500, Math.quadEaseOut, -500);
		timeline.seekTo(0);
		container.duration = timeline.duration;
		container.time = 0;
		container.start();
	}
	onTimeChanged(container) {
		let time = container.time;
		this.timeline.seekTo(time);
	}
	onFinished(container) {
		this.timeline.seekTo(container.duration);
	}
}

const SwitchScreen = Container.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, Skin: MozillaBlueSkin,
	contents: [
		Label($, { top: 30, height: 35, left: 0, right: 0, Style: OpenSans28, string: $ }),
		new Switch($),
	],
	Behavior: SwitchScreenBehavior
}));

export default {
	MozillaBlueSkin,
	OpenSans28,
	SwitchScreen,
}