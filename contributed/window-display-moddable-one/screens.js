/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
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

import ASSETS from "assets";
import Timeline from "piu/Timeline";
import Poco from "commodetto/Poco";
import BufferOut from "commodetto/BufferOut";
import Resource from "Resource";
import parseBMP from "commodetto/parseBMP";

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
		// animate everything in
		timeline.from(data["ICON"], { x: 83 }, 350, Math.quadEaseOut, 860);
		timeline.from(data["MODDABLE"], { x: -data["MODDABLE"].width, state: 1 }, 350, Math.quadEaseOut, -350);
		timeline.to(data["ICON_CLOSE"], { state: 1 }, 50, Math.quadEaseOut, -350);
		timeline.from(data["O"], { x: application.width }, 200, Math.quadEaseOut, -100);
		timeline.from(data["N"], { x: application.width+30 }, 200, Math.quadEaseOut, -150);
		timeline.from(data["E"], { x: application.width+60 }, 200, Math.quadEaseOut, -100);
		// fade everything out
		timeline.to(data["ICON"], { state: 1 }, 375, Math.quadEaseOut, 600);
		timeline.on(data["MODDABLE"], { state: [0, 1] }, 375, Math.quadEaseOut, -375);
		timeline.to(data["O"], { state: 1 }, 375, Math.quadEaseOut, -375);
		timeline.to(data["N"], { state: 1 }, 375, Math.quadEaseOut, -375);
		timeline.to(data["E"], { state: 1 }, 375, Math.quadEaseOut, -375);
		timeline.seekTo(0);
		container.duration = timeline.duration;
		container.time = 0;
		container.start();
	}
}
Object.freeze(ModdableLogoScreenBehavior.prototype);

const ModdableLogoScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0,
	Skin: ASSETS.ModdableBlueSkin,
	contents: [
		Content($, {
			anchor: "ICON", top: 100, left: 19, Skin: ASSETS.ModdableIconSkin
		}),
		Content($, {
			anchor: "MODDABLE", top: 120, left: 49, Skin: ASSETS.ModdableLogoSkin
		}),	
		Content($, {
			anchor: "ICON_CLOSE", top: 114, left: 151, Skin: ASSETS.ModdableIconCloseSkin
		}),	
		Label($, {
			anchor: "O", top: 135, left: 130,
			string: "o", Style: ASSETS.OpenSans50
		}),		
		Label($, {
			anchor: "N", top: 135, left: 160,
			string: "n", Style: ASSETS.OpenSans50
		}),		
		Label($, {
			anchor: "E", top: 135, left: 190,
			string: "e", Style: ASSETS.OpenSans50
		})	
	],
	Behavior: ModdableLogoScreenBehavior
}));

/* -=====================================================================- */
/* -============================ Dots screen ============================- */
/* -=====================================================================- */

const redDotX = [58,66,78,88,97,106,110,111,110,106,101,95,91,88,85,83,85,86,90];
const greenDotX = [122,122,123,115,110,102,93,85,78,76,73,74,76,79,83,87,88,89,90];
const blueDotX = [90,78,70,65,63,64,68,74,79,85,91,95,97,98,96,94,92,90];
const redDotY = [110,102,98,98,101,107,115,123,130,136,140,142,142,139,137,133,131,129,130];
const greenDotY = [110,121,133,143,149,153,152,148,141,135,130,126,123,122,123,124,127,130];
const blueDotY = [164,159,153,145,135,127,120,115,112,112,114,118,122,126,128,130,131,130];
Object.freeze({redDotX, greenDotX, blueDotX, redDotY, greenDotY, blueDotY}, true);

