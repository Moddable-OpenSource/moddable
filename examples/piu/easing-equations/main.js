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
 import { HorizontalScrollerBehavior } from "scroller";

const WHITE = "#ffffff";
const BLACK = "#000000";
const BLUE = "#192eab";

const HeaderSkin = Skin.template({ fill: WHITE });
const BackgroundSkin = Skin.template({ fill: BLACK });
const BorderedSkin = Skin.template({ fill: "transparent", stroke: WHITE, borders: { left: 2, right: 2, top: 2, bottom: 2}})
const BlueSkin = Skin.template({ fill: BLUE });
const OpenSans20 = Style.template({ font: "semibold 20px Open Sans", color: [BLACK, BLUE] });

function linearEase(fraction) {
	return fraction;
}

const easingFunctions = [
	{ name: "linearEase", out: linearEase, in: linearEase },
	{ name: "backEase", out: Math.backEaseOut, in: Math.backEaseIn },
	{ name: "bounceEase", out: Math.bounceEaseOut, in: Math.bounceEaseIn },
	{ name: "circularEase", out: Math.circularEaseOut, in: Math.circularEaseIn },
	{ name: "cubicEase", out: Math.cubicEaseOut, in: Math.cubicEaseIn },
	{ name: "elasticEase", out: Math.elasticEaseOut, in: Math.elasticEaseIn },
	{ name: "exponentialEase", out: Math.exponentialEaseOut, in: Math.exponentialEaseIn },
	{ name: "quadEase", out: Math.quadEaseOut, in: Math.quadEaseIn },
	{ name: "quartEase", out: Math.quartEaseOut, in: Math.quartEaseIn },
	{ name: "quintEase", out: Math.quintEaseOut, in: Math.quintEaseIn },
	{ name: "sineEase", out: Math.sineEaseOut, in: Math.sineEaseIn },
]

class OptionBehavior extends Behavior {
	onCreate(label, data) {
		this.data = data;
	}
	onTouchBegan(label) {
		label.state = 1;
	}
	onTouchEnded(label) {
		let data = this.data;
		label.state = 0;
		application.delegate("onStartTransition", data.name, data.in, data.out);
	}
}

const Option = Label.template($ => ({
	string: $.name, active: true,
	Behavior: OptionBehavior
}));

const Header = Scroller.template($ => ({ 
	left: 0, right: 0, bottom: 0,  height: 40,
	Skin: HeaderSkin, active: true, clip: true,
	contents:  [
		Row($, {
			top: 0, bottom: 0, left: 0,
			contents: easingFunctions.map(data => new Option(data, { height: 25, left: 20, right: 20 })),
		}),
    ],
	Behavior: HorizontalScrollerBehavior
}));

class TransitionContainerBehavior extends Behavior {
	onDisplaying(container) {
		let transitioningContent = new Content(null, {
			bottom: 2, height: 50, left: container.width/2-25, width: 50,
			Skin: BlueSkin
		})
		container.add(transitioningContent);
		this.bottom = container.y+container.height-52;
		this.top = container.y+2;
	}
	onStartTransition(container, name, easeInFunction, easeOutFunction) {
		if (container.running) return;
		this.name = name;
		this.in = easeInFunction;
		this.out = easeOutFunction;
		this.easeIn(container);
	}
	easeIn(container) {
		let timeline = this.timeline = new Timeline();
		let animatedItem = container.first;
		timeline.to(animatedItem, { y: this.top }, 750, this.in, 0);
		container.duration = timeline.duration+250;
		timeline.seekTo(0);
		container.time = 0;
		container.start();
	}
	easeOut(container) {
		let timeline = this.timeline = new Timeline();
		let animatedItem = container.first;
		timeline.to(animatedItem, { y: this.bottom }, 750, this.out, 0);
		container.duration = timeline.duration;
		timeline.seekTo(0);
		container.time = 0;
		container.start();
	}
	onTimeChanged(container) {
		let time = container.time;
		this.timeline.seekTo(time);
	}
	onFinished(container) {
		if (this.in) {
			delete this.in;
			this.easeOut(container);
		} else {
			delete this.out;
			delete this.timeline;
		}
	}
}

const EasingFunctionApplication = Application.template($ => ({
	Style: OpenSans20, Skin: BackgroundSkin,
	contents: [
		Column($, {
			top: 0, bottom: 0, left: 0, right: 0,
			contents: [
				new Header($),
				Container($, {
					anchor: "TRANSITION_CONTAINER", top: 30, bottom: 30, left: 30, right: 30, 
					Skin: BorderedSkin, clip: true,
					Behavior: TransitionContainerBehavior
				}),
			]
		})
	],
	Behavior: class extends Behavior {
		onCreate(application, data) {
			this.data = data;
		}
		onStartTransition(application, name, easeInFunction, easeOutFunction) {
			this.data.TRANSITION_CONTAINER.delegate("onStartTransition", name, easeInFunction, easeOutFunction);
		}
	}
}));

export default function() {
	return new EasingFunctionApplication({}, { displayListLength:4096, touchCount:1 });
}