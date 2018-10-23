/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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

#include "commodettoFontEngine.h"
#include "commodettoPocoBlit.h"
#include "xsPlatform.h"
#if !XSTOOLS
	#include "mc.defines.h"
#endif

#ifndef MODDEF_CFE_KERN
	#define MODDEF_CFE_KERN (0)
#endif

struct CommodettoFontEngineRecord {
	CFEGlyphRecord		glyph;

	const void			*fontData;

	int					height;
	int					ascent;
	unsigned int			charCount;
	const uint8_t		*charTable;
	uint8_t				isCompressed;
	uint8_t				isContinuous;

#if MODDEF_CFE_KERN
	uint8_t				kernSorted;
	int					kernCount;
	const uint8_t		*kernTriples;
#endif
};

typedef struct CommodettoFontEngineRecord CommodettoFontEngineRecord;
typedef struct CommodettoFontEngineRecord BMFRecord;
typedef struct CommodettoFontEngineRecord *BMF;

static uint16_t bmfUnicodeToGlyphID(CommodettoFontEngine bmf, uint32_t c);

CommodettoFontEngine CFENew(void)
{
	BMF bmf = c_calloc(1, sizeof(BMFRecord));
	if (!bmf)
		return NULL;

	return bmf;
}

void CFEDispose(CommodettoFontEngine bmf)
{
	if (bmf)
		c_free(bmf);
}

void CFELockCache(CommodettoFontEngine bmf, uint8_t lock)
{
	// no lock
}

void CFESetFontData(CommodettoFontEngine bmf, const void *fontData, uint32_t fontDataSize)
{
	const unsigned char *bytes, *start;
	uint32_t size;
	uint8_t version;

	if (fontData == bmf->fontData)
		return;					// unchanged

	bmf->charCount = 0;			// zero in case of failure

	start = fontData;
	bytes = start;

	version = c_read8(bytes + 3);
	if ((0x42 != c_read8(bytes + 0)) || (0x4D != c_read8(bytes + 1)) || (0x46 != c_read8(bytes + 2)) || ((3 != version) && (4 != version)))
		return;

	bmf->isCompressed = 4 == version;

	bytes += 4;

	// skip block 1
	if (1 != c_read8(bytes))
		return;
	bytes += 1;

	bytes += 4 + c_read32(bytes);

	// get lineHeight from block 2
	if (2 != c_read8(bytes))
		return;
	bytes += 1;

	size = c_read32(bytes);
	bytes += 4;

	bmf->height = c_read16(bytes);
	bytes += 2;

	bmf->ascent = c_read16(bytes);
	bytes += 2;

	bytes += 2 + 2;		// scaleW and scaleH
	if (1 != c_read16(bytes))	// pages
		return;

	bytes += size - 8;

	// skip block 3
	if (3 != c_read8(bytes))
		return;
	bytes += 1;

	bytes += 4 + c_read32(bytes);

	// use block 4
	if (4 != c_read8(bytes))
		return;
	bytes += 1;

	bmf->charTable = bytes + 4;
	bmf->isContinuous = (0x40 & c_read8(19 + bmf->charTable)) ? 1 : 0;

	size = c_read32(bytes);
	if (size % 20)
		return;
	bmf->charCount = size / 20;

#if MODDEF_CFE_KERN
	// block 5 - kerning
	bytes += 4 + size;
	if (5 == c_read8(bytes)) {
		bytes += 1;
		bmf->kernCount = c_read32(bytes) / 10;
		bmf->kernTriples = bytes + 4;
		bmf->kernSorted = (0x80 & c_read8(19 + bmf->charTable)) ? 1 : 0;
	}
#endif

	// success
	bmf->fontData = fontData;
}

void CFESetFontSize(CommodettoFontEngine bmf, int32_t size)
{
	// no sizes
}

void CFEGetFontMetrics(CommodettoFontEngine bmf, int32_t *ascent, int32_t *descent, int32_t *leading)
{
	if (0 == bmf->charCount) {
		if (ascent) *ascent = 0;
		if (descent) *descent = 0;
		if (leading) *leading = 0;
		return;
	}

	if (ascent)
		*ascent = bmf->ascent;
	if (descent)
		*descent = bmf->height - bmf->ascent;
	if (leading)
		*leading = 0;
}

