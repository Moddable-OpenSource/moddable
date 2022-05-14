/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"

#include "xsPlatform.h"
#include "mc.xs.h"

#include "piuPlatform.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	piuCenter = 0,
	piuLeft = 1,
	piuRight = 2,
	piuLeftRight = 3,
	piuWidth = 4,
	piuLeftRightWidth = 7,
	piuMiddle = 0,
	piuTop = 1,
	piuBottom = 2,
	piuTopBottom = 3,
	piuHeight = 4,
	piuTopBottomHeight = 7,
	piuTemporary = 64
};
typedef uint8_t PiuAlignment;

enum {
	piuAspectCenter = 0,
	piuAspectFill = 1,
	piuAspectFit = 2,
	piuAspectStretch = 3,
};
typedef uint8_t PiuAspect;

typedef uint8_t PiuBoolean;
typedef uint32_t PiuFlags;
typedef uint32_t PiuSize;
typedef double PiuState;
typedef int32_t PiuVariant;

#ifdef piuPC
	typedef double PiuTick;
	typedef double PiuInterval;
#else
	typedef uint32_t PiuTick;
	typedef int32_t PiuInterval;
#endif

typedef struct PiuCoordinatesStruct PiuCoordinatesRecord, *PiuCoordinates;
typedef struct PiuMarginsStruct PiuMarginsRecord, *PiuMargins;
typedef struct PiuPointStruct PiuPointRecord, *PiuPoint;
typedef struct PiuRectangleStruct PiuRectangleRecord, *PiuRectangle;

typedef struct PiuColorStruct PiuColorRecord, *PiuColor;

typedef struct PiuAssetStruct PiuAssetRecord, *PiuAsset;

typedef struct PiuTextureStruct PiuTextureRecord, *PiuTexture;
typedef struct PiuSkinStruct PiuSkinRecord, *PiuSkin;

typedef struct PiuFontStruct PiuFontRecord, *PiuFont;
typedef struct PiuFontListStruct PiuFontListRecord, *PiuFontList;
typedef struct PiuStyleStruct PiuStyleRecord, *PiuStyle;
typedef struct PiuStyleLinkStruct PiuStyleLinkRecord, *PiuStyleLink;

typedef struct PiuDispatchStruct PiuDispatchRecord, *PiuDispatch;

typedef struct PiuContentStruct PiuContentRecord, *PiuContent;
typedef struct PiuLabelStruct PiuLabelRecord, *PiuLabel;
typedef struct PiuPortStruct PiuPortRecord, *PiuPort;
typedef struct PiuTextStruct PiuTextRecord, *PiuText;

typedef struct PiuContainerStruct PiuContainerRecord, *PiuContainer;
typedef struct PiuColumnStruct PiuColumnRecord, *PiuColumn;
typedef struct PiuLayoutStruct PiuLayoutRecord, *PiuLayout;
typedef struct PiuRowStruct PiuRowRecord, *PiuRow;
typedef struct PiuScrollerStruct PiuScrollerRecord, *PiuScroller;

typedef struct PiuApplicationStruct PiuApplicationRecord, *PiuApplication;
typedef struct PiuViewStruct PiuViewRecord, *PiuView;

typedef struct PiuDeferLinkStruct PiuDeferLinkRecord, *PiuDeferLink;
typedef struct PiuIdleLinkStruct PiuIdleLinkRecord, *PiuIdleLink;
typedef struct PiuTouchLinkStruct PiuTouchLinkRecord, *PiuTouchLink;
typedef struct PiuTouchSampleStruct PiuTouchSampleRecord, *PiuTouchSample;

typedef struct PiuTransitionStruct PiuTransitionRecord, *PiuTransition;

#define PiuHandlePart \
	xsSlot* reference

typedef struct {
	PiuHandlePart;
} PiuHandleRecord, *PiuHandle;

// UTILITIES

struct PiuCoordinatesStruct {
	PiuAlignment horizontal;
	PiuAlignment vertical;
	PiuCoordinate left;
	PiuCoordinate width;
	PiuCoordinate top;
	PiuCoordinate right;
	PiuCoordinate height;
	PiuCoordinate bottom;
};

struct PiuMarginsStruct {
	PiuCoordinate left;
	PiuCoordinate top;
	PiuCoordinate right;
	PiuCoordinate bottom;
};

