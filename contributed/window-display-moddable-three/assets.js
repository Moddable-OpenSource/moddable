/*
 * Copyright (c) 2016-2021 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

const WHITE = "#ffffff";
const BLACK = "#000000";

const WhiteSkin = Skin.template({ fill: WHITE });
const BlackSkin = Skin.template({ fill: BLACK });

const ModdableLogoTexture = Texture.template({ path:"mod-logo-blk.png" });
const ModdableLogo = Skin.template({ 
	Texture: ModdableLogoTexture, 
	color: BLACK, 
	x: 0, y: 0, width: 250, height: 122, 
});

const ThreeTexture = Texture.template({ path:"three.png" });
const Three = Skin.template({ 
	Texture: ThreeTexture, 
	color: WHITE, 
	x: 0, y: 0, width: 68, height: 21, 
});

const EyeTexture = Texture.template({ path:"eye.png" });
const Eye = Skin.template({ 
	Texture: EyeTexture, 
	color: BLACK, 
	x: 0, y: 0, width: 250, height: 122, 
});

const WeatherRightTexture = Texture.template({ path:"weathe-right.png" });
const WeatherRight = Skin.template({ 
	Texture: WeatherRightTexture, 
	color: BLACK, 
	x: 0, y: 0, width: 110, height: 120, 
});

const WeatherLeftTexture = Texture.template({ path:"weather-left.png" });
const WeatherLeft = Skin.template({ 
	Texture: WeatherLeftTexture, 
	color: WHITE, 
	x: 0, y: 0, width: 111, height: 94, 
});

const DitherGradientLeftTexture = Texture.template({ path:"dither-grad-left.png" });
const DitherGradientLeft = Skin.template({ 
	Texture: DitherGradientLeftTexture, 
	color: WHITE, 
	x: 0, y: 0, width: 93, height: 122, 
});

const ShoePriceTexture = Texture.template({ path:"shoe-price.png" });
const ShoePrice = Skin.template({ 
	Texture: ShoePriceTexture, 
	color: BLACK, 
	x: 0, y: 0, width: 236, height: 115, 
});

const WorldTexture = Texture.template({ path:"world-spin-sprite-b-w.png" });
const World = Skin.template({ 
	Texture: WorldTexture, 
	color: WHITE, 
	x: 0, y: 0, width: 98, height: 98,
	states: 98, variants: 100
});

const WiFiLeftTexture = Texture.template({ path:"wifi-left.png" });
const WiFiLeft = Skin.template({ 
	Texture: WiFiLeftTexture, 
	color: BLACK, 
	x: 0, y: 0, width: 144, height: 120, 
});

const WiFiLeftBackgroundTexture = Texture.template({ path:"wifi-left-background.png" });
const WiFiLeftBackground = Skin.template({ 
	Texture: WiFiLeftBackgroundTexture, 
	color: WHITE, 
	x: 0, y: 0, width: 144, height: 120, 
});

const WiFiRightTexture = Texture.template({ path:"wifi-right.png" });
const WiFiRight = Skin.template({ 
	Texture: WiFiRightTexture, 
	color: BLACK, 
	x: 0, y: 0, width: 99, height: 107, 
});

const DitherGradientRightTexture = Texture.template({ path:"dither-grad-right.png" });
const DitherGradientRight = Skin.template({ 
	Texture: DitherGradientRightTexture, 
	color: BLACK, 
	x: 0, y: 0, width: 93, height: 121, 
});

export default Object.freeze({
	WhiteSkin,
	BlackSkin,
	DitherGradientLeft,
	DitherGradientRight,
	Eye,
	ModdableLogo,
	ShoePrice,
	Three,
	WeatherRight,
	WeatherLeft,
	WiFiLeft,
	WiFiLeftBackground,
	WiFiRight,
	World
})