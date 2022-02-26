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

const INSPECTOR_FLASH = "#082080";
const INSPECTOR_CACHE = "#804f13";

const TRANSPARENT = "transparent";

const LITE_CODE_COMMENT = "#008d32";
const LITE_CODE_KEYWORD = "#103ffb";
const LITE_CODE_LITERAL = "#b22821";
const LITE_CODE_RESULT = "#FBF4BB";
const LITE_CODE_SELECTION = "#D1E0FF";
const FILE_SELECTION = "#6497ff";
const FRAME_SELECTION = "#fe9d27";


const DARK_CODE_COMMENT = "#00e550";
const DARK_CODE_KEYWORD = "#7e97fb";
const DARK_CODE_LITERAL = "#f7897a";
const DARK_CODE_RESULT = "#FBF4BB";
const DARK_CODE_SELECTION = "#495c82";



const BLACK = "black";
const WHITE = "white";
var LITE_GRAYS = new Array(101).fill();
var DARK_GRAYS = new Array(101).fill();
for (let i = 0; i <= 100; i++) {
	LITE_GRAYS[i] = blendColors(i / 100, WHITE, BLACK);
	DARK_GRAYS[i] = blendColors(i / 100, "#202020", WHITE);
}
const BLUE = "#192eab";
const ORANGE = "#f68100"

