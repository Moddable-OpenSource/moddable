/*
 * Copyright (c) 2016-2021 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import ASSETS from "assets";
import Timeline from "piu/Timeline";

class BaseScreenBehavior extends Behavior {
	onCreate(container, data) {
		this.data = data;
	}
	onTimeChanged(container) {
		this.timeline.seekTo(container.time);
	}
	onFinished(container) {
		application.delegate("switchScreen");
	}
}
Object.freeze(BaseScreenBehavior.prototype);

/* -=====================================================================- */
/* -============================ Logo screen ============================- */
/* -=====================================================================- */

class ModdableLogoScreenBehavior extends BaseScreenBehavior {
	onDisplaying(container) {
		let data = this.data;
		let timeline = this.timeline = new Timeline();
		timeline.from(data["THREE"], { visible: false }, 10, Math.quadEaseOut, 2345);
		timeline.seekTo(0);
		container.duration = timeline.duration + 3725;
		container.time = 0;
		container.start();
	}
	onFinished(container) {
		container.add(new TransitionBackground);
	}
}
Object.freeze(ModdableLogoScreenBehavior.prototype);

const ModdableLogoScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.WhiteSkin,
	contents: [
		Content($, {
			Skin: ASSETS.ModdableLogo
		}),
		Content($, {
			anchor: "THREE", top: 76, left: 147, Skin: ASSETS.Three
		})
	],
	Behavior: ModdableLogoScreenBehavior
}));

class TransitionBackgroundBehavior extends Behavior {
	onDisplaying(die) {
		this.index = 0;
		die.interval = 205;
		die.start();
	}
	onTimeChanged(die) {
		let coords;
		switch (this.index) {
			case 0:
				coords = [[3,0], [5,0], [6,0], [6,1], [7,1], [5,2], [7,2]];
				break;
			case 1:
				coords = [[1,0], [0,1], [2,1], [3,1], [1,2], [6,2]];
				break;
			case 2:
				coords = [[0,0], [7,0], [1,1], [5,1], [2,2], [3,2]];
				break;
			case 3:
				coords = [[3,0], [0,2]];
				break;
			case 4:
				die.stop();
				application.delegate("switchScreen");
				return;
		}
		for (let coord of coords)
			die.or(40*coord[0], 40*coord[1], 40, 40);
		die.cut();
		this.index++;
	}
}
Object.freeze(TransitionBackgroundBehavior.prototype);

const TransitionBackground = Die.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0,
	contents: [
		Content($, {
			top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.WhiteSkin
		}),
		Content($, {
			top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.Eye
		})
	],
	Behavior: TransitionBackgroundBehavior
}));

/* -=====================================================================- */
/* -============================ Eye screen =============================- */
/* -=====================================================================- */

class EyeScreenBehavior extends BaseScreenBehavior {
	onDisplaying(container) {
		let data = this.data;
		let timeline = this.timeline = new Timeline();
		timeline.seekTo(0);
		container.duration = timeline.duration + 1930;
		container.time = 0;
		container.start();
	}
}
Object.freeze(EyeScreenBehavior.prototype);

const EyeScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.WhiteSkin,
	contents: [
		Content($, {
			top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.Eye
		})
	],
	Behavior: EyeScreenBehavior
}));

/* -=====================================================================- */
/* -========================== Weather screen ===========================- */
/* -=====================================================================- */

class WeatherScreenBehavior extends BaseScreenBehavior {
	onDisplaying(container) {
		let data = this.data;
		let timeline = this.timeline = new Timeline();
		timeline.from(data["RIGHT"], { visible: false }, 10, null, 0);
		timeline.from(data["LEFT"], { visible: false }, 10, null, 485);
		timeline.to(data["SHOE"], { x: -93 }, 1800, null, 2550);
		timeline.seekTo(0);
		container.duration = timeline.duration + 3000;
		container.time = 0;
		container.start();
	}
}
Object.freeze(WeatherScreenBehavior.prototype);

const WeatherScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.BlackSkin,
	contents: [
		Content($, {
			top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.Eye
		}),
		Container($, {
			anchor: "RIGHT", right: 0, Skin: ASSETS.WhiteSkin,
			contents: [
				Content($, {
					top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.WeatherRight
				})
			]
		}),
		Container($, {
			anchor: "LEFT", left: 0, width: 140, top: 0, bottom: 0, Skin: ASSETS.BlackSkin,
			contents: [
				Content($, {
					top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.WeatherLeft
				})
			]
		}),
		Container($, {
			anchor: "SHOE", top: 0, bottom: 0, left: 250, width: 250+93,
			contents: [
				Content($, {
					top: 0, bottom: 0, left: 0, width: 93, Skin: ASSETS.DitherGradientLeft
				}),
				Content($, {
					top: 0, bottom: 0, left: 93, width: 250, Skin: ASSETS.WhiteSkin
				}),
				Content($, {
					top: 0, bottom: 0, left: 100, Skin: ASSETS.ShoePrice 
				})
			]
		})
	],
	Behavior: WeatherScreenBehavior
}));

/* -=====================================================================- */
/* -=========================== World screen ============================- */
/* -=====================================================================- */

class WorldScreenBehavior extends BaseScreenBehavior {
	onDisplaying(container) {
		this.index = 0;
		container.interval = 300;
		container.start();
	}
	onTimeChanged(container) {
		this.index++;
		if (this.index <= 11) {
			this.data["WORLD"].variant = this.index;
		} else {
			this.onFinished(container);
		}
	}
}
Object.freeze(WorldScreenBehavior.prototype);

const WorldScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.BlackSkin,
	contents: [
		Content($, {
			anchor: "WORLD", top: 10, left: 18, Skin: ASSETS.World
		})
	],
	Behavior: WorldScreenBehavior
}));

/* -=====================================================================- */
/* -=========================== Wi-Fi screen ============================- */
/* -=====================================================================- */

class WiFiScreenBehavior extends BaseScreenBehavior {
	onDisplaying(container) {
		let data = this.data;
		let timeline = this.timeline = new Timeline();
		timeline.from(data["RIGHT"], { visible: false }, 10, null, 1800);
		timeline.to(data["LOGO"], { x: 0 }, 1800, null, 2200);
		timeline.seekTo(0);
		container.duration = timeline.duration;
		container.time = 0;
		container.start();
	}
}
Object.freeze(WorldScreenBehavior.prototype);

const WiFiScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.BlackSkin,
	contents: [
		Container($, {
			anchor: "RIGHT", left:128, right: 0, top: 0, height: 122, Skin: ASSETS.WhiteSkin,
			contents: [
				Content($, {
					top: 7, left: 18, Skin: ASSETS.WiFiRight
				})
			]
		}),
		Container($, {
			anchor: "LEFT", left:0, Skin: ASSETS.WiFiLeftBackground,
			contents: [
				Content($, {
					left: 0, Skin: ASSETS.WiFiLeft
				})
			]
		}),
		Container($, {
			anchor: "LOGO", top: 0, height: 122, left: -343, width: 343,
			contents: [
				Content($, {
					top: 0, bottom: 0, left: 250, Skin: ASSETS.DitherGradientRight
				}),
				Content($, {
					top: 0, bottom: 0, left: 0, width: 250, Skin: ASSETS.WhiteSkin,
				}),
				Content($, {
					top: 0, bottom: 0, left: 0, Skin: ASSETS.ModdableLogo,
				})
			]
		})
	],
	Behavior: WiFiScreenBehavior
}));

export default Object.freeze([
	ModdableLogoScreen,
	EyeScreen,
	WeatherScreen,
	WorldScreen,
	WiFiScreen
])