struct PiuPointStruct {
	PiuCoordinate x;
	PiuCoordinate y;
};

// PiuRectangle.c

struct PiuRectangleStruct {
	PiuCoordinate x;
	PiuCoordinate y;
	PiuDimension width;
	PiuDimension height;
};

extern void PiuRectangleApplyAspect(PiuRectangle r0, PiuRectangle r1, PiuRectangle r2, PiuAspect aspect);
extern void PiuRectangleApplyMargins(PiuRectangle r0, PiuRectangle r1, PiuMargins margins);
extern PiuBoolean PiuRectangleContains(PiuRectangle r0, PiuRectangle r1);
extern void PiuRectangleEmpty(PiuRectangle r);
extern void PiuRectangleInset(PiuRectangle r, PiuCoordinate dx, PiuCoordinate dy);
extern PiuBoolean PiuRectangleIntersect(PiuRectangle r0, PiuRectangle r1, PiuRectangle r2);
extern PiuBoolean PiuRectangleIntersects(PiuRectangle r0, PiuRectangle r1);
extern PiuBoolean PiuRectangleIsEmpty(PiuRectangle r);
extern PiuBoolean PiuRectangleIsEqual(PiuRectangle r1, PiuRectangle r2);
extern void PiuRectangleOffset(PiuRectangle r, PiuCoordinate dx, PiuCoordinate dy);
extern void PiuRectangleScaleToFill(PiuRectangle r0, PiuRectangle r1, PiuRectangle r2);
extern void PiuRectangleScaleToFit(PiuRectangle r0, PiuRectangle r1, PiuRectangle r2);
extern void PiuRectangleSet(PiuRectangle r, PiuCoordinate x, PiuCoordinate y, PiuDimension width, PiuDimension height);
extern void PiuRectangleUnion(PiuRectangle r0, PiuRectangle r1, PiuRectangle r2);

// PiuColor.c

struct PiuColorStruct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

extern void PiuColorsBlend(PiuColor colors, double state, PiuColor color);
extern void PiuColorDictionary(xsMachine *the, xsSlot* slot, PiuColor color);
extern void PiuColorsDictionary(xsMachine *the, xsSlot* slot, PiuColor colors);
extern void PiuColorsSerialize(xsMachine *the, PiuColor colors);

// APPEARANCE

#define PiuAssetPart \
	PiuAsset* next; \
	PiuFlags flags

struct PiuAssetStruct {
	PiuHandlePart;
	PiuAssetPart;
};

// PiuTexture.c

#ifdef piuGPU
extern void PiuTextureBind(PiuTexture* self, PiuApplication* application, PiuView* view);
extern void PiuTextureUnbind(PiuTexture* self, PiuApplication* application, PiuView* view);
#endif

// PiuSkin.c

enum {
	piuSkinColor = 0,
	piuSkinPattern = 1 << 0,
	piuRepeatX = 1 << 1,
	piuRepeatY = 1 << 2,
	piuSkinColorized = 1 << 4,
};

struct PiuSkinStruct {
	PiuHandlePart;
	PiuAssetPart;
	union {
		struct {
			PiuColorRecord fill[4];
			PiuColorRecord stroke[4];
			PiuMarginsRecord borders;
		} color;
		struct {
			PiuTexture* texture;
			PiuRectangleRecord bounds;
			PiuPointRecord delta;
			PiuMarginsRecord tiles;
			PiuColorRecord color[4];
		} pattern;
	} data;
};

extern void PiuSkinDraw(PiuSkin* self, PiuView* view, PiuRectangle bounds, PiuVariant variant, PiuState state, PiuAlignment horizontal, PiuAlignment vertical);
extern PiuDimension PiuSkinGetWidth(PiuSkin* self);
extern PiuDimension PiuSkinGetHeight(PiuSkin* self);
#ifdef piuGPU
extern void PiuSkinBind(PiuSkin* self, PiuApplication* application, PiuView* view);
extern void PiuSkinUnbind(PiuSkin* self, PiuApplication* application, PiuView* view);
#endif

// PiuFont.c

struct PiuFontListStruct {
	PiuHandlePart;
	PiuFont* first;
};

extern void PiuFontListNew(xsMachine* the);

