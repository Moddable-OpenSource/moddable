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

class CommonBackButtonBehavior extends CommonButtonBehavior {
	onTap(container) {
		controller.goBack()
	}
}

class CommonRowBehavior extends Behavior {
	changeState(container, state) {
		container.state = state;
	}
	tweenState(container, from, to, duration) {
		this.from = from;
		this.to = to;
		container.duration = duration;
		container.time = 0;
		container.start();
	}
	onCreate(container, data, it) {
		this.data = data;
	}
	onDisplayed(container) {
		if (container.state == 1)
			this.tweenState(container, 1, 0, 250);
	}
	onTap(container) {
		debugger
	}
	onTimeChanged(container) {
		this.changeState(container, this.from + (container.fraction * (this.to - this.from)));
	}
	onTouchBegan(container, id, x, y, ticks) {
		this.tweenState(container, 0, 1, 250);
	}
	onTouchCancelled(container) {
		container.stop();
		this.tweenState(container, container.fraction, 0, container.time);
	}
	onTouchEnded(container) {
		this.onTap(container);
	}
	onUndisplaying(container) {
// 		container.active = false;
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
			thumb.y = scrollbar.y + Math.idiv(scroller.scroll.y * height, range);
			thumb.height = Math.max(20, Math.idiv(height * size, range));
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
	width:20, right:0, top:0, bottom:0, active:true, Behavior:VerticalScrollbarBehavior,
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

const StarsContainer = Container.template($ => ({
	left:0, width:screen.width, top:0, height:screen.height, Behavior:StarsBehavior,
	contents: [
		Content($, { left:28, bottom:11, skin:assets.skins.star9 }),
		Content($, { left:71, bottom:76, skin:assets.skins.star3 }),
		Content($, { left:104, top:90, skin:assets.skins.star6 }),
		Content($, { left:105, bottom:88, skin:assets.skins.star2 }),
		Content($, { right:95, bottom:90, skin:assets.skins.star2 }),
		Content($, { left:44, top:62, skin:assets.skins.star2 }),
		Content($, { left:34, top:34, skin:assets.skins.star3 }),
		Content($, { left:11, top:74, skin:assets.skins.star3 }),
		Content($, { right:79, bottom:140, skin:assets.skins.star2 }),
		Content($, { right:39, top:122, skin:assets.skins.star2 }),
		Content($, { right:59, top:90, skin:assets.skins.star6 }),
		Content($, { right:29, top:60, skin:assets.skins.star2 }),
		Content($, { right:109, top:70, skin:assets.skins.star2 }),
		Content($, { left:101, top:30, skin:assets.skins.star2 }),
		Content($, { left:54, top:50, skin:assets.skins.star2 }),
		Content($, { left:44, top:90, skin:assets.skins.star6 }),
		Content($, { right:76, bottom:120, skin:assets.skins.star2 }),
		Content($, { right:72, bottom:115, skin:assets.skins.star2 }),
		Content($, { left:32, top:132, skin:assets.skins.star3 }),
		Content($, { left:41, top:138, skin:assets.skins.star2 }),
		Content($, { left:92, top:92, skin:assets.skins.star3 }),
		Content($, { left:101, top:98, skin:assets.skins.star2 }),
		Content($, { left:71, bottom:152, skin:assets.skins.star2 }),
		Content($, { left:31, bottom:122, skin:assets.skins.star2 }),
		Content($, { left:61, bottom:42, skin:assets.skins.star2 }),
		Content($, { left:81, bottom:22, skin:assets.skins.star6 }),
		Content($, { right:89, bottom:35, skin:assets.skins.star2 }),
		Content($, { left:47, top:41, skin:assets.skins.star6 }),
		Content($, { left:47, top:41, skin:assets.skins.star6 }),
		Content($, { right:102, bottom:99, skin:assets.skins.star6 }),
		Content($, { right:102, bottom:99, skin:assets.skins.star6 }),
		Content($, { right:50, bottom:63, skin:assets.skins.star9 }),
		Content($, { right:18, bottom:146, skin:assets.skins.star6 }),
		Content($, { right:31, top:46, skin:assets.skins.star2 }),
		Content($, { left:22, top:117, skin:assets.skins.star2 }),
		Content($, { right:46, top:114, skin:assets.skins.star3 }),
		Content($, { right:77, top:27, skin:assets.skins.star3 }),
		Content($, { right:69, top:154, skin:assets.skins.star9 }),
		Content($, { left:74, top:124, skin:assets.skins.star3 }),
		Content($, { left:65, bottom:145, skin:assets.skins.star3 }),
		Content($, { left:47, bottom:114, skin:assets.skins.star6 }),
		Content($, { right:103, bottom:31, skin:assets.skins.star3 }),
		Content($, { left:89, top:66, skin:assets.skins.star2_2 }),
		Content($, { left:106, bottom:112, skin:assets.skins.star2_3 }),
		Content($, { right:59, top:122, skin:assets.skins.star2_6 }),
		Content($, { left:36, top:101, skin:assets.skins.star2_9 }),
		Content($, { left:45, bottom:20, skin:assets.skins.star2_3 }),
		Content($, { left:84, bottom:80, skin:assets.skins.star2_3 }),
		Content($, { left:90, bottom:72, skin:assets.skins.star2_3 }),
		Content($, { right:40, bottom:10, skin:assets.skins.star2_3 }),
		Content($, { left:120, top:150, skin:assets.skins.star2_3 }),
		Content($, { left:64, top:126, skin:assets.skins.star2_3 }),
		Content($, { right:42, top:155, skin:assets.skins.star2_3 }),
		Content($, { left:75, top:144, skin:assets.skins.star2_3 }),
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
	static get BackButtonBehavior() { return CommonBackButtonBehavior; }
	static get RowBehavior() { return CommonRowBehavior; }
	static get HorizontalScrollerBehavior() { return CommonHorizontalScrollerBehavior; }
	static get VerticalScrollerBehavior() { return CommonVerticalScrollerBehavior; }
	static StarsContainer($, _) { return StarsContainer($, _) }
	static VerticalScrollbar($, _) { return VerticalScrollbar($, _) }
	constructor() {
		super();
	}
	get Template() { return CommonContainer }
};
