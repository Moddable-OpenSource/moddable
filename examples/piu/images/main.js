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

const whiteSkin = new Skin({ fill:"white" });

const paths = [
	"fish.cs",
	"robby.cs",
	"screen1.cs",
	"screen2.cs",
	"street.cs",
];

class ImageBehavior extends Behavior {
	onDisplaying(image) {
		image.start();
	}
	onFinished(image) {
		image.bubble("onStep");
	}
};

let ImageContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:whiteSkin,
	contents: [
		Image($, { duration:1000, path:paths[$], Behavior:ImageBehavior } ),
	]
}));

let ImageApplication = Application.template($ => ({
	Behavior: class extends Behavior {
		onCreate(application) {
			this.index = 0;
			application.add(new ImageContainer(this.index));
		}
		onDisplaying(application) {
			if (application.height != 240 || application.width != 320)
				trace("WARNING: This application was designed to run on a 320x240 screen.\n");
		}
		onStep(application) {
			this.index++;
			if (this.index >= paths.length)
				this.index = 0;
			application.replace(application.first, new ImageContainer(this.index));
		}
	},
	contents: [
	]
}));

export default new ImageApplication(null, { displayListLength:4096, touchCount:0 });

