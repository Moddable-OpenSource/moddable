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
	onCreate(ball, delta) {
		this.dx = delta;
		this.dy = delta;
	}
	onDisplaying(ball) {
		this.x = ball.x;
		this.y = ball.y;
		this.width = ball.container.width - ball.width;
		this.height = ball.container.height - ball.height;
		ball.start();
	}
	onTimeChanged(ball) {
		var dx = this.dx;
		var dy = this.dy;
		ball.moveBy(dx, dy);
		var x = this.x + dx;
		var y = this.y + dy;
		if ((x < 0) || (x > this.width)) dx = -dx;
		if ((y < 0) || (y > this.height)) dy = -dy;
		this.dx = dx;
		this.dy = dy;
		this.x = x;
		this.y = y;
	}
};

let BallApplication = Application.template($ => ({
	skin:backgroundSkin,
	contents: [
		Content(6, { left:0, top:0, skin:ballSkin, variant:0, Behavior: BallBehavior } ),
		Content(5, { right:0, top:0, skin:ballSkin, variant:1, Behavior: BallBehavior } ),
		Content(4, { right:0, bottom:0, skin:ballSkin, variant:2, Behavior: BallBehavior } ),
		Content(3, { left:0, bottom:0, skin:ballSkin, variant:3, Behavior: BallBehavior } ),
	]
}));

export default new BallApplication(null, { touchCount:0, pixels: screen.width * 4 });
