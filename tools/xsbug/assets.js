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

const BLACK = "black";
const TRANSPARENT = "transparent";
const WHITE = "white";

const CODE_COMMENT = "#008d32";
const CODE_KEYWORD = "#103ffb";
const CODE_LITERAL = "#b22821";
const CODE_RESULT = "#FBF4BB";
const CODE_SELECTION = "#D1E0FF";
const FILE_SELECTION = "#6497ff";
const FRAME_SELECTION = "#fe9d27";

const INSPECTOR_FLASH = "#082080";
const INSPECTOR_CACHE = "#804f13";

var GRAYS = new Array(100).fill();
for (let i = 0; i < 100; i++) {
	GRAYS[i] = blendColors(i / 100, WHITE, BLACK);
}

const buttonsTexture = new Texture({ path:"assets/buttons.png", scale:2 });
const findModesTexture = new Texture({ path:"assets/findModes.png", scale:2 });
const glyphsTexture = new Texture({ path:"assets/glyphs.png", scale:2 });
const lineNumberTexture = new Texture({ path:"assets/flags.png", scale:2 });
const logoTexture = new Texture({ path:"assets/logo.png", scale:2 });
const toggleTexture = new Texture({ path:"assets/toggle.png", scale:2 });
const waitTexture = new Texture({ path:"assets/wait.png", scale:2 });

export const applicationStyle = new Style({ font:"12px Open Sans" });
export const backgroundSkin = new Skin({ fill:WHITE });

export const dividerSkin = new Skin({ fill:GRAYS[24] });

export const messageTexture = new Texture({ path:"assets/bubbles.png" });
export const messageLeftSkin = new Skin({ texture: messageTexture, x:0, y:0, width:64, height:32, tiles:{ left:16, right:16, top:12, bottom:12 }, variants:64, states:32 });
export const messageRightSkin = new Skin({ texture: messageTexture, x:0, y:128, width:64, height:32, tiles:{ left:16, right:16, top:12, bottom:12 }, variants:64, states:32 });
export const messageCenterSkin = new Skin({ texture: messageTexture, x:0, y:256, width:64, height:32, tiles:{ left:16, right:16, top:12, bottom:12 }, variants:64, states:32 });
export const messageStyle = new Style({ font:"11px Fira Mono", color:[ BLACK, CODE_KEYWORD, CODE_LITERAL, CODE_COMMENT ], top:5, bottom:5, horizontal:"left" });
export const conversationSkin = new Skin({ texture:lineNumberTexture, x:48, y:48, width:16, height:16, variants:16 });
export const conversationStyle = new Style({ font:"11px Fira Mono", color:BLACK, left:15, right:15, top:5, bottom:5, horizontal:"left" });


export const buttonSkin = new Skin({ texture:buttonsTexture, x:0, y:0, width:60, height:30, states:30, tiles:{ left:15, right:15 } });
export const buttonStyle = new Style({ font:"semibold", color:[GRAYS[19], GRAYS[85], GRAYS[85], WHITE ] });
export const buttonsSkin = new Skin({ texture:buttonsTexture, x:62, y:2, width:26, height:26, variants:30, states:30 });

export const glyphsSkin = new Skin({ texture:glyphsTexture, x:0, y:0, width:20, height:20, variants:20, states:20 });


export const lineNumberSkin = new Skin({ texture:lineNumberTexture, x:0, y:0, width:40, height:16, tiles: { left:20, right: 12 }, states:16, variants:40 });
export const lineNumberStyle = new Style({ font:"bold 10px", horizontal:"right", right:15, color: [ GRAYS[33], WHITE, WHITE ] })
export const lineNumbersSkin = new Skin({ fill:GRAYS[2], stroke:GRAYS[10], borders: { right:1 } })

export const logoSkin = new Skin({ texture:logoTexture, x:0, y:0, width:80, height:80 });

export const codeSkin = new Skin({ fill:[CODE_RESULT, TRANSPARENT, CODE_SELECTION, CODE_SELECTION] })
export const codeStyle = new Style({ font:"Fira Mono", horizontal:"left", left:8, right:8, color: [ BLACK, CODE_KEYWORD, CODE_LITERAL, CODE_COMMENT ]});


