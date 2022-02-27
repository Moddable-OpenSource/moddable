/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

const isMac = system.platform == "mac";

const liteColors = {
	"button": { "fill": [ "transparent", "transparent", "#ffffff", "#192eab" ], "color": [ "#cdcdcd", "#404040", "#262626", "#ffffff" ] },
	"divider": { "fill": "#c2c2c2" },
	"fieldScroller": { "fill": "#ffffff", "stroke": "#f0f0f0" },
	"scrollbar": { "fill": [ "transparent", "#fafafa", "#fafafa", "#fafafa" ], "stroke": [ "transparent", "#dbdbdb", "#dbdbdb", "#dbdbdb" ] },
	"scrollbarThumb": { "fill": [ "#e0e0e000", "#e0e0e0", "#e6e6e6", "#e6e6e6" ] },
	"switchBar": { "fill": [ "transparent", "#b0b0b0", "#192eab", "#192eab" ], "stroke": [ "#e6e6e6", "#b0b0b0", "#192eab", "#192eab" ] },
	"switchButton": { "fill": [ "transparent", "#ffffff", "#ffffff", "#ffffff" ], "stroke": [ "#e6e6e6", "#ffffff", "#ffffff", "#ffffff" ] },
	
	"tab": { "fill": [ "#f0f0f0", "#dbdbdb", "#e6e6e6", "#f0f0f0" ], "stroke": "#c2c2c2", "color": [ "#000000", "#404040", "#000000", "#000000" ] },
	"tabBreakpoint": { "fill": "#7fbd3b", "color": "#ffffff" },
	"tabBubble": { "fill": "#6573c7", "color": "#ffffff" },
	
	"pathName": { "color": "#000000" },
	"pathSpan": { "color": [ "#ababab", "#ababab", "#787878", "#454545" ] },
	
	"lineNumber": { "color": "#ababab", "fill": "#fafafa", "stroke": "#dbdbdb" },
	"lineBreakpoint": { "color": "#ffffff", "fill": "#7fbd3b" },

	"code": { "fill": "#ffffff", "color": "#000000" },
	"codeComment": { "color": "#008d32" },
	"codeKeyword": { "color": "#103ffb" },
	"codeLiteral": { "color": "#b22821" },
	"codeResult": { "fill": "#fbf4bb" },
	"codeSelection": { "fill": "#d1e0ff" },
	"error": { "color": "#262626" },
	
	"findLabel": { "color": "#000000" },
	"findMode": { "fill": [ "transparent", "transparent", "#e0e0e0", "#a0a0a0" ], "color": [ "transparent", "#ffffff", "#e0e0e0", "#555555" ] },
	"resultCount": { "color": "#808080" },
	"resultRow": { "fill": [ "#ffffff", "#ffffff", "#fafafa", "#f0f0f0" ] },
	"resultLabel": { "fill": [ "#000000", "#000000", "#fbf4bb", "#fbf4bb" ], "color": "#000000" },
	"searchEmpty": { "color": "#808080" },
	
	"tableRow": { "color": "#000000", "fill": [ "#fafafa", "#fafafa", "#f0f0f0", "#e6e6e6" ] },
	"tableHeader": { "color": "#404040", "fill": [ "#f0f0f0", "#f0f0f0", "#e6e6e6", "#dbdbdb" ], "stroke": "#dbdbdb", },
	"fileRow": { "fill": "#6497ff", "color": "#ffffff" },
	"infoRow": { "color": "#808080" },
	
	"messageBackground": { "fill": "#f0f0f0" },
	"messages": [
		{ "fill": "#ffffff" },
		{ "fill": "#ffefef" },
		{ "fill": "#ffffef" },
		{ "fill": "#efffef" },
		{ "fill": "#efffff" },
		{ "fill": "#efefff" },
		{ "fill": "#ffefff" }
	],
	
	"progressBar": { "fill": [ "#f0f0f0", "#404040", "#000000", "#000000" ] },
	
	"callRow": { "fill": "#fe9d27", "color": "#ffffff" },
	"inspectFlash": { "color":  "#082080" },
	"inspectCache": { "color": "#804f13" },
	
	"test262Fail": { "color": "#b22821" },
	"test262Pass": { "color": "#008d32" },
	"test262Skip": { "color": "#103ffb" }
};

