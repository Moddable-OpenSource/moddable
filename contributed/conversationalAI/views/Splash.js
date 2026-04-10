/*
 * Copyright (c) 2024-2025 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import assets from "assets";
import Timeline from "piu/Timeline";
import View from "Common";

class SplashBehavior extends View.Behavior {
	onDisplayed(container) {
		container.duration = 1000;
		container.time = 0;
		container.start();
	}
	onFinished(container) {
		controller.goTo("Personas");
	}
}

const SplashContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:assets.skins.screen, style:assets.styles.screen, Behavior:SplashBehavior,
	contents: [
		$.constructor.StarsContainer($, {}),
		Container($, {
			left:0, width:screen.width, top:0, bottom:0, 
			contents: [
				Column($, {
					contents: [
						Content($, { skin:assets.skins.logo }),
						Content($, { height:10 }),
						Label($, { string:"Conversational AI App" }),
					]
				}),
			]
		}),
	],
}));

class SplashTimeline extends Timeline {
	constructor(screen, view, other, direction) {
		super(screen, view, other, direction);
		let body = screen.last;
		const duration = 250;
		this.from(body, { x:screen.x - body.width }, duration, Math.quadEaseOut, duration);
	}
}

export default class extends View {
	constructor() {
		super();
	}
	get Template() { return SplashContainer }
	get Timeline() { return SplashTimeline }
	get historical() { return false }
};
