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

const neSkin = new Skin({ texture:new Texture("ne.png"), x:0, y:0, width:120, height:120 });
const nwSkin = new Skin({ texture:new Texture("nw.png"), x:0, y:0, width:120, height:120 });
const seSkin = new Skin({ texture:new Texture("se.png"), x:0, y:0, width:120, height:120 });
const swSkin = new Skin({ texture:new Texture("sw.png"), x:0, y:0, width:120, height:120 });

class DragBehavior extends Behavior {
	onCreate(content, target) {
		this.target = target;
	}
	onTouchBegan(content, id, x, y, ticks) {
		let container = content.container;
		container.swap(content, container.last);
		let anchor = this.anchor = content.position;
		anchor.x -= x;
		anchor.y -= y;
	}
	onTouchMoved(content, id, x, y, ticks) {
		let anchor = this.anchor;
		let target = this.target;
		x += anchor.x;
		y += anchor.y;
		if ((Math.abs(x - target.x) < 20) && (Math.abs(y - target.y) < 20))
			content.position = target;
		else
			content.position = { x, y };
	}
	onTouchEnded(content, id, x, y, ticks) {
	}
}

let DragApplication = Application.template($ => ({
	skin:new Skin({ fill:"silver" }),
	contents: [
		Container($, {
			left:0, right:0, top:0, bottom:0, clip:true,
			contents: [
				Content({ x:120, y:40 }, { right:0, bottom:40, width:120, height:120, skin:neSkin, active:true, Behavior:DragBehavior }),
				Content({ x:0, y:40 }, { right:0, top:0, width:120, height:120, skin:nwSkin, active:true, Behavior:DragBehavior }),
				Content({ x:120, y:160 }, { left:0, bottom:0, width:120, height:120, skin:seSkin, active:true, Behavior:DragBehavior }),
				Content({ x:0, y:160 }, { left:0, top:40, width:120, height:120, skin:swSkin, active:true, Behavior:DragBehavior }),
			],
		}),
	]
}));

export default function() {
	 return new DragApplication(null, { commandListLength:4096, displayListLength:4096, touchCount:1 });
}
