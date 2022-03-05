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
const BLUE = "#192eab";
const ORANGE = "#f68100"

const liteColors = {
	background: GRAYS[20],
	
	divider: GRAYS[14],
	
	footer: {
		background: [GRAYS[6], GRAYS[10], GRAYS[14], GRAYS[6]],
		text: BLACK
	},
	header: {
		background: [GRAYS[6], GRAYS[10], GRAYS[14], GRAYS[6]],
		text: [GRAYS[30], GRAYS[85], GRAYS[85], GRAYS[85]],
	},
	pane: {
		background: GRAYS[2],
		border: GRAYS[14],
	},
	
	button: {
		background:[TRANSPARENT, WHITE, GRAYS[12], BLUE],
		border: [GRAYS[10], GRAYS[20], GRAYS[20], BLUE],
		text:[GRAYS[19], GRAYS[85], GRAYS[85], WHITE],
	},
	controlName: {
		text:GRAYS[85],
	},
	controlValue: {
		text:[GRAYS[85],GRAYS[19]],
	},
	
	icon: {
		background: [TRANSPARENT, TRANSPARENT, WHITE, BLUE],
		border: [GRAYS[10], GRAYS[20], GRAYS[20], BLUE],
		text: [GRAYS[19], GRAYS[85], GRAYS[85], WHITE],
	},
	popupMenu: {
		background:WHITE,
		border:GRAYS[12],
	},
	popupMenuItem: {
		background:[TRANSPARENT, TRANSPARENT, GRAYS[10], BLUE],
		text:[GRAYS[19], GRAYS[85], GRAYS[85], WHITE],
	},
	scrollbar: {
		background:[ TRANSPARENT, GRAYS[2], GRAYS[2] ],
		border:[ TRANSPARENT, GRAYS[14], GRAYS[14] ],
		thumb: [ "#e0e0e000", GRAYS[10], GRAYS[10], GRAYS[10] ],
	},
	slider: {
		background:[TRANSPARENT, GRAYS[41], BLUE, BLUE],
		border:[GRAYS[10], GRAYS[41], BLUE, BLUE],
		button: {
			background:[GRAYS[2], WHITE, WHITE, WHITE],
			border:[GRAYS[10], GRAYS[41], GRAYS[41], GRAYS[41]],
		}
	},
	switch: {
		background:[TRANSPARENT, GRAYS[41], BLUE, BLUE],
		border:[GRAYS[10], GRAYS[41], BLUE, BLUE],
		button: {
			background:[TRANSPARENT, WHITE, WHITE, WHITE],
			border:[GRAYS[10], WHITE, WHITE, WHITE],
		}
	},
	timer: {
		background:[WHITE, BLUE],
		border:GRAYS[14],
	},
};