CFEGlyph CFEGetGlyphFromGlyphID(CommodettoFontEngine bmf, uint16_t glyphID, uint8_t needPixels)
{
	CFEGlyph glyph = &bmf->glyph;
	const uint8_t *chars = bmf->charTable;
	const uint8_t *cc = chars + (20 * glyphID);

	if (!needPixels) {
		c_memset(glyph, 0, sizeof(*glyph));
		glyph->advance = (uint8_t)c_read16(cc + 16);
		return glyph;
	}

	glyph->sx = c_read16(cc + 4);
	glyph->sy = c_read16(cc + 6);
	glyph->w = c_read16(cc + 8);
	glyph->h = c_read16(cc + 10);
	glyph->dx = (int8_t)c_read16(cc + 12);
	glyph->dy = (int8_t)c_read16(cc + 14);
	glyph->advance = (int8_t)c_read16(cc + 16);

	if (bmf->isCompressed) {
		glyph->format = kCommodettoBitmapGray16 | kCommodettoBitmapPacked;
		glyph->bits = (glyph->sx | (glyph->sy << 16)) + (char *)cc;
		glyph->sx = 0;
		glyph->sy = 0;
	}
	else {
		glyph->format = 0;		// invalid
		glyph->bits = NULL;
	}

	return glyph;
}

CFEGlyph CFEGetGlyphFromUnicode(CommodettoFontEngine bmf, uint32_t unicode, uint8_t needPixels)
{
	uint16_t glyphID = bmfUnicodeToGlyphID(bmf, unicode);
	if (kInvalidGlyphID == glyphID)
		return NULL;

	return CFEGetGlyphFromGlyphID(bmf, glyphID, needPixels);
}

int16_t CFEGetKerningOffset(CommodettoFontEngine bmf, uint32_t unicode1, uint32_t unicode2)
{
#if MODDEF_CFE_KERN
	const uint8_t *kernTriples = bmf->kernTriples;
	int count = bmf->kernCount;

	if (NULL == kernTriples)
		return 0;

	if (bmf->kernSorted) {
		int min = 0;
		int max = count;
		do {
			int mid = (min + max) >> 1;
			const uint8_t *cc = (10 * mid) + kernTriples;
			unsigned int code = c_read32(cc);
			if (code < unicode1)
				min = mid + 1;
			else if (unicode1 < code)
				max = mid - 1;
			else {
				int direction = (unicode2 < code) ? -10 : +10;

				do {
					code = c_read32(cc + 4);
					if (code == unicode2)
						return c_read16(cc + 8);

					cc += direction;
					if (direction < 0) {
						if (cc < kernTriples)
							break;
					}
					else {
						if (cc >= ((bmf->kernCount * 10) + (uint8_t *)kernTriples))
							break;
					}
				} while (unicode1 == c_read32(cc));

				 return 0;
			}
		} while (min <= max);

		return 0;
	}
	else {
		while (count--) {
			if ((unicode1 == c_read32(kernTriples)) && (unicode2 == c_read32(kernTriples + 4)))
				return c_read16(kernTriples + 8);
			kernTriples += 10;
		}
	}
#endif
	return 0;
}

int CFEShape(CommodettoFontEngine bmf, const char *utf8In, int32_t byteLengthIn, char *utf8Out, int32_t byteLengthOut)
{
	return -1;
}

void CFELayoutRun(CommodettoFontEngine bmf, const char *utf8, int32_t byteLength, CFERun run, int32_t runLength, int32_t width)
{
	const char *utf8End = utf8 + byteLength;
#if MODDEF_CFE_KERN
	unsigned int prevGlyphUnicode = 0;
#endif

	while (utf8 < utf8End) {
		CFEGlyph glyph;
		unsigned int unicode = PocoNextFromUTF8((uint8_t **)&utf8);
		uint16_t glyphID = bmfUnicodeToGlyphID(bmf, unicode);
		if (kInvalidGlyphID == glyphID)
			continue;

		glyph = CFEGetGlyphFromGlyphID(bmf, glyphID, 0);
		if (NULL == glyph)
			continue;

		run->glyphID = glyphID;
		run->advance = glyph->advance;

#if MODDEF_CFE_KERN
		if (0 != prevGlyphUnicode)
			run[-1].advance += CFEGetKerningOffset(bmf, unicode, prevGlyphUnicode);
		prevGlyphUnicode = unicode;
#endif
		run++;

		runLength -= 1;
		if (1 == runLength)
			break;		//@@ report run array overflow
	}

	run->glyphID = kInvalidGlyphID;
}

/*
	utility functions
*/

uint16_t bmfUnicodeToGlyphID(CommodettoFontEngine bmf, uint32_t c)
{
	const uint8_t *chars = bmf->charTable;
	int min, max;

	if (bmf->isContinuous) {
		// one run of continuously numbered characters
		uint32_t firstChar = c_read32(chars);
		if (c < firstChar)
			return kInvalidGlyphID;

		c -= firstChar;
		if (c >= bmf->charCount)
			return kInvalidGlyphID;

		return c;
	}

	// ascending order, with gaps. binary search.
	min = 0;
	max = bmf->charCount;
	do {
		int mid = (min + max) >> 1;
		const uint8_t *cc = (20 * mid) + chars;
		uint32_t code = c_read32(cc);
		if (code < c)
			min = mid + 1;
		else if (c < code)
			max = mid - 1;
		else
			return mid;
	} while (min <= max);

	return kInvalidGlyphID;
}
