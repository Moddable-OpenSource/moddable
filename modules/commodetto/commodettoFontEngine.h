#include "commodettoBitmap.h"

#define kInvalidGlyphID (0xFFFF)

struct CommodettoFontEngineRecord;
typedef struct CommodettoFontEngineRecord *CommodettoFontEngine;

#ifndef __commodettofontengine_internal__
struct CFEGlyphRecord {
	uint8_t					advance;
	int8_t					dx;
	int8_t					dy;

	uint8_t					format;
	uint16_t				w;
	uint16_t				h;
	void						*bits;

	uint16_t				sx;
	uint16_t				sy;
};
#endif

typedef struct CFEGlyphRecord CFEGlyphRecord;
typedef struct CFEGlyphRecord *CFEGlyph;

typedef struct {
	uint16_t	glyphID;
	int16_t		advance;
} CFERunRecord, *CFERun;

CommodettoFontEngine CFENew(void);
void CFEDispose(CommodettoFontEngine cfe);

void CFESetFontData(CommodettoFontEngine cfe, const void *fontData, uint32_t fontDataSize);
void CFESetFontSize(CommodettoFontEngine cfe, int32_t size);
void CFEGetFontMetrics(CommodettoFontEngine cfe, int32_t *ascent, int32_t *descent, int32_t *leading);

void CFELockCache(CommodettoFontEngine cfe, uint8_t lock);

CFEGlyph CFEGetGlyphFromGlyphID(CommodettoFontEngine cfe, uint16_t glyphID, uint8_t needPixels);
CFEGlyph CFEGetGlyphFromUnicode(CommodettoFontEngine cfe, uint32_t unicode, uint8_t needPixels);

int16_t CFEGetKerningOffset(CommodettoFontEngine cfe, uint32_t unicode1, uint32_t unicode2);

void CFELayoutRun(CommodettoFontEngine cfe, const char *utf8, int32_t byteLength, CFERun run, int32_t runLength, int32_t width);

