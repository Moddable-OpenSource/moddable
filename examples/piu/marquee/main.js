/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
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
import Timeline from "piu/Timeline";

const WHITE = "white";
const BLACK = "black";

const backgroundSkin = new Skin({ fill: WHITE });
const textStyle = new Style({ font: "light 42px Open Sans", color: BLACK });

class MarqueeBehavior extends Behavior {
	onDisplaying(label) {
		let timeline = this.timeline = new Timeline();
		timeline.on(label, { x: [application.width, -label.width] }, 5000, Math.linearEase, 0);
		timeline.seekTo(0);
		label.duration = timeline.duration;
		label.time = 0;
		label.start();
	}
	onTimeChanged(label) {
		let time = label.time;
		this.timeline.seekTo(time);
	}
	onFinished(label) {
		this.timeline.seekTo(0);
		label.time = 0;
		label.start();
	}
}

let MarqueeApplication = Application.template($ => ({
	skin: backgroundSkin, style: textStyle,
	contents: [
		Label($, { 
			left: 0, top: 0, bottom: 0,
			string: "Hello from Moddable",
			Behavior: MarqueeBehavior
		})
	]
}));

export default new MarqueeApplication();
