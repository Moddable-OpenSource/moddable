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

class DitherBehavior extends View.Behavior {
	onCreate(container, view) {
		super.onCreate(container, view);
	}
	onDisplayed(container) {
		screen.dither = true;
		this.backwards = false;
		container.duration = 2000;
		container.start();
	}
	onFinished(container) {
		this.backwards = !this.backwards;
		container.time = 0;
		container.start();
	}
	onTimeChanged(container) {
		const content = container.last;
		const fraction = container.fraction;
		if (this.backwards)
			content.state = 2 - (2 * fraction);
		else
			content.state = 2 * fraction;
	}
	onUndisplaying(container) {
		screen.dither = false;
	}
}

class GIFImageBehavior extends Behavior {
	onDisplaying(image) {
		image.start();
	}
	onFinished(image) {
		image.time = 0;
		image.start();
	}
};

const DitherContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:DitherBehavior,
	contents: [
		GIFImage($, { left:0, top:0, width:128, height:128, path:"dither.gif", Behavior:GIFImageBehavior }),
		Content($, { left:0, width:128, top:0, height:128, skin:assets.skins.blackFade, state:0 }),
	]
}));

class DitherTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
	}
}

export default class extends View {
	static get Behavior() { return DitherBehavior }
	
	constructor(data) {
		super(data);
	}
	get Template() { return DitherContainer }
	get Timeline() { return DitherTimeline }
};
