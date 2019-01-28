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


#ifndef __COMMODETTO_PIXELSOUT_H__
#define __COMMODETTO_PIXELSOUT_H__

#include "commodettoBitmap.h"
#include "commodettoPocoBlit.h"

typedef void (*PixelsOutBegin)(void *refcon, CommodettoCoordinate x, CommodettoCoordinate y, CommodettoDimension w, CommodettoDimension h);
typedef void (*PixelsOutBeginFrameBuffer)(void *refcon, CommodettoPixel **pixels, int16_t *rowBytes);
typedef void (*PixelsOutContinue)(void *refcon);
typedef void (*PixelsOutEnd)(void *refcon);
typedef void (*PixelsOutAdaptInvalid)(void *refcon, CommodettoRectangle r);

typedef struct {
	PixelsOutBegin				doBegin;
	PixelsOutContinue			doContinue;
	PixelsOutEnd				doEnd;
	PocoRenderedPixelsReceiver	doSend;
	PixelsOutAdaptInvalid		doAdaptInvalid;
#if kPocoFrameBuffer
	PixelsOutBeginFrameBuffer	doBeginFrameBuffer;
#endif
} PixelsOutDispatchRecord, *PixelsOutDispatch;

#endif
