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

import Timeline from "piu/Timeline";

const BLACK = "black"
const TRANSPARENT = "transparent"
const WHITE = "white";

const whiteSkin = new Skin({ fill:WHITE });
const blackSkin = new Skin({ fill:BLACK });
const backgroundSkin = new Skin({ fill:[WHITE, BLACK] });
const einkTexture = new Texture("eink.png");
const einkSkin = new Skin({ texture:einkTexture, color:[BLACK, WHITE], x:0, y:0, width:120, height:60 });
const moddableTexture = new Texture("moddable.png");
const moddableSkin = new Skin({ texture:moddableTexture, color:[BLACK, WHITE], x:0, y:0, width:210, height:60 });
const loveTexture = new Texture("love.png");
const loveSkin = new Skin({ texture:loveTexture, color:[BLACK, WHITE], x:0, y:0, width:60, height:60 });

let steps = [
	{ backgrounds: [ 0, 0, 0, 0, 0, 0, 0, 0 ], up:0, down:0 },
	{ backgrounds: [ 0, 0, 0, 0, 0, 0, 0, 0 ], up:0, down:0 },
	
	{ backgrounds: [ 0, 0, 0, 0, 0, 0, 0, 1 ], up:0, down:0 },
	{ backgrounds: [ 0, 0, 0, 0, 0, 0, 1, 1 ], up:0, down:0 },
	{ backgrounds: [ 0, 0, 0, 0, 0, 1, 1, 1 ], up:0, down:0 },
	{ backgrounds: [ 0, 0, 0, 0, 1, 1, 1, 1 ], up:0, down:0 },
	
	{ backgrounds: [ 0, 0, 0, 0, 1, 1, 1, 1 ], up:0, down:1 },
	{ backgrounds: [ 0, 0, 0, 0, 1, 1, 1, 1 ], up:0, down:1 },
	
	{ backgrounds: [ 0, 0, 0, 1, 1, 1, 1, 1 ], up:0, down:1 },
	{ backgrounds: [ 0, 0, 1, 1, 1, 1, 1, 1 ], up:0, down:1 },
	{ backgrounds: [ 0, 1, 1, 1, 1, 1, 1, 1 ], up:0, down:1 },
	{ backgrounds: [ 1, 1, 1, 1, 1, 1, 1, 1 ], up:0, down:1 },
	
	{ backgrounds: [ 1, 1, 1, 1, 1, 1, 1, 1 ], up:1, down:1 },
	{ backgrounds: [ 1, 1, 1, 1, 1, 1, 1, 1 ], up:1, down:1 },
	
	{ backgrounds: [ 1, 1, 1, 1, 1, 1, 1, 0 ], up:1, down:1 },
	{ backgrounds: [ 1, 1, 1, 1, 1, 1, 0, 0 ], up:1, down:1 },
	{ backgrounds: [ 1, 1, 1, 1, 1, 0, 0, 0 ], up:1, down:1 },
	{ backgrounds: [ 1, 1, 1, 1, 0, 0, 0, 0 ], up:1, down:1 },
	
	{ backgrounds: [ 1, 1, 1, 1, 0, 0, 0, 0 ], up:1, down:0 },
	{ backgrounds: [ 1, 1, 1, 1, 0, 0, 0, 0 ], up:1, down:0 },

	{ backgrounds: [ 1, 1, 1, 0, 0, 0, 0, 0 ], up:1, down:0 },
	{ backgrounds: [ 1, 1, 0, 0, 0, 0, 0, 0 ], up:1, down:0 },
	{ backgrounds: [ 1, 0, 0, 0, 0, 0, 0, 0 ], up:1, down:0 },
	{ backgrounds: [ 0, 0, 0, 0, 0, 0, 0, 0 ], up:1, down:0 },
];

let LoveApplication = Application.template($ => ({
	skin:blackSkin,
	Behavior: class extends Behavior {
		onCreate(application, anchors) {
			this.anchors = anchors;
			application.interval = 250;
			application.start();
			this.index = 0;
		}
		onDisplaying(application) {
			if ((122 === application.height) && (250 === application.width))
				;	// moddable three
			else
				trace("WARNING: This application was designed to run on a 250 x 122 screen.\n");
		}
		onTimeChanged(application) {
			let count = steps.length;
			let index = this.index + 1;
			if (index == count)
				index = 0;
			this.index = index;
			let step = steps[index];
			let anchors = this.anchors;
			let backgrounds = anchors.backgrounds;
			step.backgrounds.forEach((state, i) => {
				backgrounds[i].state = state;
			});
			anchors.moddable.state = step.up;
			anchors.love.state = step.down;
			anchors.eink.state = step.down;
		}
	},
	contents: [
		Container($, { width:250, height:122, contents:[
			Row($, { left:0, top:0, height:61, contents:[
				Content($.backgrounds, { anchor:0, width:65, height:61, skin:backgroundSkin } ),
				Content($.backgrounds, { anchor:1, width:60, height:61, skin:backgroundSkin } ),
				Content($.backgrounds, { anchor:2, width:60, height:61, skin:backgroundSkin } ),
				Content($.backgrounds, { anchor:3, width:65, height:61, skin:backgroundSkin } ),
			]}),
			Row($, { left:0, top:61, height:61, contents:[
				Content($.backgrounds, { anchor:4, width:65, height:61, skin:backgroundSkin } ),
				Content($.backgrounds, { anchor:5, width:60, height:61, skin:backgroundSkin } ),
				Content($.backgrounds, { anchor:6, width:60, height:61, skin:backgroundSkin } ),
				Content($.backgrounds, { anchor:7, width:65, height:61, skin:backgroundSkin } ),
			]}),
			Content($, { anchor:"moddable", left:5, top:1, skin:moddableSkin } ),
			Content($, { anchor:"love", left:65, top:61, skin:loveSkin } ),
			Content($, { anchor:"eink", left:126, top:61, skin:einkSkin } ),
		]}),
	]
}));

export default function () {
	new LoveApplication({ backgrounds:[], logos:[], loves:[] }, { displayListLength:1024, touchCount:0 });
}