const darkColors = {
	"button": { "fill": [ "transparent", "transparent", "#000000", "#f68100" ], "color": [ "#808080", "#ffffff", "#ffffff", "#ffffff" ] },
	"divider": { "fill": "#545454" },
	"fieldScroller": { "fill": "#1e1e1e", "stroke": "#545454" },
	"scrollbar": { "fill": [ "transparent", "#3b3b3b", "#3b3b3b", "#3b3b3b" ], "stroke": [ "transparent", "#454545", "#454545", "#454545" ] },
	"scrollbarThumb": { "fill": [ "#a2a2a200", "#a2a2a2", "#a2a2a2", "#a2a2a2" ] },
	"switchBar": { "fill": [ "#000000", "#5d5d5d", "#f68100", "#f68100" ], "stroke": [ "#5d5d5d", "#5d5d5d", "#f68100", "#f68100" ] },
	"switchButton": { "fill": [ "#000000", "#e4e4e4", "#e4e4e4", "#e4e4e4" ], "stroke": [ "#5d5d5d", "#e4e4e4", "#e4e4e4", "#e4e4e4" ] },
	
	"tab": { "fill": [ "#3b3b3b", "#212121", "#343434", "#3b3b3b" ], "stroke": "#4a4a4a", "color": [ "#ffffff", "#c0c0c0", "#e0e0e0", "#ffffff" ] },
	"tabBreakpoint": { "color": "#ffffff" },
	"tabBubble": { "fill": "#6573c7", "color": "#ffffff" },
	
	"pathName": { "color": "#f0f0f0" },
	"pathSpan": { "color": [ "#a0a0a0", "#a0a0a0", "#c0c0c0", "#f0f0f0" ] },
	
	"lineNumber": { "color": "#ababab", "fill": "#333333", "stroke": "#545454" },
	"lineBreakpoint": { "color": "#ffffff", "fill": "#7fbd3b" },

	"code": { "fill": "#2a2a2a", "color": "#f0f0f0" },
	"codeComment": { "color": "#3cb878" },
	"codeKeyword": { "color": "#77bafc" },
	"codeLiteral": { "color": "#f26c4f" },
	"codeResult": { "fill": "#7d4900" },
	"codeSelection": { "fill": "#003663" },
	"error": { "color": "#262626" },
	
	"findLabel": { "color": "#000000" },
	"findMode": { "fill": [ "transparent", "transparent", "#4b4b4b", "#5b5b5b" ], "color": [ "transparent", "#e0e0e0", "#ffffff", "#e0e0e0" ] },
	"resultCount": { "color": "#a0a0a0" },
	"resultRow": { "fill": [ "#2a2a2a", "#2a2a2a", "#3a3a3a", "#4a4a4a" ] },
	"resultLabel": { "fill": [ "#000000", "#000000", "#7d4900", "#7d4900" ], "color": "#f0f0f0" },
	"searchEmpty": { "color": "#a0a0a0" },
	
	"tableRow": { "color": "#ffffff", "fill": [ "#272727", "#272727", "#373737", "#474747" ], },
	"tableHeader": { "color": "#f0f0f0", "fill": [ "#3b3b3b", "#3b3b3b", "#4b4b4b", "#5b5b5b" ], "stroke": "#4e4e4e", },
	"fileRow": { "fill": "#6497ff", "color": "#ffffff" },
	"infoRow": { "color": "#a0a0a0" },

	"messageBackground": { "fill": "#3b3b3b" },
	"messages": [
		{ "fill": "#545454" },
		{ "fill": "#540000" },
		{ "fill": "#545400" },
		{ "fill": "#005400" },
		{ "fill": "#005454" },
		{ "fill": "#000054" },
		{ "fill": "#540054" }
	],
	
	"progressBar": { "fill": [ "#f0f0f0", "#404040", "#000000", "#000000" ] },
	
	"callRow": { "fill": "#fe9d27", "color": "#ffffff" },
	"inspectFlash": { "color":  "#d9caff" },
	"inspectCache": { "color": "#ffdf9e" },

	"test262Fail": { "color": "#f26c4f" },
	"test262Pass": { "color": "#3cb878" },
	"test262Skip": { "color": "#77bafc" }
};