extern PiuDimension PiuFontGetAscent(PiuFont* self);
extern PiuDimension PiuFontGetHeight(PiuFont* self);
extern PiuDimension PiuFontGetWidth(PiuFont* self, xsSlot* string, xsIntegerValue offset, xsIntegerValue length);
extern void PiuStyleLookupFont(PiuStyle* self);
#ifdef piuPC
extern double PiuFontGetWidthSubPixel(PiuFont* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length);
#endif
#ifdef piuGPU
extern void PiuFontBind(PiuFont* self, PiuApplication* application, PiuView* view);
extern void PiuFontUnbind(PiuFont* self, PiuApplication* application, PiuView* view);
#endif

// PiuStyle.c

struct PiuStyleLinkStruct {
	PiuHandlePart;
	PiuStyleLink* chain;
	PiuStyleLink* next;
	PiuStyleLink* first;
	PiuStyle* style;
	PiuStyle* computedStyle;
};

enum {
	piuStyleCondensedBit = 1 << 0,
	piuStyleItalicBit = 1 << 1,
	piuStyleBits = piuStyleCondensedBit | piuStyleItalicBit,
	piuStyleCondensed = 1 << 2,
	piuStyleItalic = 1 << 3,
	piuStyleFamily = 1 << 4,
	piuStyleAbsoluteSize = 1 << 5,
	piuStylePercentageSize = 1 << 6,
	piuStyleRelativeSize = 1 << 7,
	piuStyleAbsoluteWeight = 1 << 8,
	piuStyleRelativeWeight = 1 << 9,
	piuStyleHorizontal = 1 << 10,
	piuStyleVertical = 1 << 11,
	piuStyleIndentation = 1 << 12,
	piuStyleLeading = 1 << 13,
	piuStyleMarginLeft = 1 << 14,
	piuStyleMarginTop = 1 << 15,
	piuStyleMarginRight = 1 << 16,
	piuStyleMarginBottom = 1 << 17,
	piuStyleColor = 1 << 18,
	piuStyleDecoration = 1 << 19,
	piuStyleUnderline = 1 << 20,
  piuStyleDecorationBits = piuStyleDecoration | piuStyleUnderline,
};

struct PiuStyleStruct {
	PiuHandlePart;
	PiuAssetPart;
	xsMachine* the;
	xsSlot* archive;
	PiuFont* font;
	xsIdentifier family;
	xsIntegerValue size;
	xsIntegerValue weight;
	PiuAlignment horizontal;
	PiuAlignment vertical;
	PiuCoordinate indentation;
	PiuCoordinate leading;
	PiuMarginsRecord margins;
	PiuColorRecord color[4];
};


extern PiuStyle* PiuStyleLinkCompute(xsMachine *the, PiuStyleLink* chain, PiuApplication* application);
extern void PiuStyleLinkNew(xsMachine* the);
extern PiuStyleLink* PiuStyleLinkMatch(xsMachine *the, PiuStyleLink* list, PiuStyleLink* chain, PiuStyle* style);

extern void PiuStyleDraw(PiuStyle* self, xsSlot* string, PiuView* view, PiuRectangle bounds, PiuBoolean ellipsis, PiuState state);
extern PiuDimension PiuStyleGetWidth(PiuStyle* self, xsSlot* string);
extern PiuDimension PiuStyleGetHeight(PiuStyle* self, xsSlot* string);
extern void PiuStyleOverride(PiuStyle* self, PiuStyle* result);
#ifdef piuGPU
extern void PiuStyleBind(PiuStyle* self, PiuApplication* application, PiuView* view);
extern void PiuStyleUnbind(PiuStyle* self, PiuApplication* application, PiuView* view);
#endif

// BEHAVIOR

// PiuBehavior.c

extern void PiuBehaviorOnCreate(void* it);

extern void PiuBehaviorOnDefaultID(void* it, xsIdentifier id);
#define PiuBehaviorOnDisplaying(it) PiuBehaviorOnDefaultID(it, xsID_onDisplaying) 
#define PiuBehaviorOnFinished(it) PiuBehaviorOnDefaultID(it, xsID_onFinished) 
#define PiuBehaviorOnScrolled(it) PiuBehaviorOnDefaultID(it, xsID_onScrolled) 
#define PiuBehaviorOnTimeChanged(it) PiuBehaviorOnDefaultID(it, xsID_onTimeChanged) 
#define PiuBehaviorOnTransitionBeginning(it) PiuBehaviorOnDefaultID(it, xsID_onTransitionBeginning) 
#define PiuBehaviorOnTransitionEnded(it) PiuBehaviorOnDefaultID(it, xsID_onTransitionEnded) 

