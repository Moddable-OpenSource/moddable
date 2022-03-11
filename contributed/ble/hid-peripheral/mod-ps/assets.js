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

const BACKGROUND = "#505050";
const FOREGROUND = "#cccccc";

const ArtHistoryTexture = Texture.template({path: "art-history-tool.png"});
const ArtHistorySkin = Skin.template({
     Texture: ArtHistoryTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const ArtHistoryData = {buttonSkin: ArtHistorySkin, x: 1, y: 2, shortcut: "Y"};

const SwapTexture = Texture.template({path: "swap-color-tool.png"});
const SwapSkin = Skin.template({
     Texture: SwapTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const SwapData = {buttonSkin: SwapSkin, x: 0, y: 3, shortcut: "X"};

const CropTexture = Texture.template({path: "crop-tool.png"});
const CropSkin = Skin.template({
     Texture: CropTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const CropData = {buttonSkin: CropSkin, x: 0, y: 1, shortcut: "C"};

const EyedropperTexture = Texture.template({path: "eyedropper-tool.png"});
const EyedropperSkin = Skin.template({
     Texture: EyedropperTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const EyedropperData = {buttonSkin: EyedropperSkin, x: 1, y: 1, shortcut: "I"};

const HandTexture = Texture.template({path: "hand-tool.png"});
const HandSkin = Skin.template({
     Texture: HandTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const HandData = {buttonSkin: HandSkin, x: 2, y: 4, shortcut: "H"};

const HorizontalTypeTexture = Texture.template({path: "horizontal-type-tool.png"});
const HorizontalTypeSkin = Skin.template({
     Texture: HorizontalTypeTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const HorizontalTypeData = {buttonSkin: HorizontalTypeSkin, x: 2, y: 3, shortcut: "T"};

const LassoTexture = Texture.template({path: "lasso-tool.png"});
const LassoSkin = Skin.template({
     Texture: LassoTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const LassoData = {buttonSkin: LassoSkin, x: 2, y: 0, shortcut: "L"};

const MagicEraserTexture = Texture.template({path: "magic-eraser-tool.png"});
const MagicEraserSkin = Skin.template({
     Texture: MagicEraserTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const MagicEraserData = {buttonSkin: MagicEraserSkin, x: 2, y: 2, shortcut: "E"};

const MagicWandTexture = Texture.template({path: "magic-wand-tool.png"});
const MagicWandSkin = Skin.template({
     Texture: MagicWandTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const MagicWandData = {buttonSkin: MagicWandSkin, x: 3, y: 0, shortcut: "W"};

const MoveTexture = Texture.template({path: "move-tool.png"});
const MoveSkin = Skin.template({
     Texture: MoveTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const MoveData = {buttonSkin: MoveSkin, x: 0, y: 0, shortcut: "V"};

const PaintBucketTexture = Texture.template({path: "paint-bucket-tool.png"});
const PaintBucketSkin = Skin.template({
     Texture: PaintBucketTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const PaintBucketData = {buttonSkin: PaintBucketSkin, x: 3, y: 2, shortcut: "G"};

const PathSelectionTexture = Texture.template({path: "path-selection-tool.png"});
const PathSelectionSkin = Skin.template({
     Texture: PathSelectionTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const PathSelectionData = {buttonSkin: PathSelectionSkin, x: 3, y: 3, shortcut: "A"};

const PencilTexture = Texture.template({path: "pencil-tool.png"});
const PencilSkin = Skin.template({
     Texture: PencilTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const PencilData = {buttonSkin: PencilSkin, x: 3, y: 1, shortcut: "B"};

const PenTexture = Texture.template({path: "pen-tool.png"});
const PenSkin = Skin.template({
     Texture: PenTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const PenData = {buttonSkin: PenSkin, x: 1, y: 3, shortcut: "P"};

const RectangleTexture = Texture.template({path: "rectangle-tool.png"});
const RectangleSkin = Skin.template({
     Texture: RectangleTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const RectangleData = {buttonSkin: RectangleSkin, x: 0, y: 4, shortcut: "U"};

const RectangularMarqueeTexture = Texture.template({path: "rectangular-marquee-tool.png"});
const RectangularMarqueeSkin = Skin.template({
     Texture: RectangularMarqueeTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const RectangularMarqueeData = {buttonSkin: RectangularMarqueeSkin, x: 1, y: 0, shortcut: "M"};

const ScreenTexture = Texture.template({path: "screen-tool.png"});
const ScreenSkin = Skin.template({
     Texture: ScreenTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const ScreenData = {buttonSkin: ScreenSkin, x: 3, y: 4, shortcut: "F"};

const SpotHealingBrushTexture = Texture.template({path: "spot-healing-brush.png"});
const SpotHealingBrushSkin = Skin.template({
     Texture: SpotHealingBrushTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const SpotHealingBrushData = {buttonSkin: SpotHealingBrushSkin, x: 2, y: 1, shortcut: "J"};

const StampTexture = Texture.template({path: "stamp-tool.png"});
const StampSkin = Skin.template({
     Texture: StampTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const StampData = {buttonSkin: StampSkin, x: 0, y: 2, shortcut: "S"};

const ZoomTexture = Texture.template({path: "zoom-tool.png"});
const ZoomSkin = Skin.template({
     Texture: ZoomTexture,
     width: 56, height: 54,
     color: [FOREGROUND, BACKGROUND]
});
const ZoomData = {buttonSkin: ZoomSkin, x: 1, y: 4, shortcut: "Z"};

const data = [ArtHistoryData, SwapData, CropData, EyedropperData, HandData, HorizontalTypeData, LassoData, MagicEraserData, MagicWandData, MoveData, PaintBucketData, PathSelectionData, PencilData, PenData, RectangleData, RectangularMarqueeData, ScreenData, SpotHealingBrushData, StampData, ZoomData];

export default data;