const liteColors = {
	".button:disabled": { "backgroundColor": "transparent", "color": "#808080" },
	".button": { "backgroundColor": "transparent", "color": "#262626" },
	".button:hover": { "backgroundColor": "#ffffff", "color": "#262626" },
	".button:active": { "backgroundColor": "#192eab", "color": "#ffffff" },
	".divider": { "backgroundColor": "#c2c2c2" },
	".fieldScroller": { "backgroundColor": "#ffffff", "borderColor": "#f0f0f0" },
	".scrollbar": { "backgroundColor": "transparent", "borderColor": "transparent" },
	".scrollbar:hover": { "backgroundColor": "#fafafa", "borderColor": "#dbdbdb" },
	".scrollbar:active": { "backgroundColor": "#fafafa", "borderColor": "#dbdbdb" },
	".scrollbarThumb:disabled": { "backgroundColor": "#e0e0e000" },
	".scrollbarThumb": { "backgroundColor": "#e0e0e0" },
	".scrollbarThumb:hover": { "backgroundColor": "#e6e6e6" },
	".scrollbarThumb:active": { "backgroundColor": "#e6e6e6" },
	
	".switch:disabled": { "backgroundColor": "transparent", "borderColor":"#e6e6e6" },
	".switch": { "backgroundColor": "#b0b0b0", "borderColor":"#b0b0b0" },
	".switch:hover": { "backgroundColor": "#b0b0b0", "borderColor":"#b0b0b0" },
	".switch:active": { "backgroundColor": "#192eab", "borderColor":"#192eab" },
	".switch .thumb:disabled": { "backgroundColor": "transparent", "borderColor":"#e6e6e6" },
	".switch .thumb": { "backgroundColor": "#ffffff", "borderColor":"#ffffff" },
	".switch .thumb:hover": { "backgroundColor": "#ffffff", "borderColor":"#ffffff" },
	".switch .thumb:active": { "backgroundColor": "#ffffff", "borderColor":"#ffffff" },

	".tabsPane": { "backgroundColor": "#dbdbdb" },
	".tab:active": { "backgroundColor": "#f0f0f0", "color": "#000000" },
	".tab": { "backgroundColor": "#dbdbdb", "color": "#404040", "borderColor": "#c2c2c2" },
	".tab:hover": { "backgroundColor": "#e6e6e6", "color": "#000000" },
	".tabBreakpoint": { "color": "#ffffff" },
	".tabBubble": { "color": "#ffffff" },

	".paneHeader": { "backgroundColor": "#f0f0f0", "borderColor": "#dbdbdb", "color": "#404040"  },
	".paneHeader:hover": { "backgroundColor": "#e6e6e6", "color": "#404040" },
	".paneHeader:active": { "backgroundColor": "#dbdbdb", "color": "#404040"  },
	
	".pathName": { "color": "#000000" },
	".pathSpan": { "color": "#ababab" },
	".pathSpan:hover": { "color": "#787878" },
	".pathSpan:active": { "color": "#454545" },

	".lineNumber": { "color": "#ababab" },
	".lineNumber .breakpoint": { "color": "#ffffff" },
	".lineNumbers": { "backgroundColor": "#fafafa", "borderColor": "#dbdbdb" },

	".background": { "backgroundColor": "#ffffff" },
	".code": { "backgroundColor": "#ffffff", "color": "#000000" },
	".code .result": { "backgroundColor": "#fbf4bb" },
	".code .selection": { "backgroundColor": "#d1e0ff" },
	".code .literal": { "color": "#b22821" },
	".code .keyword": { "color": "#103ffb" },
	".code .comment": { "color": "#008d32" },

	".error": { "color": "#262626" },

	".findLabel": { "color": "#000000" },
	".findMode": { "backgroundColor": "transparent", "color": "#ffffff" },
	".findMode:hover": { "backgroundColor": "#e0e0e0", "color": "#e0e0e0" },
	".findMode:active": { "backgroundColor": "#a0a0a0", "color": "#555555" },
	".resultCount": { "color": "#808080" },
	".resultRow": { "backgroundColor": "#ffffff" },
	".resultRow:hover": { "backgroundColor": "#fafafa" },
	".resultRow:active": { "backgroundColor": "#f0f0f0" },
	".searchEmpty": { "color": "#808080" },
	
	".tableHeader": { "backgroundColor": "#f0f0f0", "borderColor": "#dbdbdb", "color": "#404040" },
	".tableHeader:hover": { "backgroundColor": "#e6e6e6", "color": "#404040" },
	".tableHeader:active": { "backgroundColor": "#dbdbdb", "color": "#404040" },
	".tableRow": { "backgroundColor": "#fafafa", "color": "#000000" },
	".tableRow:hover": { "backgroundColor": "#f0f0f0", "color": "#000000" },
	".tableRow:active": { "backgroundColor": "#e6e6e6", "color": "#000000" },
	
	".fileRow:selected": { "backgroundColor": "#6497ff", "color": "#ffffff" },
	".infoRow": { "color": "#808080" },

	".messages-0": { "backgroundColor": "#ffffff", "borderColor":"#dbdbdb" },
	".messages-1": { "backgroundColor": "#ffefef", "borderColor":"#dbdbdb" },
	".messages-2": { "backgroundColor": "#ffffef", "borderColor":"#dbdbdb" },
	".messages-3": { "backgroundColor": "#efffef", "borderColor":"#dbdbdb" },
	".messages-4": { "backgroundColor": "#efffff", "borderColor":"#dbdbdb" },
	".messages-5": { "backgroundColor": "#efefff", "borderColor":"#dbdbdb" },
	".messages-6": { "backgroundColor": "#ffefff", "borderColor":"#dbdbdb" },
	
	".progressBar": { "backgroundColor": "#f0f0f0" },
	".progressBar:hover": { "backgroundColor": "#404040" },
	
	".test262 .fail": { "color": "#b22821" },
	".test262 .pass": { "color": "#008d32" },
	".test262 .skip": { "color": "#103ffb" },
	
	".callRow:selected": { "backgroundColor": "#fe9d27", "color": "#ffffff" },
	".debugRow": { "color": "#000000" },
	".debugRow .flash": { "color": "#082080" },
	".debugRow .cache": { "color": "#804f13" },
	".instrumentRowName": { "color": "#000000" },
	".instrumentRowValue": { "color": "#000000" },
	".instrumentBar": { "color": "#404040" },
	".instrumentBar:hover": { "color": "#fe9d27" },
	".instrumentLine": { "color": "#dbdbdb" },
};

