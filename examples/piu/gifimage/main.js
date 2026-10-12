/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
import {} from "piu/GIFImage";

class GIFImageBehavior extends Behavior {
	onDisplaying(image) {
		image.start();
	}
	onFinished(image) {
		this.count = (this.count ?? 0) + 1;
		if (0 === (this.count % 4))
			application.state = (application.state + 1) % 4;
		image.time = 0;
		image.start();
	}
}

new Application(null, {
	skin: new Skin({
		fill: ["white", "yellow", "#80FFFF", "#FFB0C0"]
	})
});

application.add(new GIFImage(null, {
	path: "anemometer.gif",
	Behavior: GIFImageBehavior
}));
