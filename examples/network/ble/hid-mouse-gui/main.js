/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

import config from "mc/config";
import Mouse from "mouse";

const desktopTexture = new Texture("desktop.png");
const desktopSkin = new Skin({ texture:desktopTexture, x:0, y:0, width:40, height:40, tiles:{ left:0, right:0, top:0, bottom:0 } });

const arrowTexture = new Texture("arrow3.png");
const arrowSkin = new Skin({ texture:arrowTexture, x:0, y:0, width:12, height:18 });

const bordersSkin = new Skin({ stroke:"black", left:2, right:2, top:2, bottom:2 });

const buttonTexture = new Texture("button.png");
const buttonSkin = new Skin({ texture:buttonTexture, x:0, y:0, width:60, height:40, states:40, tiles:{ left:20, right:20 } });
const buttonStyle = new Style({ font:"600 28px Open Sans", color:["black", "white"], });

class DragBehavior extends Behavior {
	onFinished(content) {
		content.state = 0;
	}
	onTimeChanged(content) {
		content.state = 1 - Math.quadEaseOut(content.fraction);
	}
	onTouchBegan(content, id, x, y, ticks) {
		content.time = 0;
		content.stop();

		let anchor = this.anchor = content.position;
		anchor.x -= x;
		anchor.y -= y;
		content.state = 1;
	}
	onTouchMoved(content, id, x, y, ticks) {
		let anchor = this.anchor;
		content.position = { x:anchor.x + x, y:anchor.y + y };
	}
	onTouchEnded(content, id, x, y, ticks) {
		content.duration = 250;
		content.start();
	}
}

let DragApplication = Application.template($ => ({
	skin:desktopSkin, style:buttonStyle,
	Behavior: class extends Behavior {
		onCreate(applicaton, data) {
			this.data = data;
			this.mouse = new Mouse(this.data);
		}
		onMouseReady(application) {
			this.draggers = application.first.first;
			this.pointer = application.last;
			this.pointer.position = {
				x: application.width/2 - arrowSkin.width/2,
				y: application.height/2 - arrowSkin.height/2
			}
			this.pointer.visible = true;
		}
		onMouseDown(application, x, y, buttons) {
			this.doMoveTo(this.pointer, x, y);
			let position = this.pointer.position;
			let content = this.hitContent(this.draggers, position.x, position.y);
			if (content) {
				this.target = content;
				content.delegate("onTouchBegan", 0, position.x, position.y);
			}
		}
		onMouseUp(application, x, y, buttons) {
			this.doMoveTo(this.pointer, x, y);
			let position = this.pointer.position;
			if (this.target) {
				this.target.delegate("onTouchEnded", 0, position.x, position.y);
			}
			delete this.target;
		}
		onMouseMoved(application, x, y, buttons) {
			this.doMoveTo(this.pointer, x, y);
			let position = this.pointer.position;
			if (this.target) {
				let content = this.target;
				content.delegate("onTouchMoved", 0, position.x, position.y);
			}
		}
		hitContent(container, x, y) {
			let count = container.length;
			for (let i = count - 1; i >= 0; i--) {
				let content = container.content(i);
				if (content.hit(x, y))
					return content;
			}
		}
		doMoveTo(content, x, y) {
			let _x = content.x + x;
			let _y = content.y + y;
			if (_x > application.width)
				_x = application.width - 1;
			else if (_x < 0)
				_x = 0;
			if (_y > application.height)
				_y = application.height - 1;
			else if (_y < 0)
				_y = 0;
			content.position = {x:_x, y:_y};			
		}
	},
	contents: [
		Container($, {
			left:18, right:18, top:18, bottom:18, skin:bordersSkin,
			contents: [
				Container($, {
					left:2, right:2, top:2, bottom:2, clip:true,
					contents: [
						Label($, { left:10, top:10, width:120, height:40, skin:buttonSkin, string:"Drag 1", active:true, Behavior:DragBehavior }),
						Label($, { right:10, top:10, width:120, height:40, skin:buttonSkin, string:"Drag 2 ", active:true, Behavior:DragBehavior }),
						Label($, { right:10, bottom:10, width:120, height:40, skin:buttonSkin, string:"Drag 3", active:true, Behavior:DragBehavior }),
						Label($, { left:10, bottom:10, width:120, height:40, skin:buttonSkin, string:"Drag 4", active:true, Behavior:DragBehavior }),
					],
				}),
			],
		}),
		Content($, { left:0, top:0, skin:arrowSkin, visible:false }),
	]
}));

export default new DragApplication({ bonding:false }, { commandListLength:4096, displayListLength:4096 });
