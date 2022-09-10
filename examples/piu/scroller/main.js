/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
 import {
	VerticalScrollerBehavior,
	HorizontalScrollerBehavior
} from "scroller";

const WHITE = "#ffffff";
const GRAY = "#202020";
const BLUE = "#192eab"

const backgroundSkin = new Skin({ fill: GRAY });
const headerSkin = new Skin({ fill: BLUE });
const itemsSkin = new Skin({ fill: WHITE });

const OpenSans20 = new Style({ font: "20px Open Sans" });
const headerStyle = new Style({ color: WHITE });
const itemsStyle = new Style({ color: GRAY });

const ListItem = Label.template($ => ({
	skin: itemsSkin, style: itemsStyle, string: $
}));

const VerticalScrollingContent = Scroller.template($ =>({
	anchor: "SCROLLER", left: 0, right: 0, top: 0, bottom: 0, 
	skin: backgroundSkin, active: true, clip: true,
	contents:  [
		Column($, {
			top: 0, left: 0, right: 0,
			contents: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10].map(number => new ListItem(number, { top: 20, height: 50, left: 40, right: 40 })),
		}),
    ],
	Behavior: VerticalScrollerBehavior,
}));

const HorizontalScrollingContent = Scroller.template($ => ({ 
	anchor: "SCROLLER", left: 0, right: 0, top: 0, bottom: 0, 
	skin: backgroundSkin, active: true, clip: true,
	contents:  [
		Row($, {
			top: 0, bottom: 0, left: 0,
			contents: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10].map(number => new ListItem(number, { height: 50, left: 20, width: 120 })),
		}),
    ],
	Behavior: HorizontalScrollerBehavior,
}));

class HeaderBehavior extends Behavior {
	onCreate(label) {
		this.direction = 0;
	}
	onTouchEnded(label) {
		this.direction = !this.direction;
		let newScroller;
		if (this.direction) {
			label.string = "Horizontal Scroller Example";
			newScroller = new HorizontalScrollingContent();
		} else {
			label.string = "Vertical Scroller Example";
			newScroller = new VerticalScrollingContent();
		}
		label.container.replace(label.next, newScroller);
	}
}

const ScrollerApplication = Application.template($ => ({
	style: OpenSans20,
	contents: [
		Column($, {
			top: 0, bottom: 0, left: 0, right: 0,
			contents: [
				Text($, {
					anchor: "HEADER", active: true, top: 0, left: 0, right: 0,
					skin: headerSkin, style: headerStyle, string: "Vertical Scroller Example",
					Behavior: HeaderBehavior
				}),
				new VerticalScrollingContent($),
			]
		})
	]
}));

export default new ScrollerApplication(null, { displayListLength:4096, touchCount:1 });

