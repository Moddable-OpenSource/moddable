/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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

const BLACK = "black";
const WHITE = "white";
const TRANSPARENT = "transparent";

const assets = {
	skins: {
		screen: { fill: BLACK },

		logo: { texture:{ path: "logo.png" }, width:50, height:50, color:WHITE },
		moddable: { texture:{ path: "moddable.png" }, width:128, height:50, color:WHITE },
		arrows: { texture:{ path: "arrows.png" }, width:9, height:9, variants:9, color:[WHITE,BLACK] },

		about: { texture:{ path: "about.png" }, width:72, height:72, color:WHITE },
		accelerometer: { texture:{ path: "accelerometer.png" }, width:72, height:72, color:WHITE },
		balls: { texture:{ path: "balls.png" }, x:0, y:0, width:30, height:30, variants:30, color:[BLACK,WHITE] },
		gray: { texture:{ path: "gray.png" }, x:0, y:0, width:32, height:32, left:0, right:0, top:0, bottom:0, color:WHITE },

		date: { texture:{ path: "date.png" }, width:72, height:72, color:WHITE },
		dither: { texture:{ path: "dither.png" }, width:72, height:72, color:WHITE },
		blackFade: { fill:[BLACK,TRANSPARENT,WHITE] },
		
		time: { texture:{ path: "time.png" }, width:72, height:72, color:WHITE },
		hour: { texture:{ path: "hour.png" }, width:48, height:64, variants:48, color:WHITE },
		minute: { texture:{ path: "minute.png" }, width:64, height:64, variants:64, color:WHITE },
		
		focus: { fill:[BLACK,WHITE] },
		selection: { stroke:[WHITE,BLACK], left:1, right:1, top:1, bottom:1 },
		qrcode: { fill:WHITE, stroke:BLACK },
	
		bleScan: { texture:{ path: "ble-scan.png" }, width:72, height:72, color:WHITE },
		bleWeb: { texture:{ path: "ble-web.png" }, width:72, height:72, color:WHITE },
		
		wake: { texture:{ path: "wake.png" }, width:72, height:72, color:WHITE },
	},
	styles: {
		screen: { font:"24px Arial Narrow", color:[WHITE,BLACK] },
		date: { font:"42px Arial Narrow", color:[WHITE,BLACK] },
		day: { font:"bold 24px Arial Narrow", color:[WHITE,BLACK] },
		time: { font:"60px Arial Narrow", color:[WHITE,BLACK] },
		hour: { font:"60px Arial Narrow", color:[WHITE,BLACK], horizontal:"right", right:4 },
		minute: { font:"60px Arial Narrow", color:[WHITE,BLACK], horizontal:"left", left:4 },
		url: { font:"24px Arial Narrow", color:[WHITE,BLACK], horizontal:"left", left:8, right:8 },
		
		aboutHeader: { font:"bold 20px Arial Narrow", color:[WHITE,BLACK], horizontal:"left", left:4, right:4 },
		aboutRow: { font:"20px Arial Narrow", color:[WHITE,BLACK], horizontal:"left", left:4, right:4 },
		wake: { font:"22px Arial Narrow", color:[WHITE,BLACK] },
	},
};

export default Object.freeze(assets, true);
