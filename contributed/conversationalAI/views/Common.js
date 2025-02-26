/*
 * Copyright (c) 2024-2025 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import assets from "assets";
import View from "View";
import { HorizontalScrollerBehavior, VerticalScrollerBehavior } from "ScrollerBehaviors";

class CommonBehavior extends Behavior {
	onBack(container) {
		controller.goBack();
	}
	onCreate(container, view) {
		this.view = view;
	}
}

class CommonButtonBehavior extends Behavior {
	changeState(container, state) {
		container.state = state;
	}
	onCreate(container, data) {
		this.data = data;
	}
	onTap(container) {
	}
	onTouchBegan(container, id, x, y, ticks) {
		container.captureTouch(id, x, y, ticks);
		this.changeState(container, 1);
	}
	onTouchMoved(container, id, x, y, ticks) {
		this.changeState(container, container.hit(x, y) ? 1 : 0);
	}
	onTouchEnded(container) {
		if (container.state == 1) {
			this.onTap(container);
		}
	}
}

class CommonHorizontalScrollerBehavior extends HorizontalScrollerBehavior {
}

class CommonVerticalScrollerBehavior extends VerticalScrollerBehavior {
}

class VerticalScrollbarBehavior extends Behavior {
	onCreate(scrollbar) {
		this.former = 0;
	}
	onScrolled(scrollbar, scroller = scrollbar.container) {
		var thumb = scrollbar.first;
		var size = scroller.height;
		var range = scroller.first.height;
		if (size < range) {
			var height = scrollbar.height;
			thumb.y = scrollbar.y + Math.round(scroller.scroll.y * height / range);
			thumb.height = Math.round(height * size / range);
			scrollbar.visible = scrollbar.active = true;
		}
		else {
			thumb.height = 0;
			scrollbar.visible = scrollbar.active = false;
		}
	}
	onTouchBegan(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
		scrollbar.captureTouch(id, x, y, ticks);
		let thumbY = thumb.y;
		let thumbHeight = thumb.height;
		if ((y < thumbY) || ((thumbY + thumbHeight) <= y))
			this.anchor = 0 - (thumbHeight >> 1);
		else
			this.anchor = thumbY - y;
		this.min = scrollbar.y;
		this.max = scrollbar.y + scrollbar.height - thumbHeight;
		this.onTouchMoved(scrollbar, id, x, y, ticks);
	}
	onTouchEnded(scrollbar, id, x, y, ticks) {
		var thumb = scrollbar.first;
	}
	onTouchMoved(scrollbar, id, x, y, ticks) {
		var scroller = scrollbar.container;
		var thumb = scrollbar.first;
		y += this.anchor;
		if (y < this.min)
			y = this.min;
		else if (y > this.max)
			y = this.max;
		thumb.y = y;

		scroller.scrollTo(scroller.scroll.x, (y - this.min) * (scroller.first.height / scrollbar.height));
	}
};

const VerticalScrollbar = Container.template($ => ({
	width:40, right:0, top:0, bottom:0, active:true, Behavior:VerticalScrollbarBehavior,
	contents: [
		Content($, { right:0, width:20, top:0, height:0, skin:assets.skins.scrollbarY }),
	],
}));

class StarsBehavior extends Behavior {
	onDisplaying(container) {
		this.count = container.length;
		container.interval = 100;
		container.start();
	}
	onTimeChanged(container) {
		const index = Math.floor((this.count - 1) * Math.random());
		const content = container.content(index);
		content.variant = content.variant ? 0 : 1;
	}
}

const StarsContainer = Scroller.template($ => ({
	left:0, width:240, top:0, height:320, Behavior:StarsBehavior, looping:true,
	contents: [
		Content($, { left:28, top:309, skin:assets.skins.star9 }),
		Content($, { left:71, top:244, skin:assets.skins.star3 }),
		Content($, { left:104, top:90, skin:assets.skins.star6 }),
		Content($, { left:105, top:232, skin:assets.skins.star2 }),
		Content($, { left:145, top:230, skin:assets.skins.star2 }),
		Content($, { left:44, top:62, skin:assets.skins.star2 }),
		Content($, { left:34, top:34, skin:assets.skins.star3 }),
		Content($, { left:11, top:74, skin:assets.skins.star3 }),
		Content($, { left:161, top:180, skin:assets.skins.star2 }),
		Content($, { left:201, top:122, skin:assets.skins.star2 }),
		Content($, { left:181, top:90, skin:assets.skins.star6 }),
		Content($, { left:211, top:60, skin:assets.skins.star2 }),
		Content($, { left:131, top:70, skin:assets.skins.star2 }),
		Content($, { left:101, top:30, skin:assets.skins.star2 }),
		Content($, { left:54, top:50, skin:assets.skins.star2 }),
		Content($, { left:44, top:90, skin:assets.skins.star6 }),
		Content($, { left:164, top:200, skin:assets.skins.star2 }),
		Content($, { left:168, top:205, skin:assets.skins.star2 }),
		Content($, { left:32, top:132, skin:assets.skins.star3 }),
		Content($, { left:41, top:138, skin:assets.skins.star2 }),
		Content($, { left:92, top:92, skin:assets.skins.star3 }),
		Content($, { left:101, top:98, skin:assets.skins.star2 }),
		Content($, { left:71, top:168, skin:assets.skins.star2 }),
		Content($, { left:31, top:198, skin:assets.skins.star2 }),
		Content($, { left:61, top:278, skin:assets.skins.star2 }),
		Content($, { left:81, top:298, skin:assets.skins.star6 }),
		Content($, { left:151, top:285, skin:assets.skins.star2 }),
		Content($, { left:47, top:41, skin:assets.skins.star6 }),
		Content($, { left:47, top:41, skin:assets.skins.star6 }),
		Content($, { left:138, top:221, skin:assets.skins.star6 }),
		Content($, { left:138, top:221, skin:assets.skins.star6 }),
		Content($, { left:190, top:257, skin:assets.skins.star9 }),
		Content($, { left:222, top:174, skin:assets.skins.star6 }),
		Content($, { left:209, top:46, skin:assets.skins.star2 }),
		Content($, { left:22, top:117, skin:assets.skins.star2 }),
		Content($, { left:194, top:114, skin:assets.skins.star3 }),
		Content($, { left:163, top:27, skin:assets.skins.star3 }),
		Content($, { left:171, top:154, skin:assets.skins.star9 }),
		Content($, { left:74, top:124, skin:assets.skins.star3 }),
		Content($, { left:65, top:175, skin:assets.skins.star3 }),
		Content($, { left:47, top:206, skin:assets.skins.star6 }),
		Content($, { left:137, top:289, skin:assets.skins.star3 }),
	]
}));

const CommonContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:SplashBehavior,
	contents: [
	],
}));

export default class extends View {
	static get Behavior() { return CommonBehavior; }
	static get ButtonBehavior() { return CommonButtonBehavior; }
	static get HorizontalScrollerBehavior() { return CommonHorizontalScrollerBehavior; }
	static get VerticalScrollerBehavior() { return CommonVerticalScrollerBehavior; }
	static StarsContainer($, _) { return StarsContainer($, _) }
	static VerticalScrollbar($, _) { return VerticalScrollbar($, _) }
	constructor() {
		super();
	}
	get Template() { return CommonContainer }
};