extern void PiuBehaviorOnDraw(void* it, PiuRectangle area);

extern void PiuBehaviorOnFitID(void* it, xsIdentifier id, PiuDimension dimension);
#define PiuBehaviorOnFitHorizontally(it, width) PiuBehaviorOnFitID(it, xsID_onFitHorizontally, width) 
#define PiuBehaviorOnFitVertically(it, height) PiuBehaviorOnFitID(it, xsID_onFitVertically, height) 

extern PiuDimension PiuBehaviorOnMeasureID(void* it, xsIdentifier id, PiuDimension dimension);
#define PiuBehaviorOnMeasureHorizontally(it, width) PiuBehaviorOnMeasureID(it, xsID_onMeasureHorizontally, width) 
#define PiuBehaviorOnMeasureVertically(it, height) PiuBehaviorOnMeasureID(it, xsID_onMeasureVertically, height) 

extern PiuBoolean PiuBehaviorOnMouseID(void* it, xsIdentifier id, PiuCoordinate x, PiuCoordinate y);

extern void PiuBehaviorOnTouchID(void* it, xsIdentifier id, xsIntegerValue index, PiuCoordinate x, PiuCoordinate y, double ticks, PiuTouchLink* link);
#define PiuBehaviorOnTouchBegan(it, index, x, y, ticks, link) PiuBehaviorOnTouchID(it, xsID_onTouchBegan, index, x, y, ticks, link) 
#define PiuBehaviorOnTouchCancelled(it, index, x, y, ticks, link) PiuBehaviorOnTouchID(it, xsID_onTouchCancelled, index, x, y, ticks, link) 
#define PiuBehaviorOnTouchEnded(it, index, x, y, ticks, link) PiuBehaviorOnTouchID(it, xsID_onTouchEnded, index, x, y, ticks, link) 
#define PiuBehaviorOnTouchMoved(it, index, x, y, ticks, link) PiuBehaviorOnTouchID(it, xsID_onTouchMoved, index, x, y, ticks, link) 

// DISPATCH

enum {
	piuBackgroundTouch = 1 << 0,
	piuHost = 1 << 1,
	piuMessaging = 1 << 2,
	piuVisible = 1 << 3,
	piuActive = 1 << 4,
	piuClip = 1 << 5,
	piuExclusiveTouch = 1 << 6,
	piuMultipleTouch = 1 << 7,
	piuDisplaying = 1 << 8,
	piuHasOwnStyle = 1 << 9,
	piuContainer = 1 << 10,
	piuIdling = 1 << 11,
	piuPlaced = 1 << 12,
	piuXChanged = 1 << 13,
	piuYChanged = 1 << 14,
	piuWidthChanged = 1 << 15,
	piuHeightChanged = 1 << 16,
	piuLoop = 1 << 17,
	piuFocusable = 1 << 18,
	piuView = 1 << 19,
	piuContentsPlaced = 1 << 20,
	piuContentsHorizontallyChanged = 1 << 21,
	piuContentsVerticallyChanged = 1 << 22,
	piuDisplayed = 1 << 23,

	piuPositionChanged = piuXChanged | piuYChanged,
	piuSizeChanged = piuWidthChanged | piuHeightChanged,
	piuHorizontallyChanged = piuXChanged | piuWidthChanged,
	piuVerticallyChanged = piuYChanged | piuHeightChanged,
	piuContentsChanged = piuContentsHorizontallyChanged | piuContentsVerticallyChanged,

	/* Row, Column */
	piuHorizontal = 1 << 24,
	piuVertical = 1 << 25,

	/* Label */
	piuLabelEllipsis = 1 << 24,

	/* Scroller */
	piuLooping = 1 << 24,
	piuXScrolled = 1 << 25,
	piuYScrolled = 1 << 26,
	piuTracking = 1 << 27,
	
	piuScrolled = piuXScrolled | piuYScrolled,

	/* Window */
	piuAdjusting = 1 << 24,
	piuMenusChanged = 1 << 25,
	
	piuOrderChanged = 1 << 28,
};


