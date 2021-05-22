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
const TRANSPARENT_WHITE = "#FFFFFF44";
const MOZILLA_BLUE = "#4C9ACA";

const WhiteSkin = Skin.template({ fill: WHITE });
const MozillaBlueSkin = Skin.template({ fill: MOZILLA_BLUE });

const OpenSans28 = Style.template({ color: [WHITE, MOZILLA_BLUE], font: "semibold 28px Open Sans", vertical: "middle" });

const PlugTexture = Texture.template({ path:"plug-mask.png" });
const PlugSkin = Skin.template({ 
  Texture: PlugTexture,
  color: [TRANSPARENT_WHITE, WHITE, MOZILLA_BLUE],
  x: 0, y: 0, width: 91, height: 82 
});

const Plug = Content.template($ => ({ 
	active: true, Skin: PlugSkin, state: 1, 
	Behavior: class extends Behavior {
		onCreate(container, data) {
			this.data = data;
			this.state = 1;
		}
		onTouchBegan(container) {
			application.distribute("stateChanged", container.name, !this.state);
		}
		updateIcon(container, side, state) {
			if (side != container.name) return;
			this.state = state;
			container.state = state;
		}
	},
}));

class OutletScreenBehavior extends Behavior {
	onDisplaying(container) {
		let timeline = this.timeline = new Timeline();
		timeline.from(container.first, { state: 1 }, 500, Math.quadEaseOut, 0);
		timeline.from(container.first.next, { state: 2 }, 500, Math.quadEaseOut, -500);
		timeline.from(container.last, { state: 2 }, 500, Math.quadEaseOut, -500);
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
		delete this.timeline;
	}
}

const OutletScreen = Container.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, 
	Skin: MozillaBlueSkin,
	contents: [
		Label($, { top: 30, height: 30, left: 0, right: 0, Style: OpenSans28, string: $ }),
		Row($, {
			top: 30, bottom: 0,
			contents: [
				new Plug($, { anchor: "left", name: "left", right: 15 }),
				new Plug($, { anchor: "right", name: "right",  }),
			]
		})
	],
	Behavior: OutletScreenBehavior
}));

export default {
	MozillaBlueSkin,
	OpenSans28,
	OutletScreen,
}