export const tabsPaneSkin = new Skin({ fill:GRAYS[14], stroke:GRAYS[24], borders: { bottom:1 }  });
export const tabSkin = new Skin({ fill:[GRAYS[6], GRAYS[14], GRAYS[10], GRAYS[6]], stroke:GRAYS[24], borders: { right:1 }  });
export const tabStyle = new Style({ font:"semibold 12px Open Sans", color:[BLACK, GRAYS[75], GRAYS[75], BLACK], horizontal:"center", left:26, right:26 });
export const tabBreakpointSkin = new Skin({ texture:lineNumberTexture, x:16, y:16, width:24, height:16, tiles: { left:2, right: 12 }, });
export const tabBreakpointStyle = new Style({ font:"bold 10px", horizontal:"right", right:15, color: WHITE });
export const tabBubbleSkin = new Skin({ texture:lineNumberTexture, x:0, y:48, width:48, height:16, tiles: { left:16, right: 16 }, });
export const tabBubbleStyle = new Style({ font:"bold 10px", horizontal:"right", right:10, color: WHITE });
export const tabBrokenSkin = new Skin({ texture:lineNumberTexture, x:40, y:0, width:16, height:16 });
export const tabTest262Skin = new Skin({ texture:lineNumberTexture, x:0, y:64, width:48, height:16, tiles: { left:16, right: 16 }, });
export const tabTest262Style = new Style({ font:"bold 10px", horizontal:"right", right:5, color: BLACK });
export const filterTest262Skin = new Skin({ texture:lineNumberTexture, x:0, y:80, width:16, height:16, variants:16, states:16 });

export const paneBackgroundSkin = new Skin({ fill:GRAYS[6] });
export const paneHeaderSkin = new Skin({ fill:[GRAYS[6], GRAYS[10], GRAYS[14], GRAYS[6]] });
export const paneHeaderStyle = new Style({ font:"semibold", color:GRAYS[85], horizontal:"left" });
export const paneBorderSkin = new Skin({ fill:GRAYS[14] });
export const paneSeparatorSkin = new Skin({ fill:GRAYS[14] });

export const tableHeaderSkin = new Skin({ fill:[GRAYS[6], GRAYS[10], GRAYS[14], GRAYS[6]], stroke:GRAYS[14], borders: { bottom:1 } });
export const tableHeaderStyle = new Style({ font:"bold", color:GRAYS[75], horizontal:"left" });
export const tableFooterSkin = new Skin({ fill:GRAYS[2], stroke:GRAYS[14], borders: { bottom:1 }  });
export const tableRowSkin = new Skin({ fill:[GRAYS[2], GRAYS[6], GRAYS[10], GRAYS[2]]  });
export const tableRowStyle = new Style({ color:[BLACK, BLACK, BLACK, WHITE], horizontal:"left" });

export const breakpointRowNameStyle = new Style({ color:[BLACK, BLACK, BLACK, WHITE] });
export const breakpointRowLineStyle = new Style({ font:"light", color:[BLACK, BLACK, BLACK, WHITE] });
export const fileRowSkin = new Skin({ fill:[GRAYS[2], GRAYS[6], GRAYS[10], FILE_SELECTION] });
export const fileRowStyle = new Style({ font:"semibold", color:WHITE, horizontal:"left" });
export const infoRowStyle = new Style({ color:GRAYS[50] , horizontal:"left" });
export const searchEmptyStyle = new Style({ color:GRAYS[50] });
export const resultRowSkin = new Skin({ fill:[WHITE, GRAYS[2], GRAYS[6], WHITE] });
export const resultLabelSkin = new Skin({ fill:[TRANSPARENT, TRANSPARENT, CODE_RESULT, CODE_RESULT] });
export const resultLabelStyle = new Style({ font:"Fira Mono", color:GRAYS[69], horizontal:"left" });
export const resultCountStyle = new Style({ color:GRAYS[50], horizontal:"right", right:5 });