typedef void (*PiuContentBindProc)(void* it, PiuApplication* application, PiuView* view);
typedef void (*PiuContentCascadeProc)(void* it);
typedef void (*PiuContentDrawProc)(void* it, PiuView* view, PiuRectangle area);
typedef void (*PiuContentFitProc)(void* it);
typedef void* (*PiuContentHitProc)(void* it, PiuCoordinate x, PiuCoordinate y);
typedef void (*PiuContentIdleProc)(void* it, PiuInterval interval);
typedef void (*PiuContentInvalidateProc)(void* it, PiuRectangle area);
typedef void (*PiuContentMeasureProc)(void* it);
typedef void (*PiuContentPlaceProc)(void* it);
typedef void (*PiuContainerPlaceContentProc)(void* it, PiuContent* content);
typedef void (*PiuContentReflowProc)(void* it, PiuFlags flags);
typedef void (*PiuContentShowingProc)(void* it, PiuBoolean showIt);
typedef void (*PiuContentShownProc)(void* it, PiuBoolean showIt);
typedef void (*PiuContentSyncProc)(void* it);
typedef void (*PiuContentUnbindProc)(void* it, PiuApplication* application, PiuView* view);
typedef void (*PiuContentUpdateProc)(void* it, PiuView* view, PiuRectangle area);

struct PiuDispatchStruct {
	char* type;
	PiuContentBindProc bind;
	PiuContentCascadeProc cascade;
	PiuContentDrawProc draw;
	PiuContentFitProc fitHorizontally;
	PiuContentFitProc fitVertically;
	PiuContentHitProc hit;
	PiuContentIdleProc idle;
	PiuContentInvalidateProc invalidate;
	PiuContentMeasureProc measureHorizontally;
	PiuContentMeasureProc measureVertically;
	PiuContentPlaceProc place;
	PiuContainerPlaceContentProc placeContentHorizontally;
	PiuContainerPlaceContentProc placeContentVertically;
	PiuContentReflowProc reflow;
	PiuContentShowingProc showing;
	PiuContentShownProc shown;
	PiuContentSyncProc sync;
	PiuContentUnbindProc unbind;
	PiuContentUpdateProc update;
};

#define PiuIdlePart \
	PiuIdleLink* idleLink; \
	PiuDispatch dispatch; \
	double duration; \
	PiuInterval interval; \
	PiuTick ticks; \
	double time
	
#define PiuBehaviorPart \
	xsMachine* the; \
	PiuFlags flags; \
	xsSlot* behavior; \
	PiuContainer* container; \
	PiuState state; \
	PiuFlags touches; \
	PiuApplication* application

#define PiuContentPart \
	PiuContent* previous; \
	PiuContent* next; \
	PiuRectangleRecord bounds; \
	PiuCoordinatesRecord coordinates; \
	PiuSkin* skin; \
	PiuStyle* style; \
	PiuVariant variant; \
	xsSlot* name

#define PiuContainerPart \
	PiuContent* first; \
	PiuContent* last; \
	PiuTransition* transition; \
	PiuView* view

// CONTENT

// PiuContent.c

struct PiuContentStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
};

extern void PiuContentBind(void* it, PiuApplication* application, PiuView* view);
extern void PiuContentCascade(void* it);
extern void PiuContentDelete(void* it);
extern void PiuContentDictionary(xsMachine* the, void* it);
extern void PiuContentDraw(void* it, PiuView* view, PiuRectangle area);
extern void PiuContentFitHorizontally(void* it);
extern void PiuContentFitVertically(void* it);
extern void* PiuContentHit(void* it, PiuCoordinate x, PiuCoordinate y);
extern void PiuContentIdle(void* it, PiuInterval interval);
extern void PiuContentInvalidate(void* it, PiuRectangle area);
extern PiuBoolean PiuContentIsShown(void* it);
extern void PiuContentMark(xsMachine* the, void* it, xsMarkRoot markRoot);
extern void PiuContentMeasureHorizontally(void* it);
extern void PiuContentMeasureVertically(void* it);
extern void PiuContentPlace(void* it);
extern void PiuContentReflow(void* it, PiuFlags flags);
extern void PiuContentSetTime(PiuContent* self, double time);
extern void PiuContentShowing(void* it, PiuBoolean showIt);
extern void PiuContentShown(void* it, PiuBoolean showIt);
extern void PiuContentSizeBy(PiuContent* self, PiuCoordinate dx, PiuCoordinate dy);
extern void PiuContentSync(void* it);
extern void PiuContentToApplicationCoordinates(void* it, PiuCoordinate x0, PiuCoordinate y0, PiuCoordinate *x1, PiuCoordinate *y1);
extern void PiuContentUnbind(void* it, PiuApplication* application, PiuView* view);
extern void PiuContentUpdate(void* it, PiuView* view, PiuRectangle area);
extern void PiuContent_delegateAux(xsMachine *the, PiuContent* content, xsIdentifier id, xsIntegerValue c);