const darkColors = {
	
	".button:disabled": { "backgroundColor": "transparent", "color": "#808080" },
	".button": { "backgroundColor": "transparent", "color": "#ffffff" },
	".button:hover": { "backgroundColor": "#000000", "color": "#ffffff" },
	".button:active": { "backgroundColor": "#f68100", "color": "#ffffff" },
	".fieldScroller": { "backgroundColor": "#1e1e1e", "borderColor": "#545454" },
	".divider": { "backgroundColor": "#545454" },
	".scrollbar": { "backgroundColor": "transparent", "borderColor": "transparent" },
	".scrollbar:hover": { "backgroundColor": "#3b3b3b", "borderColor": "#454545" },
	".scrollbar:active": { "backgroundColor": "#3b3b3b", "borderColor": "#454545" },
	".scrollbarThumb:disabled": { "backgroundColor": "#a2a2a200" },
	".scrollbarThumb": { "backgroundColor": "#a2a2a2" },
	".scrollbarThumb:hover": { "backgroundColor": "#a2a2a2" },
	".scrollbarThumb:active": { "backgroundColor": "#a2a2a2" },
	".switch:disabled": { "backgroundColor": "transparent", "borderColor":"#5d5d5d" },
	".switch": { "backgroundColor": "#5d5d5d", "borderColor":"#5d5d5d" },
	".switch:hover": { "backgroundColor": "#5d5d5d", "borderColor":"#5d5d5d" },
	".switch:active": { "backgroundColor": "#f68100", "borderColor":"#f68100" },
	".switch .thumb:disabled": { "backgroundColor": "transparent", "borderColor":"#5d5d5d" },
	".switch .thumb": { "backgroundColor": "#e4e4e4", "borderColor":"#e4e4e4" },
	".switch .thumb:hover": { "backgroundColor": "#e4e4e4", "borderColor":"#e4e4e4" },
	".switch .thumb:active": { "backgroundColor": "#e4e4e4", "borderColor":"#e4e4e4" },
	
	".tabsPane": { "backgroundColor": "#212121" },
	".tab:active": { "backgroundColor": "#3b3b3b", "color": "#ffffff" },
	".tab": { "backgroundColor": "#212121", "color": "#c0c0c0", "borderColor": "#4a4a4a" },
	".tab:hover": { "backgroundColor": "#343434", "color": "#e0e0e0" },
	".tabBreakpoint": { "color": "#ffffff" },
	".tabBubble": { "color": "#ffffff" },
	
	".paneHeader": { "backgroundColor": "#3b3b3b", "borderColor": "#454545", "color": "#f0f0f0" },
	".paneHeader:hover": { "backgroundColor": "#4b4b4b", "color": "red" },
	".paneHeader:active": { "backgroundColor": "#5b5b5b", "color": "blue" },
	
	".pathName": { "color": "#f0f0f0" },
	".pathSpan": { "color": "#a0a0a0" },
	".pathSpan:hover": { "color": "#c0c0c0" },
	".pathSpan:active": { "color": "#f0f0f0" },
	
	".lineNumber": { "color": "#ababab" },
	".lineNumber .breakpoint": { "color": "#ffffff" },
	".lineNumbers": { "backgroundColor": "#333333", "borderColor": "#545454" },

	".background": { "backgroundColor": "#2a2a2a" },
	".code": { "backgroundColor": "#2a2a2a", "color": "#f0f0f0" },
	".code .result": { "backgroundColor": "#7d4900" },
	".code .selection": { "backgroundColor": "#003663" },
	".code .keyword": { "color": "#77bafc" },
	".code .literal": { "color": "#f26c4f" },
	".code .comment": { "color": "#3cb878" },
	".error": { "color": "#262626" },

	".findLabel": { "color": "#000000" },
	".findMode": { "backgroundColor": "transparent", "color": "#e0e0e0" },
	".findMode:hover": { "backgroundColor": "#4b4b4b", "color": "#ffffff" },
	".findMode:active": { "backgroundColor": "#5b5b5b", "color": "#e0e0e0" },
	".resultCount": { "color": "#808080" },
	".resultRow": { "backgroundColor": "#2a2a2a" },
	".resultRow:hover": { "backgroundColor": "#3a3a3a" },
	".resultRow:active": { "backgroundColor": "#4a4a4a" },
	".searchEmpty": { "color": "#808080" },

	".tableHeader": { "backgroundColor": "#3b3b3b", "borderColor": "#4e4e4e", "color": "#f0f0f0" },
	".tableHeader:hover": { "backgroundColor": "#4b4b4b", "color": "red" },
	".tableHeader:active": { "backgroundColor": "#5b5b5b", "color": "blue" },
	".tableRow": { "backgroundColor": "#272727", "color": "#ffffff" },
	".tableRow:hover": { "backgroundColor": "#373737", "color": "#ffffff" },
	".tableRow:active": { "backgroundColor": "#474747", "color": "#ffffff" },

	".fileRow:selected": { "backgroundColor": "#6497ff", "color": "#ffffff" },
	".infoRow": { "color": "#808080" },

	".messages-0": { "backgroundColor": "#545454", "borderColor":"#545454" },
	".messages-1": { "backgroundColor": "#540000", "borderColor":"#540000" },
	".messages-2": { "backgroundColor": "#545400", "borderColor":"#545400" },
	".messages-3": { "backgroundColor": "#005400", "borderColor":"#005400" },
	".messages-4": { "backgroundColor": "#005454", "borderColor":"#005454" },
	".messages-5": { "backgroundColor": "#000054", "borderColor":"#000054" },
	".messages-6": { "backgroundColor": "#540054", "borderColor":"#540054" },
	
	".progressBar": { "backgroundColor": "#f0f0f0" },
	".progressBar:hover": { "backgroundColor": "#404040" },
	
	".test262 .fail": { "color": "#f26c4f" },
	".test262 .pass": { "color": "#3cb878" },
	".test262 .skip": { "color": "#77bafc" },
	
	".callRow:selected": { "backgroundColor": "#fe9d27", "color": "#ffffff" },
	".debugRow": { "color": "#ffffff" },
	".debugRow .flash": { "color": "#d9caff" },
	".debugRow .cache": { "color": "#ffdf9e" },
	".instrumentRowName": { "color": "#000000" },
	".instrumentRowValue": { "color": "#000000" },
	".instrumentBar": { "color": "#f0f0f0" },
	".instrumentBar:hover": { "color": "#fe9d27" },
	".instrumentLine": { "color": "#4e4e4e" },
};

