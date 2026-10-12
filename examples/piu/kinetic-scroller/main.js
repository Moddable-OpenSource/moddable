/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
import { VerticalScrollerBehavior } from "piu/KineticScroller";

const WHITE = "#ffffff";
const GRAY = "#202020";
const BLUE = "#192eab";

const backgroundSkin = new Skin({ fill: GRAY });
const headerSkin = new Skin({ fill: BLUE });
const itemSkin = new Skin({ fill: [WHITE, "#d0d8ff"] });

const applicationStyle = new Style({ font: "20px Open Sans" });
const headerStyle = new Style({ color: WHITE });
const itemStyle = new Style({ color: GRAY });

const ListItem = Label.template($ => ({
	left: 0, right: 0, height: 50,
	skin: itemSkin, style: itemStyle, state: $ & 1, string: `Item ${$}`
}));

class ListScrollerBehavior extends VerticalScrollerBehavior {
	onScrolled(scroller) {
		// show the scroll position in the header
		const header = scroller.previous;
		const y = scroller.scroll.y;
		if (header.string != y)
			header.string = y;
	}
}

const ScrollerApplication = Application.template($ => ({
	skin: backgroundSkin, style: applicationStyle,
	contents: [
		Label($, { left: 0, right: 0, top: 0, height: 40, skin: headerSkin, style: headerStyle, string: "0" }),
		Scroller($, {
			left: 0, right: 0, top: 40, bottom: 0,
			active: true, backgroundTouch: true, clip: true,
			Behavior: ListScrollerBehavior,
			contents: [
				Column($, {
					left: 0, right: 0, top: 0,
					contents: Array.from({ length: 40 }, (item, index) => new ListItem(index + 1)),
				}),
			]
		}),
	]
}));

export default new ScrollerApplication({}, { touchCount: 1 });