// PiuLabel.c

struct PiuLabelStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	xsSlot* string;
	PiuStyle* computedStyle;
};

// PiuPort.c

struct PiuPortStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	PiuView* view;
	PiuStyle* computedStyle;
};

// PiuText.c

typedef struct PiuTextBufferStruct PiuTextBufferRecord, *PiuTextBuffer;
typedef struct PiuTextLinkStruct PiuTextLinkRecord, *PiuTextLink;
typedef int16_t PiuTextOffset;

struct PiuTextBufferStruct {
	PiuHandlePart;
	size_t available;
	size_t current;
};

struct PiuTextLinkStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
};

struct PiuTextStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	PiuTextBuffer* lineBuffer;
	PiuTextBuffer* nodeBuffer;
	PiuTextLink* nodeLink;
	PiuTextOffset nodeOffset;
	PiuTextOffset textOffset;
	PiuDimension textWidth;
	PiuDimension textHeight;
};

extern void PiuTextBufferAppend(xsMachine *the, PiuTextBuffer* buffer, void* data, size_t size);
extern void PiuTextBufferClear(xsMachine* the, PiuTextBuffer* buffer);
extern void PiuTextBufferGrow(xsMachine *the, PiuTextBuffer* buffer, size_t size);
extern void PiuTextBufferNew(xsMachine* the, size_t available);

// CONTAINER

// PiuContainer.c

struct PiuContainerStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	PiuContainerPart;
};

extern void PiuContainerAdjustHorizontally(void* it);
extern void PiuContainerAdjustVertically(void* it);
extern void PiuContainerBind(void* it, PiuApplication* application, PiuView* view);
extern void PiuContainerCascade(void* it);
extern xsIntegerValue PiuContainerCount(PiuContainer* self);
extern void PiuContainerDictionary(xsMachine* the, void* it);
extern void PiuContainerFitHorizontally(void* it);
extern void PiuContainerFitVertically(void* it);
extern void* PiuContainerHit(void* it, PiuCoordinate x, PiuCoordinate y);
extern void PiuContainerInvalidate(void* it, PiuRectangle area);
extern void PiuContainerMark(xsMachine* the, void* it, xsMarkRoot markRoot);
extern void PiuContainerMeasureHorizontally(void* it);
extern void PiuContainerMeasureVertically(void* it);
extern void PiuContainerPlace(void* it);
extern void PiuContainerPlaceContentHorizontally(void* it, PiuContent* content);
extern void PiuContainerPlaceContentVertically(void* it, PiuContent* content);
extern void PiuContainerReflow(void* it, PiuFlags flags);
extern void PiuContainerShowing(void* it, PiuBoolean showIt);
extern void PiuContainerShown(void* it, PiuBoolean showIt);
extern void PiuContainerUnbind(void* it, PiuApplication* application, PiuView* view);
extern void PiuContainerUpdate(void* it, PiuView* view, PiuRectangle area);

// PiuColumn.c

struct PiuColumnStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	PiuContainerPart;
	int32_t portion;
	int32_t sum;
};

// PiuLayout.c

struct PiuLayoutStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	PiuContainerPart;
};

// PiuRow.c

struct PiuRowStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	PiuContainerPart;
	int32_t portion;
	int32_t sum;
};

// PiuScroller.c

struct PiuScrollerStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	PiuContainerPart;
	PiuPointRecord delta;
};

// APPLICATION

// PiuApplication.c

struct PiuDeferLinkStruct {
	PiuHandlePart;
	PiuContent* content;
	PiuDeferLink* deferLink;
	xsIdentifier id;
	xsIntegerValue argc;
	xsSlot argv[7];
};

struct PiuIdleLinkStruct {
	PiuHandlePart;
	PiuIdlePart;
};

struct PiuTouchSampleStruct {
	PiuCoordinate x;
	PiuCoordinate y;
	double ticks;
};

#define piuTouchSampleCount 64
struct PiuTouchLinkStruct {
	PiuHandlePart;
	PiuContent* content;
	uint32_t index;
	PiuTouchSampleRecord samples[piuTouchSampleCount];
};

