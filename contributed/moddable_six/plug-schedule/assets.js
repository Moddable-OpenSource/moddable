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
const DARK = "#494949";
const GRAY = "#5d5d5d";
const LITE = "#a3a3a3";
const WHITE = "white";
const TRANSPARENT = "transparent";

const ORANGE = "#ff8e2f";
const LITE_ORANGE = "#ffa85f";

const GREEN = "#cfffa6";
const DARK_GREEN = "#8fd650";


const assets = {
	BLACK,
	GREEN,
	ORANGE,
	WHITE,
	
	skins: {
		screen: { fill:[WHITE,ORANGE] },
		schedule: { fill:DARK, stroke:[GREEN,DARK_GREEN] },
		
		alert: { texture:{ path: "alert.png" }, width:80, height:80, color:ORANGE },
		arc: { fill:[LITE_ORANGE,"#ffa85f00"] },
		back: { texture:{ path: "back.png" }, width:43, height:43, color:[GRAY,WHITE] },
		bar: { fill:WHITE },
		button: { fill:[LITE,LITE_ORANGE] },
		bottomShadow: { texture:{ path: "bottom-shadow.png" }, width:40, height:10, left:10, right:10 },
		current: { fill:"#ff8e2f80", stroke:LITE, bottom:1 },
		delete: { texture:{ path: "delete.png" }, width:40, height:40, color:[LITE,ORANGE] },
		dialog: { texture:{ path: "dialog.png" }, width:40, height:40, left:10, right:10, top:10, bottom:10 },
		field: { stroke:[ TRANSPARENT, BLACK ], right:1 },
		lamp: { texture:{ path: "lamp.png" }, width:120, height:95 },
		leftArc1: { texture:{ path: "left-arc-1.png" }, width:94, height:320, color:LITE_ORANGE },
		meridiemBar: { texture:{ path: "meridiem.png" }, width:80, height:27, left:10, right:10 },
		meridiemButton: { fill:WHITE },
		oval: { texture:{ path: "oval.png" }, width:186, height:186 },
		plug: { texture:{ path: "plug.png" }, width:100, height:84 },
		plugTitle: { texture:{ path: "plug-title.png" }, width:69, height:35, color:WHITE },
		power: { texture:{ path: "power.png" }, width:68, height:68, states:68 },
		scrollbarThumb: { fill:LITE },
		setting: { stroke:LITE, bottom:1 },
		settings: { texture:{ path: "settings.png" }, width:42, height:42, color:WHITE },
		smartTitle: { texture:{ path: "smart-title.png" }, width:30, height:101, color:BLACK },
		thumb: { texture:{ path: "thumb.png" }, width:18, height:18, color:[LITE,DARK_GREEN] },
		timezoneMap: { texture:{ path: "timezone-map.png" }, width:320, height:164, color:LITE },
		toggleBar: { texture:{ path: "toggle.png" }, width:40, y:-24, height:24, left:10, right:10, states:24 },
		toggleButton: { fill:WHITE },
		topArc1: { texture:{ path: "top-arc-1.png" }, width:240, height:105 },
		topArc2: { texture:{ path: "top-arc-2.png" }, width:240, height:48 },
		topArc3: { texture:{ path: "top-arc-3.png" }, width:240, height:94 },
		topShadow: { texture:{ path: "top-shadow.png" }, width:40, height:10, left:10, right:10 },
		tumbler: { texture:{ path: "tumbler.png" }, width:60, height:60, left:20, right:20, top:20, bottom:20 },
		wait: { texture:{ path: "wait.png" }, width:40, height:40, variants:40, color:ORANGE },
		wifiStrip: { texture:{ path: "wifi-strip.png" }, width:28, height:27, variants:28, states:27 },
	},
	styles: {
		screen: { font:"16px Roboto", color:[BLACK,WHITE] },
		
		L: { font:"20px" },
		BOLD: { font:"bold" },
		LEFT: { horizontal:"left" },
		RIGHT: { horizontal:"right" },
		ITALIC: { font:"italic", color:LITE },
		
		current: { font:"bold", color:[BLACK,ORANGE] },
		dialog: { font:"20px", color:WHITE },
		field: { color:BLACK, right:2 },
		setting: { color:[BLACK,ORANGE] },
		itemValue: { font:"light", horizontal:"right", color:[BLACK,ORANGE] },
		title: { font:"24px", color:WHITE },
		tumbler: { font:"medium 44px", color:GRAY },
	},
};

export default Object.freeze(assets, true);