class DotsScreenBehavior extends BaseScreenBehavior {
	onDisplaying(container) {
		let data = this.data;
		let timeline = this.timeline = new Timeline();
		// show dots
		timeline.from(data["RED"], { visible: false }, 20, Math.linearEase, 100);
		timeline.from(data["GREEN"], { visible: false }, 20, Math.linearEase, 175);
		timeline.from(data["BLUE"], { visible: false}, 20, Math.linearEase, 175);
		// spiral dots to the middle
		timeline.on(data["RED"],   { x: redDotX, y: redDotY }, 700, Math.linearEase, 275);
		timeline.on(data["GREEN"], { x: greenDotX, y: greenDotY  }, 700, Math.linearEase, -700);
		timeline.on(data["BLUE"],  { x: blueDotX, y: blueDotY }, 700, Math.linearEase, -700);
		// fade out colored dots, fade in white dot and 1
		timeline.on(data["RED"], { state: [0, 1] }, 300, Math.quadEaseOut, -200);
		timeline.on(data["GREEN"], { state: [0, 1] }, 300, Math.quadEaseOut, -300);
		timeline.on(data["BLUE"], { state: [0, 1] }, 300, Math.quadEaseOut, -300);
		timeline.to(data["WHITE"], { state: 0 }, 300, Math.quadEaseOut, -300);
		timeline.to(data["ONE1"], { state: 0 }, 300, Math.quadEaseOut, -300);
		timeline.to(data["ONE2"], { state: 0 }, 300, Math.quadEaseOut, -300);

		timeline.seekTo(0);
		container.duration = timeline.duration + 150;
		container.time = 0;
		container.start();
	}
}
Object.freeze(DotsScreenBehavior.prototype);

const DotsScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0,
	Skin: ASSETS.ModdableBlueSkin,
	contents: [
		Content($, {
			anchor: "RED", top: 104, left: 60, Skin: ASSETS.RedDot
		}),
		Content($, {
			anchor: "GREEN", top: 104, left: 120, Skin: ASSETS.GreenDot
		}),
		Content($, {
			anchor: "BLUE", top: 157, left: 90, Skin: ASSETS.BlueDot
		}),
		Content($, {
			anchor: "WHITE", top: 130, left: 90, Skin: ASSETS.WhiteDot, state: 1
		}),
		Content($, {
			anchor: "ONE1", width: 9, height: 37, left: 117, top: 141, Skin: ASSETS.ModdableBlueSkin2, state: 1
		}),
		Content($, {
			anchor: "ONE2", width: 6, height: 8, left: 111, top: 144, Skin: ASSETS.ModdableBlueSkin2, state: 1
		}),
	],
	Behavior: DotsScreenBehavior
}));

/* -=====================================================================- */
/* -======================== Zooming "one" screen =======================- */
/* -=====================================================================- */

class OneScreenBehavior extends BaseScreenBehavior {
	onDisplaying(container) {
		let data = this.data;
		let timeline = this.timeline = new Timeline();
		timeline.to(data["ONE1"], { state: 1, width: 26, height: 122, x: 107, y: 99 }, 500, Math.quadEaseOut, 100);
		timeline.to(data["ONE2"], { state: 1, width: 43, height: 23, x: 89, y: 108 }, 500, Math.quadEaseOut, -500);
		timeline.to(data["WHITE"], { state: 1 }, 400, Math.quadEaseOut, -500);
		timeline.seekTo(0);
		container.duration = timeline.duration;
		container.time = 0;
		container.start();
	}
}
Object.freeze(OneScreenBehavior.prototype);

const OneScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0,
	Skin: ASSETS.ModdableBlueSkin,
	contents: [
		Content($, {
			anchor: "WHITE", top: 130, left: 90, Skin: ASSETS.WhiteDot
		}),
		Content($, {
			anchor: "ONE1", width: 9, height: 37, left: 117, top: 141, Skin: ASSETS.ModdableBlueSkin
		}),
		Content($, {
			anchor: "ONE2", width: 15, height: 8, left: 111, top: 144, Skin: ASSETS.ModdableBlueSkin
		}),
	],
	Behavior: OneScreenBehavior
}));

/* -=====================================================================- */
/* -=========================== Graph screen ============================- */
/* -=====================================================================- */

