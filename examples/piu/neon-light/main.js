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

import WipeTransition from "piu/WipeTransition";

const blackSkin = new Skin({ fill:[0x00000050, "transparent"] });
const shinjukuTexture = new Texture("shinjuku.png");
const shinjukuSkin = new Skin({ texture:shinjukuTexture, x:0, y:0, width:320, height:240 });
const textStyle = new Style({ font:"myFont", color:["white","transparent"], horizontal:"center", top:10, bottom:10 });
const textSkin = new Skin({ fill:0x000000A0 });

class NeonTitleBehavior extends Behavior {
	onDisplayed(container) {
		container.duration = 2000;
		container.start();
	}
	onFinished(container) {
		container.bubble("onTitleFinished");
	}
	onTimeChanged(container) {
		container.first.first.state = container.fraction;
	}
}

let NeonEnglishTitle = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:shinjukuSkin,
	Behavior: NeonTitleBehavior,
	contents:[
		Container($, {
			left:0, right:0, top:0, bottom:0, skin:textSkin,
			contents:[
				Text($, { 
					left:0, right:0, style:textStyle,
					blocks: [
						"Neon lights",
						"Shimmering neon lights",
						"And at the fall of night",
						"This city's made of lights",
					],
				}),
			],
		}),
	],
}));

let NeonJapaneseTitle = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:shinjukuSkin,
	Behavior: NeonTitleBehavior,
	contents:[
		Container($, {
			left:0, right:0, top:0, bottom:0, skin:textSkin,
			contents:[
				Text($, { 
					left:0, right:0, style:textStyle,
					blocks: [
						"ネオンライト",
						"キラキラネオンライト",
						"夜の秋に",
						"この街はライトでできています",
					],
				}),
			],
		}),
	],
}));

let NeonLights = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:shinjukuSkin,
	Behavior: class extends Behavior {
		onDisplayed(container) {
			container.duration = 3000;
			container.start();
			container.first.start();
		}
		onFinished(container) {
			container.bubble("onLightsFinished");
		}
	},
	contents:[
		Row($, {
			left:0, right:0, top:0, bottom:0, 
			Behavior: class extends Behavior {
				onCreate(row) {
					row.duration = 250;	
					this.count = row.length - 1;
					this.lights = [
						row.first,
						row.content(3),
						row.content(7),
						row.last,
					]
					this.lights[0].state = 1;
					this.lights[1].state = 1;
					this.lights[2].state = 1;
				}
				onFinished(row) {
					let lights = this.lights;
					let count = lights.length - 1;
					let index = 0;
					while (index < count) {
						 lights[index] = lights[index + 1];
						 index++;
					}
					this.current = this.up;
					let content = row.first;
					let selection = Math.round(Math.random() * (row.length - lights.length));
					while (content) {
						let index = count;
						while (index > 0) {
							if (content == lights[index - 1])
								break;
							index--;
						}
						if (index == 0) {
							if (selection == 0)
								break;
							selection--;
						}
						content = content.next;
					}
					lights[count] = content;
					row.time = 0;
					row.start();
				}
				onTimeChanged(application) {
					let fraction = application.fraction;
					let lights = this.lights;
					lights[0].state = 1 - fraction;
					lights[lights.length - 1].state = fraction;
				}
				onUndisplaying(row) {
					row.stop();
				}
			},
			contents:[
				Content($, { width:22, top:0, bottom:0, skin:blackSkin, state:0 } ),
				Content($, { width:19, top:0, bottom:0, skin:blackSkin, state:0 } ),
				Content($, { width:18, top:0, bottom:0, skin:blackSkin, state:0 } ),
				Content($, { width:49, top:0, bottom:0, skin:blackSkin, state:0 } ),
				Content($, { width:24, top:0, bottom:0, skin:blackSkin, state:0 } ),
				Content($, { width:24, top:0, bottom:0, skin:blackSkin, state:0 } ),
				Content($, { width:23, top:0, bottom:0, skin:blackSkin, state:0 } ),
				Content($, { width:18, top:0, bottom:0, skin:blackSkin, state:0 } ),
				Content($, { width:37, top:0, bottom:0, skin:blackSkin, state:0 } ),
				Content($, { width:42, top:0, bottom:0, skin:blackSkin, state:0 } ),
				Content($, { width:44, top:0, bottom:0, skin:blackSkin, state:0 } ),
			],
		})
	],
}));

let NeonApplication = Application.template($ => ({
	Behavior: class extends Behavior {
		onCreate(application) {
			this.flag = false;
			application.first.delegate("onDisplayed");
		}
		onDisplaying(application) {
			if (application.height != 240 || application.width != 320)
				trace("WARNING: This application was designed to run on a 320x240 screen.\n");
		}
		onLightsFinished(application) {
			let title = this.flag ? new NeonEnglishTitle({}) : new NeonJapaneseTitle({});
			application.run(new WipeTransition(750, Math.quadEaseOut, "center"), application.first, title);
		}
		onTitleFinished(application) {
			this.flag = !this.flag;
			application.run(new WipeTransition(750, Math.quadEaseOut, "center"), application.first, new NeonLights({}));
		}
		onTransitionBeginning(application) {
		}
		onTransitionEnded(application) {
			application.first.delegate("onDisplayed");
		}
	},
	contents: [
		NeonJapaneseTitle($, {}),
	]
}));

export default function () {
	new NeonApplication({}, { displayListLength:2048, touchCount:0 });
}
