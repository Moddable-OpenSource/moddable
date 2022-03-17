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

const liteColors = {
	"button": { "fill": [ "transparent", "#ffffff", "#ffffff", "#6573c7" ], "stroke": [ "#dbdbdb", "#b0b0b0", "#b0b0b0", "#6573c7" ], "color": [ "#dbdbdb", "#404040", "#262626", "#ffffff" ] },
	"divider": { "fill": "#c2c2c2" },
	"field": { "fill": "#ffffff", "stroke": "#dbdbdb", "color":"#000000" },
	"fieldSelection": { "fill": "#6573c7", "color":"#ffffff" },
	"iconButton": { "fill": [ "transparent", "transparent", "#00000020", "#6573c7" ], "color": [ "#dbdbdb", "#404040", "#262626", "#ffffff" ] },
	"popupMenu": { "fill":"#ffffff", "stroke": "#e0e0e0", },
	"popupMenuItem": { "fill":["transparent", "transparent", "#e6e6e6", "#6573c7"], "color":["#d1d1d1", "#262626", "#262626", "#ffffff"] },
	"progressBar": { "fill": [ "transparent", "#ffffff", "#6573c7", "#6573c7" ], "stroke": [ "#dbdbdb", "#b0b0b0", "#6573c7", "#6573c7" ] },
	"scrollbar": { "fill": [ "transparent", "#fafafa", "#fafafa", "#fafafa" ], "stroke": [ "transparent", "#dbdbdb", "#dbdbdb", "#dbdbdb" ] },
	"scrollbarThumb": { "fill": [ "#e0e0e000", "#e0e0e0", "#e6e6e6", "#e6e6e6" ] },
	"sliderBar": { "fill": [ "transparent", "#b0b0b0", "#6573c7", "#6573c7" ], "stroke": [ "#dbdbdb", "#b0b0b0", "#6573c7", "#6573c7" ] },
	"sliderButton": { "fill": [ "#fafafa", "#ffffff", "#ffffff", "#ffffff" ], "stroke": [ "#dbdbdb", "#b0b0b0", "#b0b0b0", "#b0b0b0" ] },
	"switchBar": { "fill": [ "transparent", "#b0b0b0", "#6573c7", "#6573c7" ], "stroke": [ "#dbdbdb", "#b0b0b0", "#6573c7", "#6573c7" ] },
	"switchButton": { "fill": [ "transparent", "#ffffff", "#ffffff", "#ffffff" ], "stroke": [ "#dbdbdb", "#ffffff", "#ffffff", "#ffffff" ] },

	"pane": { "color": "#262626", "fill": [ "#fafafa", "#fafafa", "#f0f0f0", "#e6e6e6" ] },
	"header": { "color": "#404040", "fill": [ "#f0f0f0", "#f0f0f0", "#e6e6e6", "#dbdbdb" ], "stroke": "#dbdbdb", },

	"background": { "fill": "#cccccc" },
};

const darkColors = {
	"button": { "fill": [ "transparent", "#5d5d5d", "#5d5d5d", "#6573c7" ], "stroke": [ "#707070", "#707070", "#707070", "#6573c7" ], "color": [ "#808080", "#ffffff", "#ffffff", "#ffffff" ] },
	"divider": { "fill": "#545454" },
	"field": { "fill": "#1e1e1e", "stroke": "#4e4e4e", "color":"#ffffff" },
	"fieldSelection": { "fill": "#6573c7", "color":"#ffffff" },
	"iconButton": { "fill": [ "transparent", "transparent", "#ffffff50", "#6573c7" ], "color": [ "#808080", "#ffffff", "#ffffff", "#ffffff" ] },
	"popupMenu": { "fill":"#353535", "stroke": "#707070", },
	"popupMenuItem": { "fill":["transparent", "transparent", "#707070", "#6573c7"], "color":["#303030", "#ffffff", "#ffffff", "#ffffff"] },
	"progressBar": { "fill": [ "transparent", "#e4e4e4", "#6573c7", "#6573c7" ], "stroke": [ "#707070", "#e4e4e4", "#6573c7", "#6573c7" ] },
	"scrollbar": { "fill": [ "transparent", "#3b3b3b", "#3b3b3b", "#3b3b3b" ], "stroke": [ "transparent", "#454545", "#454545", "#454545" ] },
	"scrollbarThumb": { "fill": [ "#a2a2a200", "#a2a2a2", "#a2a2a2", "#a2a2a2" ] },
	"sliderBar": { "fill": [ "transparent", "#5d5d5d", "#6573c7", "#6573c7" ], "stroke": [ "#707070", "#707070", "#6573c7", "#6573c7" ] },
	"sliderButton": { "fill": [ "#2a2a2a", "#e4e4e4", "#e4e4e4", "#e4e4e4" ], "stroke": [ "#707070", "#e4e4e4", "#e4e4e4", "#e4e4e4" ] },
	"switchBar": { "fill": [ "transparent", "#5d5d5d", "#6573c7", "#6573c7" ], "stroke": [ "#707070", "#707070", "#6573c7", "#6573c7" ] },
	"switchButton": { "fill": [ "transparent", "#e4e4e4", "#e4e4e4", "#e4e4e4" ], "stroke": [ "#707070", "#e4e4e4", "#e4e4e4", "#e4e4e4" ] },

	"pane": { "color": "#ffffff", "fill": [ "#272727", "#272727", "#373737", "#474747" ], },
	"header": { "color": "#f0f0f0", "fill": [ "#3b3b3b", "#3b3b3b", "#4b4b4b", "#5b5b5b" ], "stroke": "#4e4e4e", },

	"background": { "fill": "#5d5d5d" },
};

