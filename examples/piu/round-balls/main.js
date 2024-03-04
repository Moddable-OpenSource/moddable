/*
 * Copyright (c) 2016-2023 Moddable Tech, Inc.
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

const backgroundSkin = new Skin({ fill:"silver" });
const ballTexture = new Texture("balls.png");
const ballSkin = new Skin({ texture:ballTexture, x:0, y:0, width:30, height:30, variants:30 });

class BallBehavior extends Behavior {
	onCreate(ball, $) {
		let a = (Math.random() - 0.5) * Math.PI;
		this.dx = Math.cos(a) * $;
		this.dy = Math.sin(a) * $;
	}
	onDisplaying(ball) {
		this.r = Math.round((ball.container.width - ball.width) / 2);
		this.x = ball.x;
		this.y = ball.y;
		ball.start();
	}
	onTimeChanged(ball) {
		let dx = this.dx;
		let dy = this.dy;
		let r = this.r
		let x = this.x += dx;
		let y = this.y += dy;
		ball.x = Math.round(x);
		ball.y = Math.round(y);
		x -= r;
		y -= r;
		if ((x**2 + y**2) > r**2) {
			let a = 2 * Math.atan2(y, x);
			let cos = Math.cos(a);
			let sin = Math.sin(a);
			let x0 = x - dx;
			let y0 = y - dy;
			let x1 = cos*x0 + sin*y0;
			let y1 = sin*x0 - cos*y0;
			dx = x1 - x;
			dy = y1 - y;
		}
		this.dx = dx;
		this.dy = dy;
	}
};

const size = Math.min(screen.width, screen.height);
let BallApplication = Application.template($ => ({
	skin:backgroundSkin,
	contents: [
		Content(2, { left:size >> 1, top:50, skin:ballSkin, variant:0, Behavior: BallBehavior } ),
		Content(3, { left:50, top:size >> 1, skin:ballSkin, variant:1, Behavior: BallBehavior } ),
		Content(4, { left:size - 50, top: size >> 1, skin:ballSkin, variant:2, Behavior: BallBehavior } ),
		Content(5, { left:size >> 1, top: size - 50, skin:ballSkin, variant:3, Behavior: BallBehavior } ),
	]
}));

export default new BallApplication(null, { touchCount:0, pixels: screen.width * 4 });

trace("This project, piu/round-balls, is designed to run on a circular display. ")