const themes = [
	liteColors,
	darkColors,
];
Object.freeze(themes, true)

const textures = {
	icons: { path:"assets/icons.png", scale:2 },
	findModes: { path:"assets/findModes.png", scale:2 },
	glyphs: { path:"assets/glyphs.png", scale:2 },
	lineNumber: { path:"assets/flags.png", scale:2 },
	logo: { path:"assets/logo.png", scale:2 },
	wait: { path:"assets/wait.png", scale:2 },
};

function buildTheme($, codeFont) {
	function c(selector) {
		return $[selector].color;
	}
	function f(selector) {
		return $[selector].backgroundColor;
	}
	function s(selector) {
		return $[selector].borderColor;
	}
	const colors = globalThis.colors = {};
	const skins = globalThis.skins = {};
	const styles = globalThis.styles = {};
	
	skins.button = new Skin({ fill:[f('.button:disabled'),f('.button'),f('.button:hover'),f('.button:active')] });
	styles.iconButton = new Style({ font:"semibold", color:[c('.button:disabled'),c('.button'),c('.button:hover'),c('.button:active')] });
	skins.iconButton = new Skin({ fill:[f('.button:disabled'),f('.button'),f('.button:hover'),f('.button:active')] });
	styles.serialButton = new Style({ font:"semibold", color:[c('.button:disabled'),c('.button'),c('.button:hover'),c('.button:active')] , left:15, right:15 });
	skins.icons = new Skin({ texture:textures.icons, color:[c('.button:disabled'),c('.button'),c('.button:hover'),c('.button:active')], x:2, y:2, width:26, height:26, variants:30 });
	skins.divider = new Skin({ fill:f('.divider') });
	skins.fieldScroller = new Skin({ fill: [f('.fieldScroller')], stroke:s('.fieldScroller'), borders: { left:1, right:1, bottom:1, top:1 } });
	skins.scrollbarThumb = new Skin({ fill:[f('.scrollbarThumb:disabled'),f('.scrollbarThumb'),f('.scrollbarThumb:hover'),f('.scrollbarThumb:active')] });
	skins.horizontalScrollbar = new Skin({ fill:[f('.scrollbar'),f('.scrollbar:hover'),f('.scrollbar:active')], stroke:[s('.scrollbar'),s('.scrollbar:hover'),s('.scrollbar:active')], top:1 });
	skins.verticalScrollbar = new Skin({ fill:[f('.scrollbar'),f('.scrollbar:hover'),f('.scrollbar:active')], stroke:[s('.scrollbar'),s('.scrollbar:hover'),s('.scrollbar:active')], left:1 });
	
	// TABS
	skins.tabsPane = new Skin({ fill:f('.tabsPane'), stroke:f('.divider'), borders: { bottom:1 }  });
	skins.tab = new Skin({ fill:[f('.tab:active'),f('.tab'),f('.tab:hover')], stroke:s('.tab'), borders: { right:1 }  });
	styles.tab = new Style({ font:"semibold 12px Open Sans", color:[c('.tab:active'),c('.tab'),c('.tab:hover')], horizontal:"center", left:26, right:26 });
	styles.tabTest262 = new Style({ font:"semibold 12px Open Sans", color:[c('.tab:active'),c('.tab'),c('.tab:hover')], horizontal:"center", left:13, right:13 });
	skins.tabBreakpoint = new Skin({ texture:textures.lineNumber, x:16, y:16, width:24, height:16, tiles: { left:2, right: 12 }, });
	styles.tabBreakpoint = new Style({ font:"bold 10px", horizontal:"right", right:15, color: LITE_GRAYS[0] });
	skins.tabBubble = new Skin({ texture:textures.lineNumber, x:0, y:48, width:48, height:16, tiles: { left:16, right: 16 }, });
	styles.tabBubble = new Style({ font:"bold 10px", horizontal:"right", right:10, color: LITE_GRAYS[0] });
	skins.tabBroken = new Skin({ texture:textures.lineNumber, x:40, y:0, width:16, height:16 });

	// CODE, CONSOLE, PREFERENCES
	skins.paneBorder = new Skin({ fill:s('.paneHeader') });
	skins.paneHeader = new Skin({ fill:[f('.paneHeader'),f('.paneHeader:hover'),f('.paneHeader:active')] });
	styles.paneHeader = new Style({ font:"semibold", color:c('.paneHeader'), horizontal:"left" });

	// CODE
	styles.pathName = new Style({ font:"semibold", color:c('.pathName'), horizontal:"left" });
	styles.pathSpan = new Style({ font:"semibold 12px Open Sans", color:[c('.pathSpan'),c('.pathSpan'),c('.pathSpan:hover'),c('.pathSpan:active')], horizontal:"left" });
	
	skins.lineNumber = new Skin({ texture:textures.lineNumber, x:0, y:0, width:40, height:16, tiles: { left:20, right: 12 }, states:16, variants:40 });
	styles.lineNumber = new Style({ font:"bold 10px", horizontal:"right", right:15, color: [ c('.lineNumber'),c('.lineNumber .breakpoint') ] });
	skins.lineNumbers = new Skin({ fill:f('.lineNumbers'), stroke:s('.lineNumbers'), borders: { right:1 } });
	
	skins.background = new Skin({ fill:f('.background') });
	skins.code = new Skin({ fill:[f('.code .result'), TRANSPARENT, f('.code .selection'), f('.code .selection')] });
	styles.code = new Style({ font:codeFont, horizontal:"left", left:8, right:8, color: [c('.code'),c('.code .keyword'),c('.code .literal'),c('.code .comment')]});
	
	skins.noCode = new Skin({ fill:f('.tableRow')});
	skins.logo = new Skin({ texture:textures.logo, x:0, y:0, width:80, height:80 });
	styles.error = new Style({ color:LITE_GRAYS[85] });

	// FIND SEARCH
	styles.findLabel = new Style({ color:LITE_GRAYS[100], horizontal:"left", left:5, right:5});
	skins.findModeButton = new Skin({ texture:textures.findModes, color:[TRANSPARENT,f('.findMode'),f('.findMode:hover'),f('.findMode:active')], x:0, y:20, width:20, height:20, variants:20 });
	skins.findModeIcon = new Skin({ texture:textures.findModes, color:[TRANSPARENT,c('.findMode'),c('.findMode:hover'),c('.findMode:active')], x:0, y:0, width:20, height:20, variants:20 });
	
	skins.resultRow = new Skin({ fill:[f('.resultRow'),f('.resultRow:hover'),f('.resultRow:active'),f('.resultRow')] });
	styles.resultCount = new Style({ color:LITE_GRAYS[50], horizontal:"right", right:5 });
	skins.resultLabel = new Skin({ fill:[TRANSPARENT, TRANSPARENT, f('.code .result'), f('.code .result')] });
	styles.resultLabel = new Style({ font:codeFont, color:c('.code'), horizontal:"left" });
	styles.searchEmpty = new Style({ color:c('.infoRow') });
	skins.wait = new Skin({ texture:textures.wait, x:0, y:0, width:20, height:20, variants:20 });

	// TABLES
	skins.glyphs = new Skin({ texture:textures.glyphs, color:[c('.tableRow'),c('.tableRow:hover'),c('.tableRow:active'),c('.tableRow')], x:0, y:0, width:16, height:16, variants:16 });
	skins.tableRow = new Skin({ fill:[f('.tableRow'),f('.tableRow:hover'),f('.tableRow:active'),f('.tableRow')]  });
	styles.tableRow = new Style({ color:[c('.tableRow'),c('.tableRow:hover'),c('.tableRow:active'),c('.tableRow')], horizontal:"left" });
	skins.tableFooter = new Skin({ fill:f('.tableRow'), stroke:s('.tableHeader'), borders: { bottom:1 }  });
	skins.tableHeader = new Skin({ fill:[f('.tableHeader'),f('.tableHeader:hover'),f('.tableHeader:active'),f('.tableHeader')], stroke:s('.tableHeader'), borders: { bottom:1 } });
	styles.tableHeader = new Style({ font:"bold", color:c('.tableHeader'), horizontal:"left" });
	skins.paneBackground = new Skin({ fill:f('.tableHeader') });
	skins.paneSeparator = new Skin({ fill:s('.tableHeader') });
	
	// FILES
	styles.breakpointRowName = new Style({ color:[c('.tableRow'),c('.tableRow:hover'),c('.tableRow:active'),c('.tableRow')] });
	styles.breakpointRowLine = new Style({ font:"light", color:[c('.tableRow'),c('.tableRow:hover'),c('.tableRow:active'),c('.tableRow')] });
	skins.fileRow = new Skin({ fill:[f('.tableRow'),f('.tableRow:hover'),f('.tableRow:active'),f('.fileRow:selected')] });
	styles.fileRow = new Style({ font:"semibold", color:c('.fileRow:selected'), horizontal:"left" });
	styles.infoRow = new Style({ color:c('.infoRow'), horizontal:"left" });
		
	// MESSAGES
	skins.messageBackground = new Skin({ fill:f('.tableHeader') });
	skins.conversation = new Skin({ texture:textures.glyphs, color:[c('.tableRow'),c('.tableRow:hover'),c('.tableRow:active'),c('.tableRow')], x:64, y:0, width:16, height:16, variants:16 });
	styles.conversation = new Style({ font:codeFont, color:c('.code'), left:12, right:12, top:4, bottom:4, horizontal:"left" });
	skins.messages = [
		new Skin({ fill:f('.messages-0'), stroke:s('.messages-0') }),
		new Skin({ fill:f('.messages-1'), stroke:s('.messages-1') }),
		new Skin({ fill:f('.messages-2'), stroke:s('.messages-2') }),
		new Skin({ fill:f('.messages-3'), stroke:s('.messages-3') }),
		new Skin({ fill:f('.messages-4'), stroke:s('.messages-4') }),
		new Skin({ fill:f('.messages-5'), stroke:s('.messages-5') }),
		new Skin({ fill:f('.messages-6'), stroke:s('.messages-6') }),
	];
	styles.message = new Style({ font:codeFont, color:[c('.code'),c('.code .keyword'),c('.code .literal'),c('.code .comment')], top:5, bottom:5, horizontal:"left" });
		
	// SERIAL
	skins.progressBar = new Skin({ fill:[f('.progressBar'),f('.progressBar:hover')] });
	
	// TEST262
	skins.filterTest262 = new Skin({ texture:textures.glyphs, color:[c('.tableRow'),c('.fileRow:selected')], x:0, y:16, width:16, height:16, variants:16 });
	styles.test262Headers = [
		new Style({ font:"bold", color:c('.test262 .fail'), horizontal:"right", right:2 }),
		new Style({ font:"bold", color:c('.test262 .fail'), horizontal:"left", left:2 }),
		new Style({ font:"bold", color:c('.test262 .pass'), horizontal:"right", right:2 }),
		new Style({ font:"bold", color:c('.test262 .pass'), horizontal:"left", left:2 }),
		new Style({ font:"bold", color:c('.test262 .skip'), horizontal:"right", right:2 }),
		new Style({ font:"bold", color:c('.test262 .skip'), horizontal:"left", left:2 }),
	];
		
	// DEBUG
	skins.callRow = new Skin({ fill:[f('.tableRow'),f('.tableRow:hover'),f('.tableRow:active'),f('.callRow:selected')]  });
	styles.callRow = new Style({ font:"semibold", color:c('.callRow:selected'), horizontal:"left" });
	styles.debugRowName = new Style({ color:[c('.debugRow'),c('.debugRow .flash'),c('.debugRow .cache'),c('.tableRow')] });
	styles.debugRowValue = new Style({ font:"light", color:[c('.debugRow'),c('.debugRow .flash'),c('.debugRow .cache'),c('.tableRow')] });
	styles.instrumentRowName = new Style({ font:"10px", color:c('.tableRow'), horizontal:"left", vertical:"top" });
	styles.instrumentRowValue = new Style({ font:"light 10px", color:c('.tableRow'), horizontal:"right", vertical:"top" });
	colors.instrumentBar = c('.instrumentBar');
	colors.instrumentBarHover = c('.instrumentBar:hover');
	colors.instrumentLine = c('.instrumentLine');
		
	// PREFERENCES	
	skins.preferenceHeader = new Skin({ fill:[f('.tableRow'),f('.tableRow:hover'),f('.tableRow:active'),f('.tableRow')] });
	skins.preferenceRow = new Skin({ fill:[f('.tableRow'),f('.tableRow:hover'),f('.tableRow:active'),f('.tableRow')] });
	styles.preferenceComment = new Style({ font:"light", color:[c('.tableRow'),c('.tableRow:hover'),c('.tableRow:active'),c('.tableRow')], horizontal:"left", left:10 });
	styles.preferenceFirstName = new Style({ font:"semibold", color:[c('.tableRow'),c('.tableRow:hover'),c('.tableRow:active'),c('.tableRow')], horizontal:"left" });
	styles.preferenceSecondName = new Style({ font:"semibold", color:[c('.tableRow'),c('.tableRow:hover'),c('.tableRow:active'),c('.tableRow')], horizontal:"left" });
	styles.preferenceThirdName = new Style({ color:[c('.tableRow'),c('.tableRow:hover'),c('.tableRow:active'),c('.tableRow')], horizontal:"left" });
	styles.preferenceValue = new Style({ color:[c('.tableRow'),c('.tableRow:hover'),c('.tableRow:active'),c('.tableRow')], horizontal:"left", left:2 });
	skins.switchBar = new Skin({ fill:[f('.switch:disabled'),f('.switch'),f('.switch:active'),f('.switch:active')], stroke:[s('.switch:disabled'),s('.switch'),s('.switch:active'),s('.switch:active')] });
	skins.switchButton = new Skin({ fill:[f('.switch .thumb:disabled'),f('.switch .thumb'),f('.switch .thumb:active'),f('.switch .thumb:active')], stroke:[s('.switch .thumb:disabled'),s('.switch .thumb'),s('.switch .thumb:active'),s('.switch .thumb:active')] });
}

const codeFonts = [
	"11px Fira Mono",
	"medium 12px Fira Mono",
];
Object.freeze(codeFonts, true)

export function buildAssets(which) { 
	buildTheme(themes[which], codeFonts[which]);
}

export const headerHeight = 26;
export const footerHeight = 3;
export const rowHeight = 18;
export const rowIndent = 18;