const themes = [
	liteColors,
	darkColors,
];
Object.freeze(themes, true)

const textures = {
	icons: { path:"assets/icons.png", scale:2 },
	popup: { path:"assets/popup.png", scale:2 },
	shadow: { path:"assets/shadow.png", scale:1 },
};
Object.freeze(textures, true)

function buildTheme($, codeFont) {
	const colors = globalThis.colors = {};
	const skins = globalThis.skins = {};
	const styles = globalThis.styles = {};

	skins.button = new Skin({ fill:$.button.fill, stroke:$.button.stroke });
	styles.button = new Style({ font:"semibold", color:$.button.color });
	styles.serialButton = new Style({ font:"semibold", color:$.button.color , left:15, right:15 });
	skins.iconButton = new Skin({ fill:$.iconButton.fill });
	styles.iconButton = new Style({ font:"semibold", color:$.iconButton.color });
	skins.icons = new Skin({ texture:textures.icons, color:$.iconButton.color, x:2, y:2, width:26, height:26, variants:30 });
	skins.divider = new Skin({ fill:$.divider.fill });
	skins.fieldScroller = new Skin({ fill: $.field.fill, stroke:$.field.stroke, borders: { left:1, right:1, bottom:1, top:1 } });
	skins.field = new Skin({ fill: [$.field.fill, $.fieldSelection.fill], });
	styles.field = new Style({ color:[$.field.color, $.fieldSelection.color], horizontal:"left", left:5, right:5 });
	skins.popupButton = new Skin({ fill:$.button.fill, stroke:$.button.stroke });
	styles.popupButton = new Style({ font:"semibold", color:$.button.color, horizontal:"left" }),
	skins.popupIcons = new Skin({ texture:textures.popup, color:$.popupMenuItem.color, x:0, y:0, width:20, height:30, variants:20 });
	skins.popupMenu = new Skin({ fill:$.popupMenu.fill, stroke:$.popupMenu.stroke, left:1, right:1, top:1, bottom:1 });
	skins.popupMenuItem = new Skin({ fill:$.popupMenuItem.fill });
	styles.popupMenuItem = new Style({ font: "semibold", color:$.popupMenuItem.color, horizontal:"left" });
	skins.popupMenuShadow = new Skin({ texture:textures.shadow, x:0, y:0, width:60, height:40, left:20, right:20, top:10, bottom:20 });
	skins.progressBar = new Skin({ fill:$.progressBar.fill, stroke:$.progressBar.stroke, left:1, right:1, top:1, bottom:1 });
	skins.scrollbarThumb = new Skin({ fill:$.scrollbarThumb.fill });
	skins.horizontalScrollbar = new Skin({ fill:$.scrollbar.fill, stroke:$.scrollbar.stroke, top:1 });
	skins.verticalScrollbar = new Skin({ fill:$.scrollbar.fill, stroke:$.scrollbar.stroke, left:1 });
	skins.sliderBar = new Skin({ fill:$.sliderBar.fill, stroke:$.sliderBar.stroke });
	skins.sliderButton = new Skin({ fill:$.sliderButton.fill, stroke:$.sliderButton.stroke });
	skins.switchBar = new Skin({ fill:$.switchBar.fill, stroke:$.switchBar.stroke });
	skins.switchButton = new Skin({ fill:$.switchButton.fill, stroke:$.switchButton.stroke });
	
	skins.paneBody = new Skin({ fill:$.pane.fill });
	skins.paneBorder = new Skin({ fill:$.header.stroke });
	skins.paneHeader = new Skin({ fill:$.header.fill });
	styles.paneHeader = new Style({ font:"semibold", color:$.header.color, horizontal:"left" });
	styles.paneFooterLeft = new Style({ font:"light", color:$.header.color, horizontal:"left", left:10 });
	styles.paneFooterRight = new Style({ font:"light", color:$.header.color, horizontal:"right", right:10 });

	styles.controlName = new Style({ color:$.pane.color, horizontal:"left", left:10 });
	styles.controlValue = new Style({ font:"light", color:$.pane.color, horizontal:"left", left:10 });
	
	skins.background = new Skin({ fill:$.background.fill });
}

export function buildAssets(which) { 
	const theme = themes[which];
	buildTheme(theme);
}

