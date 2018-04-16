/*
	trivial example font engine.

	- font data is array of 8x8 one-bit bitmaps
	- glyphs are sequential starting with ASCII 0
	- metrics always the same: 8 pixel advance, 7 pixel ascent,
		 1 pixel descent, 1 pixel leading
*/

#include "commodettoFontEngine.h"
#include "commodettoPocoBlit.h"
#include "xsPlatform.h"

#if 0 != kPocoRotation
	#error unsupported rotation
#endif

struct CommodettoFontEngineRecord {
	CFEGlyphRecord		glyph;
	const uint8_t		*font;
	uint32_t			glyphCount;
};

typedef struct CommodettoFontEngineRecord CommodettoFontEngineRecord;
typedef struct CommodettoFontEngineRecord TinyFontRecord;
typedef struct CommodettoFontEngineRecord *TinyFont;

CommodettoFontEngine CFENew(void)
{
	CFEGlyph glyph;
	TinyFont tf = c_malloc(sizeof(TinyFontRecord));
	if (!tf)
		return NULL;

	glyph = &tf->glyph;
	glyph->sx = 0;
	glyph->sy = 0;
	glyph->w = 8;
	glyph->h = 8;
	glyph->dx = 0;
	glyph->dy = 0;
	glyph->advance = 8;
	glyph->format = kCommodettoBitmapMonochrome;

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
	tf->font = fontData;
	tf->glyphCount = fontDataSize >> 3;
}

void CFESetFontSize(CommodettoFontEngine tf, int32_t size) {}

void CFEGetFontMetrics(CommodettoFontEngine tf, int32_t *ascent, int32_t *descent, int32_t *leading)
{
	if (ascent)
		*ascent = 7;
	if (descent)
		*descent = 1;
	if (leading)
		*leading = 1;
}

CFEGlyph CFEGetGlyphFromGlyphID(CommodettoFontEngine tf, uint16_t glyphID, uint8_t needPixels)
{
	return CFEGetGlyphFromUnicode(tf, glyphID, needPixels);
}

CFEGlyph CFEGetGlyphFromUnicode(CommodettoFontEngine tf, uint32_t unicode, uint8_t needPixels)
{
	if (unicode >= tf->glyphCount)
		return NULL;

	tf->glyph.bits = (void *)(tf->font + (unicode << 3));

	return &tf->glyph;
}

int16_t CFEGetKerningOffset(CommodettoFontEngine tf, uint32_t unicode1, uint32_t unicode2)
{
	return 0;
}

void CFELayoutRun(CommodettoFontEngine tf, const char *utf8, int32_t byteLength, CFERun run, int32_t runLength, int32_t width)
{
	const char *utf8End = utf8 + byteLength;

	while (utf8 < utf8End) {
		run->glyphID = (uint16_t)PocoNextFromUTF8((uint8_t **)&utf8);;
		run->advance = 8;

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
