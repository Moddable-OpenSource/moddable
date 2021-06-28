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

const WHITE = "#ffffff";
const BLACK = "#000000";
const BLUE = "#192eab";

const backgroundSkin = new Skin({ fill: BLUE });
const headerSkin = new Skin({ fill: WHITE });
const OpenSans20 = new Style({ font: "20px Open Sans", color: [BLACK, "#637784"] });

const CircleTexture = Texture.template({ path:"circle.png" });
const CircleSkin = Skin.template({ 
	Texture: CircleTexture, 
	color: ["#637784", "#7b909e", "#b3c0c9", WHITE ], 
	x: 0, y: 0, width: 32, height: 32, 
});

class TimelineBehavior extends Behavior {
	startTimeline(container) {
		let timeline = this.timeline;
		timeline.seekTo(0);
		container.duration = timeline.duration;
		container.time = 0;
		container.start();
	}
	onTimeChanged(container) {
		let time = container.time;
		this.timeline.seekTo(time);
	}
}

class ToBehavior extends TimelineBehavior {
	onDisplaying(container) {
		let timeline = this.timeline = new Timeline();
		let one=container.first, two=one.next, three=two.next, four=container.last, size=one.width;
		let r=container.x+container.width-size, b=container.y+container.height-size;
		let circleWidth = one.width, startY = one.y;
		timeline.to(one, { state: 3 }, 500, Math.quadEaseOut, 0);
		timeline.to(two, { x: r, state: 3 }, 500, Math.quadEaseOut, -400);
		timeline.to(three, { x: r, y: b, state: 3 }, 500, Math.quadEaseOut, -400);
		timeline.to(four, { y: b, state: 3 }, 500, Math.quadEaseOut, -400);
		super.startTimeline(container);
	}
}

class FromBehavior extends TimelineBehavior {
	onDisplaying(container) {
		let timeline = this.timeline = new Timeline();
		let one=container.first, two=one.next, three=two.next, four=container.last, size=one.width;
		let r=container.x+container.width-size, b=container.y+container.height-size;
		let circleWidth = one.width, startY = one.y;
		timeline.from(one, { state: 3 }, 500, Math.quadEaseOut, 0);
		timeline.from(two, { x: r, state: 3 }, 500, Math.quadEaseOut, -400);
		timeline.from(three, { x: r, y: b, state: 3 }, 500, Math.quadEaseOut, -400);
		timeline.from(four, { y: b, state: 3 }, 500, Math.quadEaseOut, -400);
		super.startTimeline(container);
	}
}

class OnBehavior extends TimelineBehavior {
	onDisplaying(container) {
		let timeline = this.timeline = new Timeline();
		let one=container.first, two=one.next, three=two.next, four=container.last, size=one.width, startX = one.x+64, startY = one.y+64;
		one.state = 0, two.state = 1, three.state = 2, four.state = 3;
		let l=container.x, r=container.x+container.width-size, t=container.y, b=container.y+container.height-size;
		timeline.on(one, { x: [startX, l, r, r, l], y: [startY, t, t, b, b] }, 2000, null, 0);
		timeline.on(two, { x: [startX, l, l, r, r], y: [startY, b, t, t, b] }, 2000, null, -2000);
		timeline.on(three, { x: [startX, r, l, l, r], y: [startY, b, b, t, t] }, 2000, null, -2000);
		timeline.on(four, { x: [startX, r, r, l, l], y: [startY, t, b, b, t] }, 2000, null, -2000);
		super.startTimeline(container);
	}
}

const TimelineContainer = Container.template($ => ({
	height: 160, width: 160, clip: true,
	contents: [
		Content($, { top: 0, left: 0, Skin: CircleSkin  }),
		Content($, { top: 0, left: 0, Skin: CircleSkin  }),
		Content($, { top: 0, left: 0, Skin: CircleSkin  }),
		Content($, { top: 0, left: 0, Skin: CircleSkin  }),
	]
}));

class OptionBehavior extends Behavior {
	onCreate(label, data) {
		this.timelineBehavior = data.timelineBehavior;
	}
	onTouchBegan(label) {
		label.state = 1;
	}
	onTouchEnded(label) {
		label.state = 0;
		label.container.next.delegate("createTimeline", this.timelineBehavior);
	}
}

const Option = Label.template($ => ({
	active: true, top: 0, bottom: 0, right: 0, left: 0, Behavior: OptionBehavior
}));

const TimelineApplication = Application.template($ => ({
	style: OpenSans20, skin: backgroundSkin,
	contents: [
		Column($, {
			top: 0, bottom: 0, left: 0, right: 0,
			contents: [
				Row($, {
					top: 0, height: 40, left: 0, right: 0, skin: headerSkin,
					contents: [
						new Option({ timelineBehavior: ToBehavior }, { string: "to" }),
						new Option({ timelineBehavior: FromBehavior }, { string: "from" }),
						new Option({ timelineBehavior: OnBehavior }, { string: "on" }),
					]
				}),
				Container($, {
					top: 0, bottom: 0, left: 0, right: 0, skin: backgroundSkin,
					Behavior: class extends Behavior {
						createTimeline(container, Behavior) {
							container.empty();
							container.add((new TimelineContainer(null, { Behavior })));
						}
					}
				}),
			]
		})
	]
}));

export default new TimelineApplication({}, { displayListLength:4096, touchCount:1 });