class GraphScreenBehavior extends BaseScreenBehavior {
	onDisplaying(container) {
		let data = this.data;
		let timeline = this.timeline = new Timeline();
		// move one down and add axes
		timeline.to(data["ONE1"], { y: 134 }, 205, Math.quadEaseOut, 1000);
		timeline.to(data["ONE2"], { y: 143, width: 26, x: 107 }, 205, Math.quadEaseOut, -205);
		timeline.from(data["AXIS3"], { y: application.height }, 200, Math.quadEaseOut, -200);
		timeline.on(data["AXIS2"], { visible: [false, true] }, 5, Math.linearEase, -100);
		timeline.from(data["AXIS2"], { y: 255 }, 200, Math.quadEaseOut, -100);
		timeline.on(data["AXIS1"], { visible: [false, true] }, 5, Math.linearEase, 0);
		timeline.from(data["AXIS1"], { y: 185 }, 200, Math.quadEaseOut, -100);
		timeline.on(data["ONE2"], { visible: [true, false] }, 5, Math.linearEase, -200);
		// add lines and scroll them all to the side
		timeline.on(data["ONE1"], { x: [107, 207] }, 1485, Math.linearEase, 0);
		timeline.from(data["LINE1"], { x: 76 }, 1485, Math.linearEase, -1485);
		timeline.from(data["LINE1"], { height:0 }, 415, Math.quadEaseOut, -1485);
		timeline.from(data["LINE2"], { x: 62 }, 1175, Math.linearEase, -1175);
		timeline.from(data["LINE2"], { height:0 }, 415, Math.quadEaseOut, -1175);
		timeline.from(data["LINE3"], { x: 56 }, 690, Math.linearEase, -690);
		timeline.from(data["LINE3"], { height:0 }, 415, Math.quadEaseOut, -690);
		timeline.from(data["LINE4"], { x: 33 }, 345, Math.linearEase, -345);
		timeline.from(data["LINE4"], { height:0}, 345, Math.quadEaseOut, -345);
		timeline.from(data["LINE5"], { height:0 }, 310, Math.quadEaseOut, -310);
		timeline.from(data["LINE5"], { x: -18 }, 310, Math.linearEase, -300);
		// fade everything out
		timeline.on(data["ONE1"], { visible: [true, false] }, 20, Math.linearEase, 70);
		timeline.on(data["LINE1"], { visible: [true, false] }, 20, Math.linearEase, -20);
		timeline.on(data["AXIS3"], { state: [0, 1] }, 275, Math.linearEase, 0);
		timeline.on(data["AXIS2"], { state: [0, 1] }, 275, Math.linearEase, -275);
		timeline.on(data["AXIS1"], { state: [0, 1] }, 275, Math.linearEase, -275);
		timeline.on(data["LINE2"], { visible: [true, false] }, 20, Math.linearEase, -205);
		timeline.on(data["LINE3"], { visible: [true, false] }, 20, Math.linearEase, -135);
		timeline.on(data["LINE4"], { visible: [true, false] }, 20, Math.linearEase, -65);
		timeline.on(data["LINE5"], { visible: [true, false] }, 20, Math.linearEase, 5);
		timeline.seekTo(0);
		container.duration = timeline.duration;
		container.time = 0;
		container.start();
	}
}
Object.freeze(GraphScreenBehavior.prototype);

const GraphScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0,
	Skin: ASSETS.ModdableBlueSkin,
	contents: [
		Content($, {
			anchor: "AXIS1", width: 240, height: 1, top: 115, Skin: ASSETS.LightBlueSkin
		}),
		Content($, {
			anchor: "AXIS2", width: 240, height: 1, top: 185, Skin: ASSETS.LightBlueSkin
		}),
		Content($, {
			anchor: "AXIS3", width: 240, height: 1, top: 255, Skin: ASSETS.WhiteSkin
		}),
		Content($, {
			anchor: "ONE1", width: 26, height: 122, left: 107, top: 99, Skin: ASSETS.WhiteSkin
		}),
		Content($, {
			anchor: "ONE2", width: 43, height: 23, left: 89, top: 108, Skin: ASSETS.WhiteSkin
		}),
		Content($, {
			anchor: "LINE1", width: 26, bottom: 64, height: 96, left: 167,Skin: ASSETS.WhiteSkin
		}),		
		Content($, {
			anchor: "LINE2", width: 26, bottom: 64, height: 66, left: 127, Skin: ASSETS.WhiteSkin
		}),		
		Content($, {
			anchor: "LINE3", width: 26, bottom: 64, height: 98, left: 87, Skin: ASSETS.WhiteSkin
		}),		
		Content($, {
			anchor: "LINE4", width: 26, bottom: 64, height: 78, left: 47, Skin: ASSETS.WhiteSkin
		}),		
		Content($, {
			anchor: "LINE5", width: 26, bottom: 64, height: 56, left: 7, Skin: ASSETS.WhiteSkin
		})
	],
	Behavior: GraphScreenBehavior
}));

/* -=====================================================================- */
/* -=========================== Dates screen ============================- */
/* -=====================================================================- */

