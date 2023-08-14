/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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

import assets from "assets";
import Timeline from "piu/Timeline";
import View from "View";
import Menu from "Menu";

class GravityBehavior extends View.Behavior {
	onCreate(container, view) {
		super.onCreate(container, view);
	}
}

class BallBehavior extends View.Behavior {
	onCreate(ball, delta) {
		this.delta = delta;
	}
	onDisplayed(ball) {
		ball.interval = 20;
		ball.start();
	}
	onDisplaying(ball) {
		this.vx = 0;
		this.vy = 0;
		this.x = ball.x;
		this.y = ball.y;
		this.width = ball.container.width - ball.width;
		this.height = ball.container.height - ball.height;
	}
	onTimeChanged(ball) {
		const sample = controller.sampleAccelerometer();
		let { x, y, width, height, vx, vy, delta} = this;
		sample.x = 0 - sample.x;
		
		let ax = Math.abs(sample.x);
		if (ax > 1) {
			ax -= 1;
			vx += (Math.sign(sample.x) * ax);
		}
		else {
			vx = 0;
		}
		let dx = vx * delta;
		x += dx;
		if (x < 0) {
			x = 0;
			vx = -vx;
		}
		else if (x > width) {
			x = width;
			vx = -vx;
		}
		this.vx = vx;
		ball.x = this.x = x;

		let ay = Math.abs(sample.y);
		if (ay > 1) {
			ay -= 1;
			vy += (Math.sign(sample.y) * ay);
		}
		else {
			vy = 0;
		}
		let dy = vy * delta;
		y += dy;
		if (y < 0) {
			y = 0;
			vy = -vy;
		}
		else if (y > height) {
			y = height;
			vy = -vy;
		}
		this.vy = vy;
		ball.y = this.y = y;
	}
	onUndisplaying(ball) {
		ball.stop();
	}
}

const GravityContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:GravityBehavior,
	contents: [
		Content($, { left:0, width:128, top:0, height:128, skin:assets.skins.gray }),
		Container(0.2, { left:16, top:16, skin:assets.skins.balls, state:1, variant:4, Behavior:BallBehavior,
			contents: [ Content($, { skin:assets.skins.balls, variant:0 }) ]
		}),
		Container(0.1, { left:16, bottom:16, skin:assets.skins.balls, state:1, variant:4, Behavior:BallBehavior,
			contents: [ Content($, { skin:assets.skins.balls, variant:1 }) ]
		}),
		Container(0.1, { right:16, top:16, skin:assets.skins.balls, state:1, variant:4, Behavior:BallBehavior,
			contents: [ Content($, { skin:assets.skins.balls, variant:2 }) ]
		}),
		Container(0.2, { right:16, bottom:16, skin:assets.skins.balls, state:1, variant:4, Behavior:BallBehavior,
			contents: [ Content($, { skin:assets.skins.balls, variant:3 }) ]
		}),
	]
}));

class GravityTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		let ball = screen.first;
		let offset = 0;
		while (ball) {
			if (controller.going != direction)
				this.from(ball, { x:screen.x - ball.width }, 200, Math.quadEaseOut, offset);
			else
				this.from(ball, { x:screen.x + screen.width }, 200, Math.quadEaseOut, offset);
			ball = ball.next;
			offset = -100;
		}
	}
}

export default class extends View {
	static get Behavior() { return GravityBehavior }
	
	constructor(data) {
		super(data);
	}
	get Template() { return GravityContainer }
	get Timeline() { return GravityTimeline }
};


const GravityMenuItemContainer = Container.template($ => ({
	left:0, width:128, top:0, height:128, clip:true,
	contents: [
		Container($, {
			left:0, right:0, top:12, height:72,
			contents: [
				Content($, { skin:assets.skins[$.icon] }),
			]
		}),
		Label($, { left:0, right:0, height:24, bottom:12, style:assets.styles.wake, string:$.label }),
	]
}));

Menu.Gravity = GravityMenuItemContainer;
