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

/*
	Macintosh NFNT font engine.

	- font data is NFNT resource
*/

#include "commodettoFontEngine.h"
#include "commodettoPocoBlit.h"
#include "xsPlatform.h"

#if 0 != kPocoRotation
	#error unsupported rotation
#endif

struct CommodettoFontEngineRecord {
	uint8_t			*widthOffsetTable;
	uint16_t		*bitmapLocationTable;

	CFEGlyphRecord	glyph;

	uint16_t		firstChar;
	uint16_t		lastChar;
	int16_t			maxKern;
	int16_t			descent;
	int16_t			height;
	int16_t			leading;

	const void		*fontData;
};

typedef struct CommodettoFontEngineRecord CommodettoFontEngineRecord;
typedef struct CommodettoFontEngineRecord TinyFontRecord;
typedef struct CommodettoFontEngineRecord *TinyFont;

static uint16_t unicodeToMacRoman(uint32_t unicode);

CommodettoFontEngine CFENew(void)
{
	TinyFont tf = c_calloc(1, sizeof(TinyFontRecord));
	if (!tf)
		return NULL;

	return tf;
}

void CFEDispose(CommodettoFontEngine tf)
{
	if (tf)
		c_free(tf);
}

void CFELockCache(CommodettoFontEngine tf, uint8_t lock) {}

void CFESetFontData(CommodettoFontEngine tf, const void *fontData, uint32_t fontDataSize)
{
	const uint8_t *font = fontData;
	uint16_t rowWords;

	if (fontData == tf->fontData)
		return;

	font += 2;		// fontType
	tf->firstChar = c_read16be(font); font += 2;
	tf->lastChar = c_read16be(font); font += 2;
	font += 2;		// maxWidth
	tf->maxKern = c_read16be(font); font += 2;
	tf->descent = -c_read16be(font); font += 2;
	font += 2;		// width
	tf->height = c_read16be(font); font += 2;
	tf->widthOffsetTable = ((c_read16be(font) * 2) + 16) + (uint8_t *)fontData; font += 2;
	font += 2;		// maxAscent
	font += 2;		// maxDescent
	tf->leading = c_read16be(font); font += 2;
	rowWords = c_read16be(font); font += 2;		// rowWords
	font += 2 * rowWords * tf->height;		// skip bits
	tf->bitmapLocationTable = (uint16_t *)font;

	tf->fontData = fontData;
}

void CFESetFontSize(CommodettoFontEngine tf, int32_t size) {}

void CFEGetFontMetrics(CommodettoFontEngine tf, int32_t *ascent, int32_t *descent, int32_t *leading)
{
	if (ascent)
		*ascent = tf->height - tf->descent;
	if (descent)
		*descent = tf->descent;
	if (leading)
		*leading = tf->leading;
}

CFEGlyph CFEGetGlyphFromGlyphID(CommodettoFontEngine tf, uint16_t glyphID, uint8_t needPixels)
{
	CFEGlyph glyph = &tf->glyph;

	if ((glyphID < tf->firstChar) || (glyphID > tf->lastChar))
		return NULL;

	glyphID -= tf->firstChar;

	glyph->dx = c_read8(tf->widthOffsetTable + (glyphID << 1) + 0);
	if (glyph->dx & 0x80)
		glyph->dx = -128 + (glyph->dx & 0x7f);

	glyph->advance = c_read8(tf->widthOffsetTable + (glyphID << 1) + 1);
	if (glyph->advance & 0x80)
		glyph->advance = -128 + (glyph->advance & 0x7f);

	glyph->sx = c_read16be(tf->bitmapLocationTable + glyphID);
	glyph->w = c_read16be(tf->bitmapLocationTable + glyphID + 1) - glyph->sx;
	glyph->h = tf->height;

	return glyph;
}

CFEGlyph CFEGetGlyphFromUnicode(CommodettoFontEngine tf, uint32_t unicode, uint8_t needPixels)
{
	uint16_t glyphID = unicodeToMacRoman(unicode);
	if (kInvalidGlyphID == glyphID)
		return NULL;

	return CFEGetGlyphFromGlyphID(tf, glyphID, needPixels);
}

int16_t CFEGetKerningOffset(CommodettoFontEngine tf, uint32_t unicode1, uint32_t unicode2)
{
	return 0;
}

int CFEShape(CommodettoFontEngine tf, const char *utf8In, int32_t byteLengthIn, char *utf8Out, int32_t byteLengthOut)
{
	return -1;
}

void CFELayoutRun(CommodettoFontEngine tf, const char *utf8, int32_t byteLength, CFERun run, int32_t runLength, int32_t width)
{
	const char *utf8End = utf8 + byteLength;

	while (utf8 < utf8End) {
		CFEGlyph glyph;
		uint16_t glyphID = unicodeToMacRoman(PocoNextFromUTF8((uint8_t **)&utf8));
		if (kInvalidGlyphID == glyphID)
			continue;

		glyph = CFEGetGlyphFromGlyphID(tf, glyphID, false);
		if (NULL == glyph)
			continue;

		run->glyphID = glyphID;
		run->advance = glyph->advance;

		runLength -= 1;
		if (1 == runLength)
			break;		//@@ report run array overflow
	}

	run->glyphID = kInvalidGlyphID;
}

void CFEGetOutlineFromUnicode(CommodettoFontEngine cfe, uint32_t unicode, uint8_t **outline, uint32_t *outlineSize)
{
	*outline = NULL;
	*outlineSize = 0;
}

void CFERenderOutline(CommodettoFontEngine cfe, uint8_t *outline, uint32_t outlineSize, double scaleX, double scaleY, CFEGlyph glyph) {}

static const uint16_t gMacRomanToUnicode[] ICACHE_RODATA_ATTR  = {
	0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1,
	0x00E0, 0x00E2, 0x00E4, 0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8,
	0x00EA, 0x00EB, 0x00ED, 0x00EC, 0x00EE, 0x00EF, 0x00F1, 0x00F3,
	0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC,
	0x2020, 0x00B0, 0x00A2, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF,
	0x00AE, 0x00A9, 0x2122, 0x00B4, 0x00A8, 0x2260, 0x00C6, 0x00D8,
	0x221E, 0x00B1, 0x2264, 0x2265, 0x00A5, 0x00B5, 0x2202, 0x2211,
	0x220F, 0x03C0, 0x222B, 0x00AA, 0x00BA, 0x03A9, 0x00E6, 0x00F8,
	0x00BF, 0x00A1, 0x00AC, 0x221A, 0x0192, 0x2248, 0x2206, 0x00AB,
	0x00BB, 0x2026, 0x00A0, 0x00C0, 0x00C3, 0x00D5, 0x0152, 0x0153,
	0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x25CA,
	0x00FF, 0x0178, 0x2044, 0x20AC, 0x2039, 0x203A, 0xFB01, 0xFB02,
	0x2021, 0x00B7, 0x201A, 0x201E, 0x2030, 0x00C2, 0x00CA, 0x00C1,
	0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC, 0x00D3, 0x00D4,
	0xF8FF, 0x00D2, 0x00DA, 0x00DB, 0x00D9, 0x0131, 0x02C6, 0x02DC,
	0x00AF, 0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7};

uint16_t unicodeToMacRoman(uint32_t unicode)
{
	uint8_t i;

	if (unicode < 128)
		return unicode;

	// faster implementation possible with unicode to macroman table
	for (i = 0; i < 128; i++)
		if (c_read16(&gMacRomanToUnicode[i]) == unicode)
			return i + 128;

	return kInvalidGlyphID;
}