struct PiuApplicationStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	PiuContainerPart;
	PiuDeferLink* deferChain;
	PiuDeferLink* deferLoop;
	PiuIdleLink* idleChain;
	PiuIdleLink* idleLoop;
	xsSlot DeferLink;
	xsSlot Skin;
	xsSlot Style;
	xsSlot Texture;
	PiuStyleLink* styleList;
	PiuContent* focus;
#ifdef piuPC
	int32_t cursorShape;
	PiuContent* hover;
	PiuPointRecord mouse;
#endif
	xsIntegerValue touchLinkCount;
	PiuTouchLink* touchLinks[1];
};

extern void PiuApplicationAdjust(PiuApplication* self);
extern void PiuApplicationCaptureTouch(PiuApplication* self, PiuContent* it, xsIntegerValue index, PiuCoordinate x,  PiuCoordinate y, double ticks);
extern void PiuApplicationDeferContents(xsMachine* the, PiuApplication* self);
extern void PiuApplicationIdleCheck(PiuApplication* self);
extern void PiuApplicationIdleContents(PiuApplication* self);
extern void PiuApplicationResize(PiuApplication* self);
extern void PiuApplicationSetFocus(PiuApplication* self, void* it);
extern void PiuApplicationStartContent(PiuApplication* self, void* it);
extern void PiuApplicationStopContent(PiuApplication* self, void* it);
extern void PiuApplicationTouchBegan(PiuApplication* self, xsIntegerValue index, PiuCoordinate x, PiuCoordinate y, xsNumberValue ticks);
extern void PiuApplicationTouchEnded(PiuApplication* self, xsIntegerValue index, PiuCoordinate x, PiuCoordinate y, xsNumberValue ticks);
extern void PiuApplicationTouchIdle(PiuApplication* self);
extern void PiuApplicationTouchMoved(PiuApplication* self, xsIntegerValue index, PiuCoordinate x, PiuCoordinate y, xsNumberValue ticks);
#ifdef piuPC
enum {
	piuMenuEnabled = 1,
	piuMenuChecked = 2,
	piuMenuTitled = 4,
};
extern xsIntegerValue PiuApplicationCanMenu(PiuApplication* self, xsIntegerValue id);
extern void PiuApplicationDoMenu(PiuApplication* self, xsIntegerValue id);
extern void PiuApplicationKeyDown(PiuApplication* self, xsStringValue string);
extern void PiuApplicationKeyUp(PiuApplication* self, xsStringValue string);
extern void PiuApplicationModifiersChanged(PiuApplication* self, PiuBoolean controlKey, PiuBoolean optionKey, PiuBoolean shiftKey);
extern void PiuApplicationMouseEntered(PiuApplication* self, PiuCoordinate x, PiuCoordinate y);
extern void PiuApplicationMouseExited(PiuApplication* self, PiuCoordinate x, PiuCoordinate y);
extern void PiuApplicationMouseMoved(PiuApplication* self, PiuCoordinate x, PiuCoordinate y);
extern void PiuApplicationMouseScrolled(PiuApplication* self, PiuCoordinate dx, PiuCoordinate dy);
#endif

// PiuView.c

extern void PiuViewAdjust(PiuView* self);
extern void PiuViewDictionary(xsMachine* the, void* it);
extern void PiuViewDrawString(PiuView* self, xsSlot* string, xsIntegerValue offset, xsIntegerValue length, PiuFont* font, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension sw);
extern void PiuViewDrawTexture(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh);
extern void PiuViewFillColor(PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h);
extern void PiuViewFillTexture(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh);
extern void PiuViewGetSize(PiuView* view, PiuDimension *width, PiuDimension *height);
extern void PiuViewIdleCheck(PiuView* view, PiuInterval idle);
extern void PiuViewInvalidate(PiuView* view, PiuRectangle area);
extern void PiuViewPopClip(PiuView* view);
extern void PiuViewPopColor(PiuView* view);
extern void PiuViewPopColorFilter(PiuView* view);
extern void PiuViewPopOrigin(PiuView* view);
extern void PiuViewPushClip(PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h);
extern void PiuViewPushColor(PiuView* view, PiuColor color);
extern void PiuViewPushColorFilter(PiuView* view, PiuColor color);
extern void PiuViewPushOrigin(PiuView* view, PiuCoordinate x, PiuCoordinate y);
extern void PiuViewReflow(PiuView* self);
extern void PiuViewReschedule(PiuView* self);
extern PiuTick PiuViewTicks(PiuView* view);
extern void PiuViewValidate(PiuView* view, PiuRectangle area);
#ifdef piuPC
extern void PiuViewChangeCursor(PiuView* self, int32_t shape);
extern void PiuViewDrawStringSubPixel(PiuView* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length, PiuFont* font, double x, double y, PiuDimension w, PiuDimension sw);
#endif

