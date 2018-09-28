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
const OpenSans52 = Style.template({ color: [WHITE, MOZILLA_BLUE], font: "52px Open Sans", horizontal: "left" });

class MarqueeBehavior extends Behavior {
	onDisplaying(label) {
		let timeline = this.timeline = new Timeline();
		if (label.width > label.container.width) {
			let duration = label.width * 15;
			timeline.to(label, { x: -label.width }, duration, Math.linearEase, 0);
		} else {
			this.in = true;
			label.x = (label.container.width - label.width)/2;
			timeline.from(label, { state: 1 }, 500, Math.quadEaseOut, 0);
		}
		timeline.seekTo(0);
		label.duration = timeline.duration;
		label.time = 0;
		label.start();
	}
	fadeOut(label) {
		label.stop();
		let timeline = this.timeline = new Timeline();
		timeline.to(label, { state: 1 }, 500, Math.quadEaseOut, 0);
		timeline.seekTo(0);
		label.duration = timeline.duration;
		label.time = 0;
		this.out = true;
		label.start();
	}
	onTimeChanged(label) {
		let time = label.time;
		this.timeline.seekTo(time);
	}
	onFinished(label) {
		if (this.in) {
			this.timeline.seekTo(label.duration);	
		} else if (this.out) {
			label.container.remove(label);
			return;
		} else {
			label.time = 0;
			label.start();
		}
	}
}

const Marquee = Label.template($ => ({
	top: 0, bottom: 0, Style: OpenSans52, string: $,
	Behavior: MarqueeBehavior
}));

class SignBehavior extends Behavior {
	updateMsg(container, msg) {
		container.last.delegate("fadeOut");
		let font = new OpenSans52();
		let width = font.measure(msg).width;
		container.add(new Marquee(msg, {width, left: container.width}));
	}
}

const SignScreen = Container.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0, Skin: MozillaBlueSkin,
	contents: [
		Label($, {
			top: 20, height: 35, left: 0, right: 0, Style: OpenSans28, string: $,
		}),
		new Marquee(""),
	],
	Behavior: SignBehavior
}));

export default {
	MozillaBlueSkin,
	OpenSans28,
	SignScreen,
}
