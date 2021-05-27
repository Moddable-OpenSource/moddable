/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
import WipeTransition from "piu/WipeTransition";

const whiteSkin = new Skin({ fill:"white" });
const labelSkin = new Skin({ fill:"black", stroke:"white", top:1 });
const labelStyle = new Style({ font:"semibold 16px Open Sans", color:"black" });
const teamSkin = new Skin({ texture:new Texture("team.png"), x:0, y:0, width:120, height:120, variants:120 });
const teamStrings = [
	"moddable", 
	"Andy Carle", 
	"Chris Krueger", 
	"Lizzie Prader", 
	"Michael Kellner", 
	"Patrick Soquet",
	"Peter Hoddie", 
];

let BarsPort = Port.template($ => ({
	Behavior: class extends Behavior {
		drawBar(port, x, y, w, h, code) {
			let bar = BARS[code];
			for (let j = 15; j >= 0; j--) {
				if (bar & (1 << j)) 
					port.fillColor(0x000000FF, x, y, w, h);
				x += w;
			}
			return x;
		}
		onCreate(port, $) {
			this.string = teamStrings[$];
		}
		onDraw(port, x, y, w, h) {
			let string = this.string;
			let c = string.length, i, j;
			let check = 0;
			w = 10 + ((1 + c + 2) * 16) + 10;
			h = port.height;
			x = (port.width - w) >> 1;
			x = this.drawBar(port, x + 10, 0, 1, h, 104);
			for (i = 0; i < c; i++) {
				let code = string.charCodeAt(i) - 32;
				x = this.drawBar(port, x, 0, 1, h, code);
				check += i * code;
			}
			x = this.drawBar(port, x, 0, 1, h, check % 103);
			x = this.drawBar(port, x, 0, 1, h, 106);
		}
	},
}));

let BarsContainer = Container.template($ => ({
	left:0, right:0, top:0, bottom:0, skin:whiteSkin,
	Behavior: class extends Behavior {
		onDisplayed(container) {
			this.reverse = false;
			this.timeline = (new Timeline)
				.to(container.first.next, { y:180 }, 500, Math.quadEaseOut, 0)
				.to(container.first, { x:application.width }, 1000, Math.quadEaseOut, 0)
				.to(container.last, { x:(application.width - 120) >> 1 }, 1000, Math.quadEaseOut, -1000)
			container.duration = this.timeline.duration + 1000;
			container.start();	
		}
		onFinished(container) {
			//if (this.reverse)
				container.bubble("onStep");
// 			else {
// 				this.reverse = true;
// 				container.time = 0;
// 				container.start();	
// 			}
		}
		onTimeChanged(container) {
			let time = container.time;
			if (this.reverse)
				time = container.duration - time;
			this.timeline.seekTo(time);
		}
	},
	contents:[
		BarsPort($, { left:0, width:application.width, top:60, height:120, }),
		Label($, { left:0, right:0, top:240, style:labelStyle, string:teamStrings[$] }),
		Content($, { left:-120, width:120, top:60, height:120, skin:teamSkin, variant:$ }),
	],
}));

let BarsApplication = Application.template($ => ({
	Behavior: class extends Behavior {
		onCreate(application) {
			this.index = 0;
			application.add(new BarsContainer(this.index));
			application.first.delegate("onDisplayed");
		}
		onDisplaying(application) {
			if (application.height != 240 || application.width != 320)
				trace("WARNING: This application was designed to run on a 320x240 screen.\n");
		}
		onStep(application) {
			this.index++;
			if (this.index >= teamStrings.length)
				this.index = 0;
			application.run(new WipeTransition(1000, Math.quadEaseOut, "top"), application.first, new BarsContainer(this.index));
		}
		onTransitionBeginning(application) {
		}
		onTransitionEnded(application) {
			application.first.delegate("onDisplayed");
		}
	}, 
}));

export default function () {
	new BarsApplication({}, { commandListLength:4096, displayListLength:4096, touchCount:0 });
}

const BARS = [
	0b11011001100,
	0b11001101100,
	0b11001100110,
	0b10010011000,
	0b10010001100,
	0b10001001100,
	0b10011001000,
	0b10011000100,
	0b10001100100,
	0b11001001000,
	0b11001000100,
	0b11000100100,
	0b10110011100,
	0b10011011100,
	0b10011001110,
	0b10111001100,
	0b10011101100,
	0b10011100110,
	0b11001110010,
	0b11001011100,
	0b11001001110,
	0b11011100100,
	0b11001110100,
	0b11101101110,
	0b11101001100,
	0b11100101100,
	0b11100100110,
	0b11101100100,
	0b11100110100,
	0b11100110010,
	0b11011011000,
	0b11011000110,
	0b11000110110,
	0b10100011000,
	0b10001011000,
	0b10001000110,
	0b10110001000,
	0b10001101000,
	0b10001100010,
	0b11010001000,
	0b11000101000,
	0b11000100010,
	0b10110111000,
	0b10110001110,
	0b10001101110,
	0b10111011000,
	0b10111000110,
	0b10001110110,
	0b11101110110,
	0b11010001110,
	0b11000101110,
	0b11011101000,
	0b11011100010,
	0b11011101110,
	0b11101011000,
	0b11101000110,
	0b11100010110,
	0b11101101000,
	0b11101100010,
	0b11100011010,
	0b11101111010,
	0b11001000010,
	0b11110001010,
	0b10100110000,
	0b10100001100,
	0b10010110000,
	0b10010000110,
	0b10000101100,
	0b10000100110,
	0b10110010000,
	0b10110000100,
	0b10011010000,
	0b10011000010,
	0b10000110100,
	0b10000110010,
	0b11000010010,
	0b11001010000,
	0b11110111010,
	0b11000010100,
	0b10001111010,
	0b10100111100,
	0b10010111100,
	0b10010011110,
	0b10111100100,
	0b10011110100,
	0b10011110010,
	0b11110100100,
	0b11110010100,
	0b11110010010,
	0b11011011110,
	0b11011110110,
	0b11110110110,
	0b10101111000,
	0b10100011110,
	0b10001011110,
	0b10111101000,
	0b10111100010,
	0b11110101000,
	0b11110100010,
	0b10111011110,
	0b10111101110,
	0b11101011110,
	0b11110101110,
	0b11010000100,
	0b11010010000,
	0b11010011100,
	0b11000111010,
];
