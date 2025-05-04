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
*   You should have received a copy of the GNU Lesser Gen
eral Public License
*   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "xsmc.h"
#include "xsHost.h"

#include "applib/fonts/fonts.h"
#include "applib/graphics/gcontext.h"
#include "applib/graphics/graphics.h"
#include "applib/ui/app_window_stack.h"
#include "applib/rockyjs/api/rocky_api_graphics_color.h"
#include "applib/rockyjs/api/rocky_api_graphics_path2d.h"
#include "applib/rockyjs/api/rocky_api_graphics_text.h"

#include "util/trig.h"

struct Context2DStoredState {
	struct Context2DStoredState *next;
	GDrawState draw_state;
	RockyAPITextState text_state;
};
typedef struct Context2DStoredState Context2DStoredState;

struct PebbleGraphicsContextRecord {
	GContext			*ctx;
	Window 			*w;
	xsMachine 		*the;
	xsSlot			obj;
	xsSlot			*onUpdate;

	uint8_t			redrawPending;

	RockyAPIPathStep *path_steps;
	size_t path_steps_array_len;
	size_t path_steps_num;

	RockyAPITextState text_state;

	Context2DStoredState *storedStates;
//	GFont s_default_font;
};

typedef struct PebbleGraphicsContextRecord PebbleGraphicsContextRecord;
typedef struct PebbleGraphicsContextRecord *PebbleGraphicsContext;

extern Fixed_S16_3 prv_fixed_s3_from_double(double d);

#define adjustAngle(a) ((int32_t)(a * TRIG_MAX_ANGLE / (2 * M_PI)) + TRIG_MAX_ANGLE / 4)