class DatesScreenBehavior extends BaseScreenBehavior {
	onDisplaying(container) {
		let data = this.data;
		let timeline = this.timeline = new Timeline();
		let circleWidth = data["Sun"].width + 3;
		timeline.from(data["Sun"], { x: -circleWidth }, 350, Math.backEaseOut, 0);
		timeline.to(data["SCROLLING_CONTENT"], { y: -280 }, 1400, Math.quadEaseInOut, -300);
		timeline.on(data["SCROLLER"], { y: [13, 240] }, 1400, Math.quadEaseInOut, -1400);
		timeline.from(data["Mon"], { x: -circleWidth }, 350, Math.backEaseOut, -1150);
		timeline.from(data["Tue"], { x: -circleWidth }, 350, Math.backEaseOut, -1000);
		timeline.from(data["Wed"], { x: -circleWidth }, 350, Math.backEaseOut, -850);
		timeline.from(data["Thu"], { x: -circleWidth }, 350, Math.backEaseOut, -700);
		timeline.from(data["Fri"], { x: -circleWidth }, 350, Math.backEaseOut, -550);
		timeline.from(data["Sat"], { x: -circleWidth }, 350, Math.backEaseOut, -400);
		timeline.to(data["SCROLLER"], { y: 13 }, 1800, Math.quadEaseInOut, 0);
		timeline.on(data["SCROLLING_CONTENT"], { y: [-280, 0] }, 1800, Math.quadEaseInOut, -1500);
		timeline.to(data["Fri"], { state: 0 }, 20, Math.linearEase, -1400);
		timeline.to(data["Wed"], { state: 0 }, 20, Math.linearEase, -1100);
		timeline.to(data["Mon"], { state: 0 }, 20, Math.linearEase, -800);
		timeline.seekTo(0);
		container.duration = timeline.duration + 400;
		container.time = 0;
		container.start();
	}
	onFinished(container) {
		container.add(new TransitionBackground);
	}
}
Object.freeze(DatesScreenBehavior.prototype);	

class TransitionBackgroundBehavior extends Behavior {
	onDisplaying(die) {
		die.duration = 450;
		die.start();
	}
	onTimeChanged(die) {
		let fraction = Math.quadEaseOut(die.fraction);
		let height = Math.round(320 * fraction);
		die.set(0, 0, 240, height)
			.cut();
	}
	onFinished(die) {
		application.delegate("switchScreen");
	}
}
Object.freeze(TransitionBackgroundBehavior.prototype);

const TransitionBackground = Die.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0,
	contents: [
		Content($, {
			top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.FeatherSkin
		})
	],
	Behavior: TransitionBackgroundBehavior
}));

const DayCircle = Label.template($ => ({
	left: 36, top: 12, height:72, width: 72, 
	Skin: ASSETS.CircleSkin, Style: ASSETS.OpenSans28, state: 1
}));

const DatesScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.ModdableBlueSkin,
	contents: [
		Column($, {
			anchor: "SCROLLING_CONTENT", top: 0, height: 640, left: 0, width: 200,
			contents: ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"].map(day => new DayCircle($, { anchor: day, string: day }))
		}),
		Content($, {
			anchor: "SCROLLER", top: 240, left: 212, Skin: ASSETS.ScrollerSkin,
		}),
	],
	Behavior: DatesScreenBehavior
}));

/* -=====================================================================- */
/* -======================== Color picker screen ========================- */
/* -=====================================================================- */

const crosshairX = [85,70,55,41,29,19,13,11,14,25,39,54,70,85,100,114,127,132,133,130,128,132,144,144,144,144,154,164,173,179,184,188,192,195,200,210,215,225,240];
const crosshairY = [-69,-61,-52,-42,-31,-17,-1,16,32,46,56,64,71,78,86,96,109,119,131,143,155,165,168,168,168,168,166,161,154,145,135,125,114,104,94,92,89,88,85];
Object.freeze({crosshairX, crosshairY}, true);

class ColorPickerScreenBehavior extends BaseScreenBehavior {
	onDisplaying(container) {
		let data = this.data;
		let timeline = this.timeline = new Timeline();
		timeline.from(data["COLOR"], { y: application.height }, 150, Math.quadEaseOut, 0);
		timeline.on(data["CROSSHAIR"], { x: crosshairX, y: crosshairY }, 1700, Math.linearEase, 100);
		timeline.to(data["COLOR"], { y: application.height }, 450, Math.quadEaseOut, -450);
		timeline.seekTo(0);
		container.duration = timeline.duration + 150;
		container.time = 0;
		container.start();
	}
	onFinished(container) {
		this.data["CROSSHAIR"].stop();
		super.onFinished(container);
	}
}
Object.freeze(ColorPickerScreenBehavior.prototype);