const themes = [
	liteColors,
	darkColors,
];
Object.freeze(themes, true)

const codeFonts = [
	"12px Fira Mono",
	"medium 12px Fira Mono",
];
Object.freeze(codeFonts, true)

const textures = {
	icons: { path:"assets/icons.png", scale:2 },
	findModes: { path:"assets/findModes.png", scale:2 },
	glyphs: { path:"assets/glyphs.png", scale:2 },
	logo: { path:"assets/logo.png", scale:2 },
	wait: { path:"assets/wait.png", scale:2 },
};

function buildTheme($, codeFont) {
	const colors = globalThis.colors = {};
	const skins = globalThis.skins = {};
	const styles = globalThis.styles = {};
	
	skins.button = new Skin({ fill:$.button.fill });
	styles.iconButton = new Style({ font:"semibold", color:$.button.color });
	skins.iconButton = new Skin({ fill:$.button.fill });
	styles.serialButton = new Style({ font:"semibold", color:$.button.color , left:15, right:15 });
	skins.icons = new Skin({ texture:textures.icons, color:$.button.color, x:2, y:2, width:26, height:26, variants:30 });
	skins.divider = new Skin({ fill:$.divider.fill });
	skins.fieldScroller = new Skin({ fill: [$.fieldScroller.fill], stroke:$.fieldScroller.stroke, borders: { left:1, right:1, bottom:1, top:1 } });
	skins.scrollbarThumb = new Skin({ fill:$.scrollbarThumb.fill });
	skins.horizontalScrollbar = new Skin({ fill:$.scrollbar.fill, stroke:$.scrollbar.stroke, top:1 });
	skins.verticalScrollbar = new Skin({ fill:$.scrollbar.fill, stroke:$.scrollbar.stroke, left:1 });
	skins.switchBar = new Skin({ fill:$.switchBar.fill, stroke:$.switchBar.stroke });
	skins.switchButton = new Skin({ fill:$.switchButton.fill, stroke:$.switchButton.stroke });
	
	// TABS
	skins.tabsPane = new Skin({ fill:$.tab.fill[1], stroke:$.divider.fill, borders: { bottom:1 }  });
	skins.tab = new Skin({ fill:$.tab.fill, stroke:$.tab.stroke, borders: { right:1 }  });
	styles.tab = new Style({ font:"semibold 12px Open Sans", color:$.tab.color, horizontal:"center", left:26, right:26 });
	styles.tabTest262 = new Style({ font:"semibold 12px Open Sans", color:$.tab.color, horizontal:"center", left:13, right:13 });
	styles.tabBreakpoint = new Style({ font:"bold 10px", horizontal:"right", right:15, color:$.tabBubble.color });
	skins.tabBubble = new Skin({ texture:textures.glyphs, color:$.tabBubble.fill, x:0, y:48, width:48, height:16 });
	styles.tabBubble = new Style({ font:"bold 10px", horizontal:"right", right:10, color:$.tabBreakpoint.color });

	// CODE, CONSOLE, PREFERENCES
	skins.paneBorder = new Skin({ fill:$.tableHeader.stroke });
	skins.paneHeader = new Skin({ fill:$.tableHeader.fill });
	styles.paneHeader = new Style({ font:"semibold", color:$.tableHeader.color, horizontal:"left" });

	// CODE
	styles.pathName = new Style({ font:"semibold", color:$.pathName.color, horizontal:"left" });
	styles.pathSpan = new Style({ font:"semibold 12px Open Sans", color:$.pathSpan.color, horizontal:"left" });
	
	skins.lineNumber = new Skin({ fill:$.lineNumber.fill, stroke:$.lineNumber.stroke, borders: { right:1 } });
	styles.lineNumber = new Style({ font:"bold 10px", horizontal:"right", right:15, color: [ $.lineNumber.color,$.lineBreakpoint.color ] });

	skins.lineBreakpoint = new Skin({ texture:textures.glyphs, color:["transparent", $.lineBreakpoint.fill], x:16, y:32, width:42, height:16, variants:42 });
	skins.lineCall = new Skin({ texture:textures.glyphs, color:["transparent", $.callRow.fill], x:0, y:32, width:16, height:16 });
	
	skins.background = new Skin({ fill:$.code.fill });
	skins.code = new Skin({ fill:[$.codeResult.fill, "transparent", $.codeSelection.fill, $.codeSelection.fill] });
	styles.code = new Style({ font:codeFont, horizontal:"left", left:8, right:8, color:[$.code.color, $.codeKeyword.color, $.codeLiteral.color, $.codeComment.color] });
	
	skins.noCode = new Skin({ fill:$.tableRow.fill});
	skins.logo = new Skin({ texture:textures.logo, x:0, y:0, width:80, height:80 });
	styles.error = new Style({ color:$.error.color });

	// FIND SEARCH
	styles.findLabel = new Style({ color:$.findLabel.color, horizontal:"left", left:5, right:5});
	skins.findModeButton = new Skin({ texture:textures.findModes, color:$.findMode.fill, x:0, y:20, width:20, height:20, variants:20 });
	skins.findModeIcon = new Skin({ texture:textures.findModes, color:$.findMode.color, x:0, y:0, width:20, height:20, variants:20 });
	
	skins.resultRow = new Skin({ fill:$.resultRow.fill });
	styles.resultCount = new Style({ color:$.resultCount.color, horizontal:"right", right:5 });
	skins.resultLabel = new Skin({ fill:["transparent", "transparent", $.codeResult.fill, $.codeResult.fill] });
	styles.resultLabel = new Style({ font:codeFont, color:$.code.color, horizontal:"left" });
	styles.searchEmpty = new Style({ color:$.infoRow.color });
	skins.wait = new Skin({ texture:textures.wait, x:0, y:0, width:20, height:20, variants:20 });

	// TABLES
	skins.glyphs = new Skin({ texture:textures.glyphs, color:$.tableRow.color, x:0, y:0, width:16, height:16, variants:16 });
	skins.tableRow = new Skin({ fill:$.tableRow.fill  });
	styles.tableRow = new Style({ color:$.tableRow.color, horizontal:"left" });
	skins.tableFooter = new Skin({ fill:$.tableHeader.fill[0], stroke:$.tableHeader.stroke, borders: { bottom:1 }  });
	skins.tableHeader = new Skin({ fill:$.tableHeader.fill, stroke:$.tableHeader.stroke, borders: { bottom:1 } });
	styles.tableHeader = new Style({ font:"bold", color:$.tableHeader.color, horizontal:"left" });
	skins.paneBackground = new Skin({ fill:$.tableHeader.fill[0] });
	skins.paneSeparator = new Skin({ fill:$.tableHeader.stroke });
	
	// FILES
	styles.breakpointRowName = new Style({ color:$.tableRow.color });
	styles.breakpointRowLine = new Style({ font:"light", color:$.tableRow.color });
	skins.fileRow = new Skin({ fill:$.fileRow.fill });
	styles.fileRow = new Style({ font:"semibold", color:$.fileRow.color, horizontal:"left" });
	styles.infoRow = new Style({ color:$.infoRow.color, horizontal:"left" });
		
	// MESSAGES
	skins.messageBackground = new Skin({ fill:$.tableHeader.fill });
	skins.conversation = new Skin({ texture:textures.glyphs, color:$.tableRow.color, x:64, y:0, width:16, height:16, variants:16 });
	styles.conversation = new Style({ font:codeFont, color:$.code.color, left:12, right:12, top:4, bottom:4, horizontal:"left" });
	skins.messages = [
		new Skin({ fill:$.messages[0].fill }),
		new Skin({ fill:$.messages[1].fill }),
		new Skin({ fill:$.messages[2].fill }),
		new Skin({ fill:$.messages[3].fill }),
		new Skin({ fill:$.messages[4].fill }),
		new Skin({ fill:$.messages[5].fill }),
		new Skin({ fill:$.messages[6].fill }),
	];
	skins.messageMarks = [
		new Skin({ texture:textures.glyphs, color:$.messages[0].fill, x:0, y:56, width:16, height:8, variants:80 }),	
		new Skin({ texture:textures.glyphs, color:$.messages[1].fill, x:0, y:56, width:16, height:8, variants:80 }),	
		new Skin({ texture:textures.glyphs, color:$.messages[2].fill, x:0, y:56, width:16, height:8, variants:80 }),	
		new Skin({ texture:textures.glyphs, color:$.messages[3].fill, x:0, y:56, width:16, height:8, variants:80 }),	
		new Skin({ texture:textures.glyphs, color:$.messages[4].fill, x:0, y:56, width:16, height:8, variants:80 }),	
		new Skin({ texture:textures.glyphs, color:$.messages[5].fill, x:0, y:56, width:16, height:8, variants:80 }),	
		new Skin({ texture:textures.glyphs, color:$.messages[6].fill, x:0, y:56, width:16, height:8, variants:80 }),	
	];
	styles.message = new Style({ font:codeFont, color:$.code.color, top:5, bottom:5, horizontal:"left" });
		
	// SERIAL
	skins.progressBar = new Skin({ fill:$.progressBar.fill });
	
	// TEST262
	skins.filterTest262 = new Skin({ texture:textures.glyphs, color:$.tableRow.color, x:0, y:16, width:16, height:16, variants:16 });
	styles.test262Headers = [
		new Style({ font:"bold", color:$.test262Fail.color, horizontal:"right", right:2 }),
		new Style({ font:"bold", color:$.test262Fail.color, horizontal:"left", left:2 }),
		new Style({ font:"bold", color:$.test262Pass.color, horizontal:"right", right:2 }),
		new Style({ font:"bold", color:$.test262Pass.color, horizontal:"left", left:2 }),
		new Style({ font:"bold", color:$.test262Skip.color, horizontal:"right", right:2 }),
		new Style({ font:"bold", color:$.test262Skip.color, horizontal:"left", left:2 }),
	];
		
	// DEBUG
	skins.callRow = new Skin({ fill:$.callRow.fill  });
	styles.callRow = new Style({ font:"semibold", color:$.callRow.color, horizontal:"left" });
	styles.debugRowName = new Style({ color:[$.tableRow.color, $.inspectFlash.color, $.inspectCache.color, $.tableRow.color] });
	styles.debugRowValue = new Style({ font:"light", color:[$.tableRow.color, $.inspectFlash.color, $.inspectCache.color, $.tableRow.color] });
	styles.instrumentRowName = new Style({ font:"10px", color:$.tableRow.color, horizontal:"left", vertical:"top" });
	styles.instrumentRowValue = new Style({ font:"light 10px", color:$.tableRow.color, horizontal:"right", vertical:"top" });
	colors.instrumentBar = $.tableRow.color;
	colors.instrumentBarHover = $.callRow.fill;
	colors.instrumentLine = $.tableHeader.stroke;
		
	// PREFERENCES	
	skins.preferenceHeader = new Skin({ fill:$.tableRow.fill });
	skins.preferenceRow = new Skin({ fill:$.tableRow.fill });
	styles.preferenceComment = new Style({ font:"light", color:$.tableRow.color, horizontal:"left", left:10 });
	styles.preferenceFirstName = new Style({ font:"semibold", color:$.tableRow.color, horizontal:"left" });
	styles.preferenceSecondName = new Style({ font:"semibold", color:$.tableRow.color, horizontal:"left" });
	styles.preferenceThirdName = new Style({ color:$.tableRow.color, horizontal:"left" });
	styles.preferenceValue = new Style({ color:$.tableRow.color, horizontal:"left", left:2 });
}

export function buildAssets(which) { 
	buildTheme(themes[which], codeFonts[which]);
}

export const headerHeight = 26;
export const footerHeight = 3;
export const rowHeight = 18;
export const rowIndent = 18;
