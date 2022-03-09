/*
 * Copyright (c) 2022 Moddable Tech, Inc.
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

const WHITE = "white";
const ORANGE = "#feb511";

const OpenSans20White = Style.template({ font: "20px Open Sans", color: WHITE });

const BkgTexture = Texture.template(({ path: "bkg.png"}));
const BkgSkin = Skin.template({
	Texture: BkgTexture,
	height: 320, width: 240
});

const BTTexture = Texture.template({path: "bt-logo.png"});
const BTSkin = Skin.template({
	Texture: BTTexture,
	width: 18, height: 24,
	color: WHITE
});

const VolumeDownTexture = Texture.template({ path: "vol-down-btns.png" });
const VolumeDownSkin = Skin.template({
	Texture: VolumeDownTexture,
	height: 62, width: 137, states: 62
});

const VolumeUpTexture = Texture.template({ path: "vol-up-btns.png" });
const VolumeUpSkin = Skin.template({
	Texture: VolumeUpTexture,
	height: 62, width: 137, states: 62
});

const PlayPauseTexture = Texture.template({ path: "center-btns.png" });
const PlayPauseSkin = Skin.template({
	Texture: PlayPauseTexture,
	height: 88, width: 88, states: 88
});

const ForwardTexture = Texture.template({ path: "next-track-btns.png" });
const ForwardSkin = Skin.template({
	Texture: ForwardTexture,
	width: 62, height: 138, states: 138
});

const BackTexture = Texture.template({ path: "prev-track-btns.png" });
const BackSkin = Skin.template({
	Texture: BackTexture,
	width: 62, height: 138, states: 138
});

const CurvedLine = Texture.template({ path: "arc-mask-shape.png" })
const Shadow = Texture.template({ path: "arc-inner-shadow.png" })
const Ticks = Texture.template({ path: "vert-ticks.png" })

export {WHITE, ORANGE, OpenSans20White, BkgSkin, BTSkin, VolumeDownSkin, VolumeUpSkin, PlayPauseSkin, ForwardSkin, BackSkin, CurvedLine, Shadow, Ticks};