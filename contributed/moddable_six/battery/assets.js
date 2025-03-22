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
const GRAY = "#808080";
const LITE = "#cbcbcb";
const WHITE = "white";
const TRANSPARENT = "transparent";

const ORANGE = "#e48d71";
const GREEN = "#c2f080";
const BLUE = "#6deeff";
const YELLOW = "#ffce2a";
const DARK_GREEN = "#8fc062";

const assets = {
	BLUE,
	GREEN,
	ORANGE,
	WHITE,
	
	skins: {
		screen: { fill: BLACK },
		
		field: { fill: WHITE },
		fill: { fill:[BLACK,ORANGE,GREEN] },
		level: { fill:[ORANGE,GREEN,BLUE], stroke:WHITE },
		reset: { fill:[GREEN,BLUE] },
		separator: { fill: [WHITE,BLACK] },
		setting: { fill: BLACK, stroke:LITE, bottom:1 },
		temperatureUnitBar: { fill:[DARK_GREEN,DARK_GREEN,DARK_GREEN] },
		temperatureUnitButton: { fill:[WHITE,WHITE,WHITE], stroke:[DARK_GREEN,DARK_GREEN,DARK_GREEN], left:2, right:2, top:2, bottom:2 },
		title: { stroke:[WHITE,TRANSPARENT], bottom:1 },
		
		arrows: { texture:{ path: "arrows.png" }, width:20, y:-20, height:20, states:20, variants:20 },
		button: { texture:{ path: "button.png" }, width:50, height:50, left:10, right:10, color: [LITE,BLUE] },
		home: { texture:{ path: "home.png" }, width:86, height:57 },
		icons: { texture:{ path: "icons.png" }, width:50, height:50, variants:50, color:[WHITE,BLUE,BLACK,YELLOW] },
		input: { texture:{ path: "input.png" }, width:75, height:49 },
		output: { texture:{ path: "output.png" }, width:75, height:49 },
		scrollbarX: { texture:{ path: "scrollbarX.png" }, width:64, height:16, left:16, right:16, variants:64, color:[GRAY,LITE,LITE] },
		scrollbarY: { texture:{ path: "scrollbarY.png" }, width:20, height:60, top:10, bottom:10, color:LITE },
		sun: { texture:{ path: "sun.png" }, width:40, height:40 },
		status: { texture:{ path: "status.png" }, width:114, height:76, color:WHITE },
		type: { texture:{ path: "type.png" }, width:100, height:70, states:70 },
	},
	styles: {
		screen: { font:"18px Open Sans", color:[WHITE,BLACK] },
		
		BOLD: { font:"bold" },

		L: { font:"24px" },
		XL: { font:"26px" },
		XXXL: { font:"66px" },
		
		LEFT: { horizontal:"left" },
		RIGHT: { horizontal:"right" },

		charge: { font:"bold 10px", vertical:"top" },
		
		pad: { font:"30px", color:BLACK },
		
		reset: { font:"bold 10px", color:BLACK },
		setting: { color:[WHITE,BLUE] },
		
		time: { horizontal:"left", left:10 },		
	},
	transitions: {
		delay: 125,
		duration: 250,
	}
};

export default Object.freeze(assets, true);
