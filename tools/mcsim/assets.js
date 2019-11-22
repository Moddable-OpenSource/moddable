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
const GRAYS = [
	"#FFFFFF", "#FCFCFC", "#FAFAFA", "#F7F7F7", "#F5F5F5", "#F2F2F2", "#F0F0F0", "#EDEDED", "#EBEBEB", "#E8E8E8",
	"#E6E6E6", "#E3E3E3", "#E0E0E0", "#DEDEDE", "#DBDBDB", "#D9D9D9", "#D6D6D6", "#D4D4D4", "#D1D1D1", "#CFCFCF",
	"#CCCCCC", "#C9C9C9", "#C7C7C7", "#C4C4C4", "#C2C2C2", "#BFBFBF", "#BDBDBD", "#BABABA", "#B8B8B8", "#B5B5B5",
	"#B3B3B3", "#B0B0B0", "#ADADAD", "#ABABAB", "#A8A8A8", "#A6A6A6", "#A3A3A3", "#A1A1A1", "#9E9E9E", "#9C9C9C",
	"#999999", "#969696", "#949494", "#919191", "#8F8F8F", "#8C8C8C", "#8A8A8A", "#878787", "#858585", "#828282",
	"#808080", "#7D7D7D", "#7A7A7A", "#787878", "#757575", "#737373", "#707070", "#6E6E6E", "#6B6B6B", "#696969",
	"#666666", "#636363", "#616161", "#5E5E5E", "#5C5C5C", "#595959", "#575757", "#545454", "#525252", "#4F4F4F",
	"#4D4D4D", "#4A4A4A", "#474747", "#454545", "#424242", "#404040", "#3D3D3D", "#3B3B3B", "#383838", "#363636",
	"#333333", "#303030", "#2E2E2E", "#2B2B2B", "#292929", "#262626", "#242424", "#212121", "#1F1F1F", "#1C1C1C",
	"#191919", "#171717", "#141414", "#121212", "#0F0F0F", "#0D0D0D", "#0A0A0A", "#080808", "#050505", "#030303",
];
// piu/Scrollbars
export const dividerSkin = { fill:GRAYS[24] };

// piu/Dividers
export const horizontalScrollbarSkin = { 
	fill: [ TRANSPARENT, GRAYS[2], GRAYS[2] ],
	stroke: [ TRANSPARENT, GRAYS[14], GRAYS[14] ],
	borders: { top:1 },
};
export const verticalScrollbarSkin = { 
	fill: [ TRANSPARENT, GRAYS[2], GRAYS[2] ],
	stroke: [ TRANSPARENT, GRAYS[14], GRAYS[14] ],
	borders: { left:1 },
};
export const scrollbarThumbSkin = {
	fill: [ "#e0e0e000", GRAYS[10], GRAYS[10], GRAYS[10] ],
};

export const backgroundSkin = { fill:GRAYS[20] };
export const applicationStyle = { font:"12px Open Sans" };

export const controlsScrollerSkin = { fill:GRAYS[2] };
export const controlRowSkin = { fill:[GRAYS[2], GRAYS[6]] };
export const controlNameStyle = { color:GRAYS[85], horizontal:"left", left:10 };
export const controlValueStyle = { font:"light", color:[GRAYS[85],GRAYS[19]], horizontal:"left", left:10 };

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
export const sliderBarSkin = { texture:sliderTexture, x:0, y:0, width:60, height:30, left:15, right:15, variants:90, states:30 };
export const sliderButtonSkin = { texture:sliderTexture, x:60, y:0, width:30, height:30, variants:90, states:30 };

const switchTexture = { path:"assets/switch.png", scale:2 };
export const switchBarSkin = { texture:switchTexture, x:0, y:0, width:60, height:30, variants:90, states:30, tiles: { left:20, right:20 }  };
export const switchButtonSkin = { texture:switchTexture, x:60, y:0, width:30, height:30, variants:90, states:30 };

export const paneBackgroundSkin = { fill:GRAYS[6] };
export const paneBodySkin = { fill:GRAYS[2] };
export const paneHeaderSkin = { fill:[GRAYS[6], GRAYS[10], GRAYS[14], GRAYS[6]] };
export const paneHeaderStyle = { font:"semibold", color:[GRAYS[30], GRAYS[85], GRAYS[85], GRAYS[85]], horizontal:"left" };
export const paneBorderSkin = { fill:GRAYS[14] };
export const paneSeparatorSkin = { fill:GRAYS[14] };

export const timerSkin = { fill:[WHITE, BLUE], stroke:GRAYS[14], left:1, right:1, top:1, bottom:1 };

Object.freeze(dividerSkin, true)
Object.freeze(horizontalScrollbarSkin, true)
Object.freeze(verticalScrollbarSkin, true)
Object.freeze(scrollbarThumbSkin, true)
Object.freeze(backgroundSkin, true)
Object.freeze(applicationStyle, true)
Object.freeze(controlsScrollerSkin, true)
Object.freeze(controlRowSkin, true)
Object.freeze(controlNameStyle, true)
Object.freeze(controlValueStyle, true)
Object.freeze(buttonsSkin, true)
Object.freeze(controlsMenuSkin, true)
Object.freeze(controlsMenuGlyphSkin, true)
Object.freeze(controlsMenuItemSkin, true)
Object.freeze(controlsMenuItemStyle, true)
Object.freeze(glyphsSkin, true)
Object.freeze(buttonSkin, true)
Object.freeze(buttonStyle, true)
Object.freeze(popupStyle, true)
Object.freeze(dotSkin, true)
Object.freeze(sliderBarSkin, true)
Object.freeze(sliderButtonSkin, true)
Object.freeze(switchBarSkin, true)
Object.freeze(switchButtonSkin, true)
Object.freeze(paneBackgroundSkin, true)
Object.freeze(paneBodySkin, true)
Object.freeze(paneHeaderSkin, true)
Object.freeze(paneHeaderStyle, true)
Object.freeze(paneBorderSkin, true)
Object.freeze(paneSeparatorSkin, true)
Object.freeze(timerSkin, true)