export const callRowSkin = new Skin({ fill:[GRAYS[2], GRAYS[6], GRAYS[10], FRAME_SELECTION]  });
export const callRowStyle = new Style({ font:"semibold", color:WHITE, horizontal:"left" });
export const debugRowNameStyle = new Style({ color:[BLACK, INSPECTOR_FLASH, INSPECTOR_CACHE, WHITE] });
export const debugRowValueStyle = new Style({ font:"light", color:[BLACK, INSPECTOR_FLASH, INSPECTOR_CACHE, WHITE] });
export const instrumentBarColor = GRAYS[75];
export const instrumentLineColor = GRAYS[14];
export const instrumentBarHoverColor = FRAME_SELECTION;
export const instrumentBarHilightColor = WHITE;
export const instrumentRowNameStyle = new Style({ font:"10px", color:BLACK, horizontal:"left", vertical:"top" });
export const instrumentRowValueStyle = new Style({ font:"light 10px", color:BLACK, horizontal:"right", vertical:"top" });

export const noCodeSkin = new Skin({ fill:GRAYS[2] });

export const fieldScrollerSkin = new Skin({ fill: [ WHITE, WHITE ], stroke:GRAYS[6], borders: { left:1, right:1, bottom:1, top:1 } });

export const horizontalScrollbarSkin = new Skin({ 
	fill: [ TRANSPARENT, GRAYS[2], GRAYS[2] ],
	stroke: [ TRANSPARENT, GRAYS[14], GRAYS[14] ],
	borders: { top:1 },
});
export const verticalScrollbarSkin = new Skin({ 
	fill: [ TRANSPARENT, GRAYS[2], GRAYS[2] ],
	stroke: [ TRANSPARENT, GRAYS[14], GRAYS[14] ],
	borders: { left:1 },
});
export const scrollbarThumbSkin = new Skin({
	fill: [ "#e0e0e000", GRAYS[10], GRAYS[10], GRAYS[10] ],
});

export const findLabelStyle = new Style({ color:BLACK, horizontal:"left", left:5, right:5});
export const findModesSkin = new Skin({ texture: findModesTexture, x:0, y:0, width:20, height:20, variants:20, states:20 });

export const pathSpanStyle = new Style({ font:"semibold 12px Open Sans", color:[GRAYS[33],GRAYS[33],GRAYS[53],GRAYS[73]], horizontal:"left" });
export const pathNameStyle = new Style({ font:"semibold", color:BLACK, horizontal:"left" });

export const errorStyle = new Style({ color:GRAYS[85] });

export const waitSkin = new Skin({ texture:waitTexture, x:0, y:0, width:20, height:20, variants:20 });
export const waitStyle = new Style({ color:GRAYS[50] });

export const headerHeight = 26;
export const footerHeight = 3;
export const rowHeight = 18;
export const rowIndent = 18;

export const preferenceHeaderSkin = new Skin({ fill:[GRAYS[2], GRAYS[6], GRAYS[10], GRAYS[2]]  });
export const preferenceRowSkin = new Skin({ fill:[GRAYS[2], GRAYS[6], GRAYS[10], GRAYS[2]]  });

export const preferenceCommentStyle = new Style({ font:"light", color:GRAYS[85], horizontal:"left", left:10 });
export const preferenceFirstNameStyle = new Style({ font:"semibold", color:GRAYS[85], horizontal:"left" });
export const preferenceSecondNameStyle = new Style({ font:"semibold", color:GRAYS[85], horizontal:"left" });
export const preferenceThirdNameStyle = new Style({ color:GRAYS[85], horizontal:"left" });
export const preferenceValueStyle = new Style({ color:GRAYS[85], horizontal:"left", left:2 });

export const toggleBarSkin = new Skin({ texture:toggleTexture, x:0, y:0, width:60, height:30, states:30, tiles: { left:20, right:20 }  });
export const toggleButtonSkin = new Skin({ texture:toggleTexture, x:60, y:0, width:30, height:30, states:30 });
