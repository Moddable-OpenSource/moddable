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
import GIFImage from "piu/GIFImage";
import Timeline from "piu/Timeline";
import View from "View";

class SplashBehavior extends View.Behavior {
	onDisplaying(container) {
		container.duration = 1;
		container.time = 0;
		container.start();
	}
	onFinished(container) {
		container.first.start();
	}
	onJogDialReleased(container) {
		container.first.stop();
		controller.goTo("Home");
		return true;
	}
}

class GIFImageBehavior extends Behavior {
	onFinished(image) {
		controller.goTo("Home");
	}
};

const SplashContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:SplashBehavior,
	contents: [
		GIFImage($, { left:0, top:0, width:128, height:128, path:"splash.gif", Behavior:GIFImageBehavior }),
	]
}));

class SplashTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super();
		let image = screen.first;
		this.from(image, { x:screen.x - image.width }, 250, Math.quadEaseOut, 0);
	}
}

export default class extends View {
	static get Behavior() { return SplashBehavior }
	
	constructor(data) {
		super(data);
	}
	get Template() { return SplashContainer }
	get Timeline() { return SplashTimeline }
	get historical() { return false }
};
