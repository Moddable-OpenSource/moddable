/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

const TRANSPARENT = "#002f8700";
const WHITE = "#ffffff";
const BLACK = "#000000";
const MODDABLE_BLUE = "#002f87";
const LIGHT_BLUE = "#4e6fb1";
const RED ="#E93F33";
const GREEN ="#8CC63F";
const BLUE = "#45AAE1";
const ORANGE = "#fab023";

const OpenSans28 = Style.template({ font: "open-sans-28-date+color", color: [BLACK, WHITE], top: -5 });
const OpenSans50 = Style.template({ font: "open-sans-50-one", color: [WHITE, TRANSPARENT] });

const WhiteSkin = Skin.template({ fill: [WHITE, TRANSPARENT] });
const ModdableBlueSkin = Skin.template({ fill: [MODDABLE_BLUE, WHITE] });
const ModdableBlueSkin2 = Skin.template({ fill: [MODDABLE_BLUE, TRANSPARENT] });
const LightBlueSkin = Skin.template({ fill: [LIGHT_BLUE, TRANSPARENT] });

const ModdableLogoTexture = Texture.template({ path:"moddable-text.png" });
const ModdableLogoSkin = Skin.template({ 
	Texture: ModdableLogoTexture, 
	color: [WHITE, TRANSPARENT], 
	x: 0, y: 0, width: 168, height: 30, 
});

const ModdableIconTexture = Texture.template({ path:"moddable-box.png" });
const ModdableIconSkin = Skin.template({ 
	Texture: ModdableIconTexture, 
	color: [WHITE, TRANSPARENT], 
	x: 0, y: 0, width: 70, height: 74, 
});

const ModdableIconCloseTexture = Texture.template({ path:"moddable-close-box.png" });
const ModdableIconCloseSkin = Skin.template({ 
	Texture: ModdableIconCloseTexture, 
	color: [WHITE, TRANSPARENT], 
	x: 0, y: 0, width: 2, height: 50, 
});

const DotTexture = Texture.template({ path:"dot.png" });
const RedDot = Skin.template({ 
	Texture: DotTexture, 
	color: [RED, TRANSPARENT], 
	x: 0, y: 0, width: 60, height: 60, 
});
const GreenDot = Skin.template({ 
	Texture: DotTexture, 
	color: [GREEN, TRANSPARENT], 
	x: 0, y: 0, width: 60, height: 60, 
});
const BlueDot = Skin.template({ 
	Texture: DotTexture, 
	color: [BLUE, TRANSPARENT], 
	x: 0, y: 0, width: 60, height: 60, 
});
const WhiteDot = Skin.template({ 
	Texture: DotTexture, 
	color: [WHITE, TRANSPARENT], 
	x: 0, y: 0, width: 60, height: 60, 
});

const FeatherTexture = Texture.template({ path:"feathers-color.bmp" });
const FeatherSkin = Skin.template({ 
	Texture: FeatherTexture, 
	x: 0, y: 0, width: 240, height: 320, 
});

const CrosshairTexture = Texture.template({ path:"crosshair.png" });
const CrosshairSkin = Skin.template({ 
	Texture: CrosshairTexture, 
	color: WHITE,
	x: 0, y: 0, width: 73, height: 73, 
});

const RectangleTexture = Texture.template({ path:"rectangle.png" });
const RectangleSkin = Skin.template({ 
	Texture: RectangleTexture, 
	x: 0, y: 0, width: 158, height: 68, 
});

const CircleTexture = Texture.template({ path:"circle.png" });
const CircleSkin = Skin.template({ 
	Texture: CircleTexture, 
	color: ORANGE,
	x: 0, y: 0, width: 72, height: 72, 
	states: 72
});

const ScrollerTexture = Texture.template({ path:"scroller.png" });
const ScrollerSkin = Skin.template({ 
	Texture: ScrollerTexture, 
	color: WHITE,
	x: 0, y: 0, width: 14, height: 64, 
});

export default Object.freeze({
	OpenSans28,
	OpenSans50,
	WhiteSkin,
	ModdableBlueSkin,
	ModdableBlueSkin2,
	LightBlueSkin,
	ModdableLogoSkin,
	ModdableIconSkin,
	ModdableIconCloseSkin,
	RedDot,
	GreenDot,
	BlueDot,
	WhiteDot,
	FeatherSkin,
	CrosshairSkin,
	RectangleSkin,
	CircleSkin,
	ScrollerSkin
})