class CrosshairBehavior extends Behavior {
	onCreate(content, data) {
		this.data = data;
        this.backgroundGraphic = parseBMP(new Resource("feathers-color.bmp"));
        let offscreen = new BufferOut({ width: 1, height: 1, pixelFormat: screen.pixelFormat });
        this.pocoOff = new Poco(offscreen);
	}
	onDisplaying(content) {
		content.interval = 75;
		content.start();
	}
	onTimeChanged(content) {
    	let x = content.x+36;
    	let y = content.y+36;

        this.pocoOff.begin();
        this.pocoOff.drawBitmap(this.backgroundGraphic, -x, -y);
        this.pocoOff.end();

        let buff = this.pocoOff.pixelsOut.buffer;
        let view = new DataView(buff);
        let testcolor = view.getUint16(0, true);

        let red = ((testcolor >> 11) & 0x1F);
        let green = ((testcolor >> 5) & 0x3F);
        let blue = (testcolor & 0x1F);
        red = red << 3 | red >> 2;
        green = green << 2 | green >> 6;
        blue = blue << 3 | blue >> 2;

        if ((red === 0) && (green === 0) && (blue == 0)) {
        	this.data["COLOR"].last.state = 0;
        	this.data["COLOR"].last.string = `#000000`;
        	return;
        } else {
        	this.data["COLOR"].last.state = 1;
        }

    	let r = this.toHex(red);
    	let g = this.toHex(green);
    	let b = this.toHex(blue);

    	let hexColor = `#${r}${g}${b}`;
    	this.data["COLOR"].last.skin = new Skin({ fill: hexColor });
		this.data["COLOR"].last.string = hexColor;
    }
    toHex(val) {
    	return val.toString(16).padStart(2, 0).toUpperCase();
    }
}
Object.freeze(CrosshairBehavior.prototype);

const ColorPickerScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0,
	Skin: ASSETS.FeatherSkin,
	contents: [
		Content($, {
			anchor: "CROSSHAIR", top: -73, left: -73, Skin: ASSETS.CrosshairSkin,
			Behavior: CrosshairBehavior
		}),
		Container($, {
			anchor: "COLOR", left: 50, top: 250, Skin: ASSETS.RectangleSkin,
			contents: [
				Label($, {
					top: 0, bottom: 18, left: 8, right: 8,
					Style: ASSETS.OpenSans28
				})
			]
		}),
	],
	active: true, Behavior: ColorPickerScreenBehavior
}));

/* -=====================================================================- */
/* -========================== Squares screen ===========================- */
/* -=====================================================================- */

class SquaresScreenBehavior extends Behavior {
	onDisplaying(container) {
		let coords = [];
		for (let i=0; i<6; i++) {
			for (let j=0; j<8; j++) {
				coords.push([i, j]);
			}
		}
		this.coords = coords;
		container.interval = 35;
		container.start();
	}
	onTimeChanged(container) {
		let len = this.coords.length;
		if (len) {
			let index = Math.floor(Math.random() * len);
			let coord = this.coords.splice(index, 1)[0];
			container.first.or(40*coord[0], 40*coord[1], 40, 40)
				.cut();
		} else {
			container.stop();
			application.delegate("switchScreen");
		}
	}
}
Object.freeze(SquaresScreenBehavior.prototype);

const SquaresScreen = Container.template($ => ({
	top: 0, bottom: 0, left: 0, right: 0,
	Skin: ASSETS.FeatherSkin,
	contents: [
		Die($, {
			top: 0, bottom: 0, left: 0, right: 0,
			contents: [
				Content($, {
					top: 0, bottom: 0, left: 0, right: 0, Skin: ASSETS.ModdableBlueSkin
				})
			]
		})
	],
	Behavior: SquaresScreenBehavior
}));

export default Object.freeze([
	ModdableLogoScreen,
	DotsScreen,
	OneScreen,
	GraphScreen,
	DatesScreen,
	ColorPickerScreen,
	SquaresScreen
])