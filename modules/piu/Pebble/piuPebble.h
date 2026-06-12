/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

 #include "piuAll.h"
#include "mc.defines.h"

#include "applib/graphics/bitblt.h"
#include "applib/graphics/gcontext.h"
#include "applib/graphics/gdraw_command_image.h"
#include "applib/graphics/gdraw_command_private.h"
#include "applib/graphics/gdraw_command_sequence.h"
#include "applib/graphics/gdraw_command_transforms.h"
#include "applib/graphics/gtypes.h"
#include "applib/graphics/graphics.h"
#include "applib/graphics/graphics_line.h"

#include "applib/ui/app_window_stack.h"
#include "applib/ui/layer.h"
#include "applib/ui/window.h"

#include "process_state/app_state/app_state.h"

#include "util/trig.h"

#include "commodettoPoco.h"
#include "commodettoFontEngine.h"
extern CommodettoFontEngine gCFE;

enum {
	piuTextureAlpha = 1 << 0,
	piuTextureColor = 1 << 1,
};

struct PiuTextureStruct {
	PiuHandlePart;
	PiuAssetPart;
	GBitmap* gbitmap;
	GBitmap bits;
	GBitmap mask;
	PiuDimension width;
	PiuDimension height;
};

struct PiuFontStruct {
	PiuHandlePart;
	xsMachine* the;
	PiuFont* next;
	PiuFlags flags;
	xsIdentifier family;
	PiuCoordinate size;
	PiuCoordinate weight;
	PiuDimension ascent;
	PiuDimension height;
	void *gfont;
	uint8_t *buffer;
	size_t bufferSize;
	PiuTexture* texture;
};

#define MODDEF_PIU_PEBBLE_CLIPS (8)
#define MODDEF_PIU_PEBBLE_ORIGINS (8)

struct PiuViewStruct {
	PiuHandlePart;
	xsMachine* the;
	PiuApplication* application;
	xsSlot* screen;
	Window* window;
	GContext* ctx;
	GColor color;
	PiuTick idleTicks;
	PiuInterval idle;
	uint8_t clipDepth;
	uint8_t clipError;
	uint8_t	originDepth;
	uint8_t originError;
	PiuRectangleRecord clips[MODDEF_PIU_PEBBLE_CLIPS];
	PiuPointRecord origins[MODDEF_PIU_PEBBLE_ORIGINS];
};

typedef void (*PiuViewDrawContentProc)(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);

extern void PiuViewDrawContent(PiuView* self, PiuViewDrawContentProc proc, void* it, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);
