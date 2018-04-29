/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
 */

const BLACK = "black";
const BLUE = "#192eab";
const TRANSPARENT = "transparent";
const WHITE = "white";
var GRAYS = new Array(100).fill();
for (let i = 0; i < 100; i++) {
	GRAYS[i] = blendColors(i / 100, WHITE, BLACK);
}

// piu/Scrollbars
export const dividerSkin = { fill:GRAYS[24] };

// piu/Dividers
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

export const backgroundSkin = { fill:GRAYS[20] };
export const applicationStyle = { font:"12px Open Sans" };

export const controlsScrollerSkin = { fill:GRAYS[2] };
export const controlRowSkin = { fill:[GRAYS[2], GRAYS[6]] };
export const controlNameStyle = { color:GRAYS[85], horizontal:"left", left:10 };
export const controlValueStyle = { font:"light", color:GRAYS[85], horizontal:"left", left:10 };

const dialogTexture = { path:"assets/dialog.png", scale:1 };
const menuTexture = { path:"assets/menu.png", scale:2 };
const buttonsTexture = { path:"assets/buttons.png", scale:2 };
export const buttonsSkin = { texture:buttonsTexture, x:2, y:2, width:26, height:26, variants:30, states:30 };

export const controlsMenuSkin = { texture:dialogTexture, x:0, y:0, width:60, height:40, left:20, right:20, top:10, bottom:20 };
export const controlsMenuGlyphSkin = { texture:menuTexture, x:0, y:0, width:20, height:20, variants:20, states:20 };
export const controlsMenuItemSkin = { fill:[TRANSPARENT, TRANSPARENT, GRAYS[10], BLUE] };
export const controlsMenuItemStyle = { font: "semibold", color:[GRAYS[19], GRAYS[85], GRAYS[85], WHITE], horizontal:"left" }

const glyphsTexture = { path:"assets/glyphs.png", scale:2 };
export const glyphsSkin = { texture:glyphsTexture, x:0, y:0, width:20, height:20, variants:20, states:20 };

const buttonTexture = { path:"assets/button.png", scale:2 };
export const buttonSkin = { texture:buttonTexture, x:0, y:0, width:60, height:30, states:30, tiles:{ left:15, right:40 }, variants:60 };
export const buttonStyle = { font:"semibold", color:[GRAYS[19], GRAYS[85], GRAYS[85], WHITE] };
export const popupStyle = { font:"semibold", color:[GRAYS[19], GRAYS[85], GRAYS[85], WHITE], horizontal:"left",  };
const dotTexture = { path:"assets/dot.png", scale:2 };
export const dotSkin = { texture:dotTexture, x:0, y:0, width:20, height:30 };

const sliderTexture = { path:"assets/slider.png", scale:2};
export const sliderBarSkin = { texture:sliderTexture, x:0, y:0, width:60, height:30, left:15, right:15, states:30 };
export const sliderButtonSkin = { texture:sliderTexture, x:60, y:0, width:30, height:30, states:30 };

const switchTexture = { path:"assets/switch.png", scale:2 };
export const switchBarSkin = { texture:switchTexture, x:0, y:0, width:60, height:30, states:30, tiles: { left:20, right:20 }  };
export const switchButtonSkin = { texture:switchTexture, x:60, y:0, width:30, height:30, states:30 };

export const paneBackgroundSkin = { fill:GRAYS[6] };
export const paneBodySkin = { fill:GRAYS[2] };
export const paneHeaderSkin = { fill:[GRAYS[6], GRAYS[10], GRAYS[14], GRAYS[6]] };
export const paneHeaderStyle = { font:"semibold", color:[GRAYS[30], GRAYS[85], GRAYS[85], GRAYS[85]], horizontal:"left" };
export const paneBorderSkin = { fill:GRAYS[14] };
export const paneSeparatorSkin = { fill:GRAYS[14] };

export const timerSkin = { fill:[WHITE, BLUE], stroke:GRAYS[14], left:1, right:1, top:1, bottom:1 };
