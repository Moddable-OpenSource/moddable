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
import {} from "piu/RoundRect";

const frameSkin = new Skin({ fill: "white" });
const fillSkin = new Skin({ fill: "#8c96d5" });

const corners = [
	RoundRect.topLeft | RoundRect.topRight | RoundRect.bottomLeft | RoundRect.bottomRight,
	RoundRect.topLeft | RoundRect.bottomRight,
	RoundRect.topLeft | RoundRect.topRight,
	RoundRect.topRight | RoundRect.bottomLeft,
	RoundRect.bottomLeft | RoundRect.bottomRight,
];

class RoundRectBehavior extends Behavior {
	onCreate(container) {
		this.index = 0;
		container.duration = 2000;
	}
	onDisplaying(container) {
		container.start();
	}
	onFinished(container) {
		this.index = (this.index + 1) % corners.length;
		container.first.corners = container.last.corners = corners[this.index];
		container.time = 0;
		container.start();
	}
	onTimeChanged(container) {
		const fraction = 1 - Math.abs((container.fraction * 2) - 1);
		const radius = Math.round(fraction * 80);
		container.first.radius = radius;
		container.last.radius = Math.max(radius - 5, 0);
	}
}

new Application(null, {
	displayListLength: 4096,
	skin: new Skin({
		fill: "#192eab"
	})
});

application.add(new Container(null, {
	left: 40, right: 40, top: 40, bottom: 40,
	Behavior: RoundRectBehavior,
	contents: [
		new RoundRect(null, { left: 0, right: 0, top: 0, bottom: 0, radius: 0, skin: frameSkin }),
		new RoundRect(null, { left: 5, right: 5, top: 5, bottom: 5, radius: 0, skin: fillSkin }),
	]
}));
