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

import {} from "piu/MC";

import Timeline from "piu/Timeline";

const WHITE = "white";
const RED = "#ff2600"
const BLUE = "#192eab"

const whiteSkin = new Skin({ fill:WHITE });
const heartTexture = new Texture("heart.png");
const heartSkin = new Skin({ texture:heartTexture, color:[WHITE,RED], x:0, y:0, width:56, height:51 });
const logoTexture = new Texture("js-logo.png");
const logoSkin = new Skin({ texture:logoTexture, x:0, y:0, width:128, height:128 });
const stripTexture = new Texture("strip.png");
const stripSkin = new Skin({ texture:stripTexture, color:[WHITE,BLUE], x:0, y:0, width:70, height:92, variants:70 });

let StripContainer = Container.template($ => ({
	skin:whiteSkin,
	Behavior: class extends Behavior {
		onDisplaying(container) {
			container.duration = 1000;
			container.start();
		}
		onFinished(container) {
			container.time = 0;
			container.start();
		}
		onTimeChanged(container) {
			container.first.variant = (17 * container.fraction);
		}
	},
	contents: [
		Content($, { skin:stripSkin, variant:0 } ),
	]
}));

let LoveApplication = Application.template($ => ({
	skin:whiteSkin,
	Behavior: class extends Behavior {
		onCreate(application, anchors) {
			this.anchors = anchors;
			this.reverse = false;
			this.timeline = (new Timeline)
				.to(anchors.strip, { variant:17 }, 1000, null, 0)
				.to(anchors.strip, { variant:17 }, 1000, null, 0)
				.to(anchors.strip, { variant:17, state:0, y:-92 }, 1000, null, 0)
				.to(anchors.heart, { state:1, y:36 }, 500, Math.quadEaseOut, -250)
				.to(anchors.logo, { y:0 }, 500, Math.quadEaseOut, 500);
			application.duration = this.timeline.duration + 500;
			application.start();
		}
		onDisplaying(application) {
			if (application.width != 128 || application.height != 128)
				trace("WARNING: This application was designed to run on a 128x128 screen.\n");
		}
		onFinished(application) {
			this.reverse = !this.reverse;
			application.time = 0;
			application.start();
		}
		onTimeChanged(application) {
			let time = application.time;
			if (this.reverse)
				time = application.duration - time;
			this.timeline.seekTo(time);
		}
	},
	contents: [
		Content($, { anchor:"heart", skin:heartSkin, top:128, state:0 } ),
		Content($, { anchor:"strip", skin:stripSkin, top:18, variant:0, state:1 } ),
		Content($, { anchor:"logo", skin:logoSkin, top:128, } ),
	]
}));

export default function () {
	new LoveApplication({}, { displayListLength:1024, touchCount:0 });
}
