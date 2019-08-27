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
import lorem from "lorem";

const itemSkin = new Skin({ fill:[ "#192eab", "black" ] });
const itemStyle = new Style({ font:"20px Open Sans", color:"white", horizontal:"left" });
const stripSkin = new Skin({ 
	texture: { path:"wifi-strip.png" }, 
	color: "white", 
	x: 0, y: 0, width: 28, height: 28, 
	states: 27, variants: 28 
});

class ListBehavior extends Behavior {
	countItems(port) {
		return this.data.length;
	}
	drawItem(port, index, x, y, width, height) {
		let item = this.data[index];

		port.skin = null;
		port.style = itemStyle;
		port.drawLabel(item.ssid, x + height, y, width - height, height, true);

		port.skin = stripSkin;
		port.variant = Math.abs(item.variant);
		port.state = (item.variant < 0) ? 0 : 1;
		port.drawContent(x, y, height, height);
	}
	invalidateItem(port, index) {
		let delta = this.delta;
		port.invalidate(port.x, delta * index, port.width, delta);
	}
	measureItem(port) {
		return 40;
	}
	onCreate(port, data) {
		this.data = data;
		this.delta = this.measureItem(port);
		this.hit = -1;
		this.state = 0;
		port.duration = 500;
	}
	onMeasureVertically(port, height) {
		return this.countItems(port) * this.delta;
	}
	onDraw(port, x, y, width, height) {
		port.state = 0;
		port.skin = itemSkin;
		port.drawContent(x, y, width, height);

		let delta = this.delta;
		let hit = this.hit;
		let index = Math.floor(y / delta);
		let limit = y + height;
		x = 0;
		y = index * delta;
		width = port.width;
		while (y < limit) {
			if (hit == index) {
				port.state = this.state;
				port.skin = itemSkin;
				port.drawContent(x, y, width, delta);
				port.state = 0;
			}

			this.drawItem(port, index, x, y, width, delta);
			index++;
			y += delta;
		}
	}
	onFinished(port) {
		this.hit = -1;
		port.stop();
	}
	onTimeChanged(port) {
		this.state = 1 - port.fraction;
		this.invalidateItem(port, this.hit);
	}
	onTouchBegan(port, id, x, y) {
		port.stop();
		let delta = this.delta;
		let index = Math.floor((y - port.y) / delta);
		this.hit = index;
		this.state = 1;
		this.invalidateItem(port, index);
	}
	onTouchCancelled(port, id, x, y) {
		port.time = 0;
		port.start();
	}
	onTouchEnded(port, id, x, y) {
		let index = this.hit;
		this.tapItem(port, this.hit);
		port.time = 0;
		port.start();
	}
	tapItem(port, index) {
		trace("Tap " + index + "\n");
	}
};

class ScrollerBehavior extends Behavior {
	onTouchBegan(scroller, id, x, y) {
		this.anchor = scroller.scroll.y;
		this.y = y;
		this.waiting = true;
	}
	onTouchMoved(scroller, id, x, y, ticks) {
		let delta = y - this.y;
		if (this.waiting) {
			if (Math.abs(delta) < 8)
				return;
			this.waiting = false;
			scroller.captureTouch(id, x, y, ticks);
		}
		scroller.scrollTo(0, this.anchor - delta);
	}
};

let TestApplication = Application.template($ => ({
	contents: [
		Scroller($, { 
			left:0, right:0, top:0, bottom:0, active:true, backgroundTouch:true, Behavior:ScrollerBehavior,
			contents: [
				Port($, {left:0, right:0, top:0, active:true, Behavior:ListBehavior }),
			]
		}),
	]
}));

let data = lorem.split(".").map((item, index) => ({ ssid:item.trim(), variant:(index % 10) - 5 }));

export default new TestApplication(data, { displayListLength: 8192 });
