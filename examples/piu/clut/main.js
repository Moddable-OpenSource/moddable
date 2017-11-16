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

import { CLUT } from "piu/MC";

const blackSkin = new Skin({ fill:"black" });
const panoramaTexture = new Texture("panorama.png");
const panoramaSkin = new Skin({ texture:panoramaTexture, x:0, y:0, width:694, height:240 });

const grayCLUT = new CLUT("gray.cct");
const mainCLUT = new CLUT("main.cct");

class ScrollerBehavior extends Behavior {
	onDisplaying(scroller) {
		this.state = 0;
		this.grayColors = grayCLUT.colors;
		this.mainColors = mainCLUT.colors;
		this.blueColors = new Array(16).fill(0x192eabFF);
		this.whiteColors = new Array(16).fill(0xFFFFFFFF);
		this.colors = new Array(16).fill();
		scroller.duration = 1000;
		//scroller.interval = 50;
		scroller.time = 0;
		scroller.start();
	}
	onFinished(scroller) {
		this.state++;
		if (this.state == 8)
			this.state = 0;
		switch (this.state) {
		case 0: application.clut = mainCLUT; break;
		case 1: scroller.scrollTo(0x7FFF, 0); break;
		case 3: scroller.scrollTo(0, 0); break;
		case 4: application.clut = grayCLUT; break;
		case 5: scroller.scrollTo(0x7FFF, 0); break;
		case 7: scroller.scrollTo(0, 0); break;
		}
		scroller.time = 0;
		scroller.start();
	}
	onTimeChanged(scroller) {
		let fromColors = this.flag ? this.fromColors : this.toColors;
		let toColors = this.flag ? this.toColors : this.fromColors;
		switch (this.state) {
		case 0: fromColors = this.mainColors; toColors = this.blueColors; break;
		case 1: fromColors = this.blueColors; toColors = this.mainColors; break;
		case 2: fromColors = this.mainColors; toColors = this.whiteColors; break;
		case 3: fromColors = this.whiteColors; toColors = this.mainColors; break;
		case 4: fromColors = this.grayColors; toColors = this.blueColors; break;
		case 5: fromColors = this.blueColors; toColors = this.grayColors; break;
		case 6: fromColors = this.grayColors; toColors = this.whiteColors; break;
		case 7: fromColors = this.whiteColors; toColors = this.grayColors; break;
		}
		let colors = this.colors;
		let fraction = Math.quadEaseOut(scroller.fraction);
		for (let i = 0; i < 16; i++) {
			colors[i] = blendColors(fraction, fromColors[i], toColors[i]);
		}
		application.animateColors(colors);
	}
};

let CLUTApplication = Application.template($ => ({
	skin:blackSkin,
	contents: [
		Scroller($, {
			left:0, right:0, height:240, clip:true, Behavior:ScrollerBehavior,
			contents: [
				Content($, { left:0, top:0, skin:panoramaSkin } ),
			],
		}),
	]
}));

export default new CLUTApplication(null, { displayListLength:4096, touchCount:1 });