// TRANSITION

// PiuTransition.c

struct PiuTransitionStruct {
	PiuHandlePart;
	PiuIdlePart;
	xsMachine* the;
	PiuTransition* next;
	PiuApplication* application;
	PiuContainer* container;
};

extern void PiuTransitionRun(PiuTransition* self, PiuContainer* container);
extern void PiuTransitionComplete(PiuTransition* self, PiuContainer* container);

// RESOURCES

extern int mcCountResources(xsMachine* the);
extern const char* mcGetResourceName(xsMachine* the, int i);
extern const void *mcGetResource(xsMachine* the, const char* path, size_t* size);

extern const void *fxGetResource(xsMachine* the, void* archive, const char* path, size_t* size);

// XS

extern xsBooleanValue fxFindBoolean(xsMachine* the, xsSlot* slot, xsIdentifier id, xsBooleanValue* value);
extern xsBooleanValue fxFindInteger(xsMachine* the, xsSlot* slot, xsIdentifier id, xsIntegerValue* value);
extern xsBooleanValue fxFindNumber(xsMachine* the, xsSlot* slot, xsIdentifier id, xsNumberValue* value);
extern xsBooleanValue fxFindString(xsMachine* the, xsSlot* slot, xsIdentifier id, xsStringValue* value);
extern xsBooleanValue fxFindResult(xsMachine* the, xsSlot* slot, xsIdentifier id);

#define xsFindBoolean(_THIS,_ID,_RESULT) fxFindBoolean(the, &_THIS, _ID, _RESULT)
#define xsFindInteger(_THIS,_ID,_RESULT) fxFindInteger(the, &_THIS, _ID, _RESULT)
#define xsFindNumber(_THIS,_ID,_RESULT) fxFindNumber(the, &_THIS, _ID, _RESULT)
#define xsFindString(_THIS,_ID,_RESULT) fxFindString(the, &_THIS, _ID, _RESULT)
#define xsFindResult(_THIS,_ID) fxFindResult(the, &_THIS, _ID)

#define xsNewFunction0(_FUNCTION) \
	(xsOverflow(-XS_FRAME_COUNT-0), \
	fxPush(_FUNCTION), \
	fxNew(the), \
	fxRunCount(the, 0), \
	fxPop())
#define xsNewFunction1(_FUNCTION,_SLOT0) \
	(xsOverflow(-XS_FRAME_COUNT-1), \
	fxPush(_FUNCTION), \
	fxNew(the), \
	fxPush(_SLOT0), \
	fxRunCount(the, 1), \
	fxPop())
#define xsNewFunction2(_FUNCTION,_SLOT0,_SLOT1) \
	(xsOverflow(-XS_FRAME_COUNT-2), \
	fxPush(_FUNCTION), \
	fxNew(the), \
	fxPush(_SLOT0), \
	fxPush(_SLOT1), \
	fxRunCount(the, 2), \
	fxPop())

#define PiuMarkHandle(THE, HANDLE) if (HANDLE) (*markRoot)(THE, (*((PiuHandle*)(HANDLE)))->reference)
#define PiuMarkReference(THE, REFERENCE) if (REFERENCE) (*markRoot)(THE, REFERENCE)
	
#define PIU(CAST, SLOT) ((Piu##CAST*)xsGetHostHandle(SLOT))

extern xsSlot* fxDuplicateString(xsMachine* the, xsSlot* slot);

#define PiuMarkString(THE, STRING) if (STRING) (fxClosure(THE, &the->scratch, STRING), (*markRoot)(THE, &the->scratch))
#define PiuString(SLOT) (the->scratch = (SLOT), fxDuplicateString(the, &(the->scratch)))
#define PiuToString(SLOT) (fxToString(the, SLOT))

#ifdef __cplusplus
}
#endif