const darkColors = {
	background: "#4D4D4D",
	
	divider: GRAYS[67],
	
	footer: {
		background: [GRAYS[84], GRAYS[80], GRAYS[76], GRAYS[84]],
		text: WHITE
	},
	header: {
		background: [GRAYS[84], GRAYS[80], GRAYS[76], GRAYS[84]],
		text: [GRAYS[70], GRAYS[15], GRAYS[15], GRAYS[15]],
	},
	pane: {
		background: GRAYS[80],
		border: GRAYS[67],
	},
	
	button: {
		background:[TRANSPARENT, "#5D5D5D", "#5D5D5D", ORANGE],
		border: ["#5D5D5D", "#707070", "#707070", ORANGE],
		text:["#5D5D5D", "#E4E4E4", "#E4E4E4", WHITE],
	},
	controlName: {
		text:GRAYS[15],
	},
	controlValue: {
		text:[GRAYS[15],GRAYS[81]],
	},
	
	icon: {
		background: [TRANSPARENT, TRANSPARENT, BLACK, ORANGE],
		border: [GRAYS[10], GRAYS[20], GRAYS[20], ORANGE],
		text: [GRAYS[81], GRAYS[15], GRAYS[15], WHITE],
	},
	popupMenu: {
		background:"#353535",
		border:"#707070",
	},
	popupMenuItem: {
		background:[TRANSPARENT, TRANSPARENT, "#707070", ORANGE],
		text:[GRAYS[81], WHITE, WHITE, WHITE],
	},
	scrollbar: {
		background:[ TRANSPARENT, GRAYS[98], GRAYS[98] ],
		border:[ TRANSPARENT, GRAYS[86], GRAYS[86] ],
		thumb: [ "#e0e0e000", GRAYS[90], GRAYS[90], GRAYS[90] ],
	},
	slider: {
		background:[TRANSPARENT, "#5D5D5D", ORANGE, ORANGE],
		border:["#5D5D5D", "#5D5D5D", ORANGE, ORANGE],
		button: {
			background:[GRAYS[80], "#E4E4E4", "#E4E4E4", "#E4E4E4"],
			border:["#5D5D5D", "#707070", "#707070", "#707070"],
		}
	},
	switch: {
		background:[TRANSPARENT, "#5D5D5D", ORANGE, ORANGE],
		border:["#5D5D5D", "#5D5D5D", ORANGE, ORANGE],
		button: {
			background:[TRANSPARENT, "#E4E4E4", "#E4E4E4", "#E4E4E4"],
			border:["#5D5D5D", "#E4E4E4", "#E4E4E4", "#E4E4E4"],
		}
	},
	timer: {
		background:[BLACK, ORANGE],
		border:"#707070",
	},
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

function buildSkins($) {
	globalThis.skins = {
		background: new Skin({ fill:$.background }),
		button: new Skin({ fill:$.button.background, stroke:$.button.border }),
		iconButton: new Skin({ fill:$.icon.background, stroke:$.icon.border }),
		icons: new Skin({ texture:textures.icons, color:$.icon.text, x:0, y:0, width:30, height:30, variants:30 }),
		divider: new Skin({ fill:$.divider }),
		
		paneBody: new Skin({ fill:$.pane.background }),
		paneBorder: new Skin({ fill:$.pane.border }),
		
		paneHeader: new Skin({ fill:$.header.background }),
		
		popupButton: new Skin({ fill:$.button.background, stroke:$.button.border }),
		popupIcons: new Skin({ texture:textures.popup, color:$.popupMenuItem.text, x:0, y:0, width:20, height:30, variants:20 }),
		popupMenu: new Skin({ fill:$.popupMenu.background, stroke:$.popupMenu.border, left:1, right:1, top:1, bottom:1 }),
		popupMenuItem: new Skin({ fill:$.popupMenuItem.background }),
		popupMenuShadow: new Skin({ texture:textures.shadow, x:0, y:0, width:60, height:40, left:20, right:20, top:10, bottom:20 }),
	
		scrollbarThumb: new Skin({ fill:$.scrollbar.thumb }),
		horizontalScrollbar: new Skin({ fill:$.scrollbar.background, stroke:$.scrollbar.border, top:1 }),
		verticalScrollbar: new Skin({ fill:$.scrollbar.background, stroke:$.scrollbar.border, left:1 }),

		sliderBar: new Skin({ fill:$.slider.background, stroke:$.slider.border }),
		sliderButton: new Skin({ fill:$.slider.button.background, stroke:$.slider.button.border }),
		
		switchBar: new Skin({ fill:$.switch.background, stroke:$.switch.border }),
		switchButton: new Skin({ fill:$.switch.button.background, stroke:$.switch.button.border }),
		
		timer: new Skin({ fill:$.timer.background, stroke:$.timer.border, left:1, right:1, top:1, bottom:1 }),
	};
}

function buildStyles($) {
	globalThis.styles = {
		button: new Style({ font:"semibold", color:$.button.text }),
		controlName: new Style({ color:$.controlName.text, horizontal:"left", left:10 }),
		controlValue: new Style({ font:"light", color:$.controlValue.text, horizontal:"left", left:10 }),
		paneFooterLeft: new Style({ font:"light", color:$.footer.text, horizontal:"left", left:10 }),
		paneFooterRight: new Style({ font:"light", color:$.footer.text, horizontal:"right", right:10 }),
		paneHeader: new Style({ font:"semibold", color:$.header.text, horizontal:"left" }),
		popupButton: new Style({ font:"semibold", color:$.button.text, horizontal:"left" }),
		popupMenuItem: new Style({ font: "semibold", color:$.popupMenuItem.text, horizontal:"left" }),
	};
}

export function buildAssets(which) { 
	const theme = themes[which];
	buildSkins(theme);
	buildStyles(theme);
}

