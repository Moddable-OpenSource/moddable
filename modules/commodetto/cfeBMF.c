/*
 * Copyright (c) 2018-2026  Moddable Tech, Inc.
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
	unsigned int		charCount;
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
static const char *getASCIISubstitution(uint32_t cp);

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

	glyph->substitute = C_NULL;

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
	if (kInvalidGlyphID == glyphID) {
		bmf->glyph.substitute = getASCIISubstitution(unicode);
		if (bmf->glyph.substitute)
			return &bmf->glyph;
		return C_NULL;
	}

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
		uint16_t glyphID = bmfUnicodeToGlyphID(bmf, unicode);		//@@ subsitute here too
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

typedef struct {
    uint16_t	codepoint;
    char		ascii[4];
} UnicodeEntry;

static const UnicodeEntry TABLE[] = {
//  { 0x0060, "'"   },  /* GRAVE ACCENT (used as open quote)                */
    { 0x00A0, " "   },  /* NO-BREAK SPACE                                   */
    { 0x00A2, "c"   },  /* CENT SIGN                                        */
    { 0x00A3, "#"   },  /* POUND SIGN                                       */
    { 0x00A5, "Y"   },  /* YEN SIGN                                         */
    { 0x00A9, "(C)" },  /* COPYRIGHT SIGN                                   */
    { 0x00AA, "a"   },  /* FEMININE ORDINAL INDICATOR                       */
    { 0x00AB, "\""  },  /* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK        */
    { 0x00AD, "-"   },  /* SOFT HYPHEN                                      */
    { 0x00AE, "(R)" },  /* REGISTERED SIGN                                  */
    { 0x00B2, "2"   },  /* SUPERSCRIPT TWO                                  */
    { 0x00B3, "3"   },  /* SUPERSCRIPT THREE                                */
    { 0x00B4, "'"   },  /* ACUTE ACCENT (used as close quote)               */
    { 0x00B7, "."   },  /* MIDDLE DOT                                       */
    { 0x00B9, "1"   },  /* SUPERSCRIPT ONE                                  */
    { 0x00BA, "o"   },  /* MASCULINE ORDINAL INDICATOR                      */
    { 0x00BB, "\""  },  /* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK       */
    { 0x00C6, "AE"  },  /* LATIN CAPITAL LETTER AE                          */
    { 0x00D0, "D"   },  /* LATIN CAPITAL LETTER ETH                         */
    { 0x00D8, "O"   },  /* LATIN CAPITAL LETTER O WITH STROKE               */
    { 0x00DE, "Th"  },  /* LATIN CAPITAL LETTER THORN                       */
    { 0x00E6, "ae"  },  /* LATIN SMALL LETTER AE                            */
    { 0x00F0, "d"   },  /* LATIN SMALL LETTER ETH                           */
    { 0x00F8, "o"   },  /* LATIN SMALL LETTER O WITH STROKE                 */
    { 0x00FE, "th"  },  /* LATIN SMALL LETTER THORN                         */
    { 0x0132, "IJ"  },  /* LATIN CAPITAL LIGATURE IJ                        */
    { 0x0133, "ij"  },  /* LATIN SMALL LIGATURE IJ                          */
    { 0x0152, "OE"  },  /* LATIN CAPITAL LIGATURE OE                        */
    { 0x0153, "oe"  },  /* LATIN SMALL LIGATURE OE                          */
    { 0x01F1, "DZ"  },  /* LATIN CAPITAL LETTER DZ                          */
    { 0x01F2, "Dz"  },  /* LATIN CAPITAL LETTER D WITH SMALL LETTER Z       */
    { 0x01F3, "dz"  },  /* LATIN SMALL LETTER DZ                            */
    { 0x02B9, "'"   },  /* MODIFIER LETTER PRIME                            */
    { 0x02BB, "'"   },  /* MODIFIER LETTER TURNED COMMA                     */
    { 0x02BC, "'"   },  /* MODIFIER LETTER APOSTROPHE                       */
    { 0x02BD, "'"   },  /* MODIFIER LETTER REVERSED COMMA                   */
    { 0x02BE, "'"   },  /* MODIFIER LETTER RIGHT HALF RING                  */
    { 0x02BF, "'"   },  /* MODIFIER LETTER LEFT HALF RING                   */
    { 0x02C8, "'"   },  /* MODIFIER LETTER VERTICAL LINE                    */
    { 0x0313, "'"   },  /* COMBINING COMMA ABOVE                            */
    { 0x0314, "'"   },  /* COMBINING REVERSED COMMA ABOVE                   */
    { 0x2000, " "   },  /* EN QUAD                                          */
    { 0x2001, " "   },  /* EM QUAD                                          */
    { 0x2002, " "   },  /* EN SPACE                                         */
    { 0x2003, " "   },  /* EM SPACE                                         */
    { 0x2004, " "   },  /* THREE-PER-EM SPACE                               */
    { 0x2005, " "   },  /* FOUR-PER-EM SPACE                                */
    { 0x2006, " "   },  /* SIX-PER-EM SPACE                                 */
    { 0x2007, " "   },  /* FIGURE SPACE                                     */
    { 0x2008, " "   },  /* PUNCTUATION SPACE                                */
    { 0x2009, " "   },  /* THIN SPACE                                       */
    { 0x200A, " "   },  /* HAIR SPACE                                       */
    { 0x200B, ""    },  /* ZERO WIDTH SPACE                                 */
    { 0x200C, ""    },  /* ZERO WIDTH NON-JOINER                            */
    { 0x200D, ""    },  /* ZERO WIDTH JOINER                                */
    { 0x2010, "-"   },  /* HYPHEN                                           */
    { 0x2011, "-"   },  /* NON-BREAKING HYPHEN                              */
    { 0x2012, "-"   },  /* FIGURE DASH                                      */
    { 0x2013, "-"   },  /* EN DASH                                          */
    { 0x2014, "--"  },  /* EM DASH                                          */
    { 0x2015, "--"  },  /* HORIZONTAL BAR                                   */
    { 0x2016, "||"  },  /* DOUBLE VERTICAL LINE                             */
    { 0x2017, "_"   },  /* DOUBLE LOW LINE                                  */
    { 0x2018, "'"   },  /* LEFT SINGLE QUOTATION MARK                       */
    { 0x2019, "'"   },  /* RIGHT SINGLE QUOTATION MARK                      */
    { 0x201A, "'"   },  /* SINGLE LOW-9 QUOTATION MARK                      */
    { 0x201B, "'"   },  /* SINGLE HIGH-REVERSED-9 QUOTATION MARK            */
    { 0x201C, "\""  },  /* LEFT DOUBLE QUOTATION MARK                       */
    { 0x201D, "\""  },  /* RIGHT DOUBLE QUOTATION MARK                      */
    { 0x201E, "\""  },  /* DOUBLE LOW-9 QUOTATION MARK                      */
    { 0x201F, "\""  },  /* DOUBLE HIGH-REVERSED-9 QUOTATION MARK            */
    { 0x2020, "+"   },  /* DAGGER                                           */
    { 0x2021, "++"  },  /* DOUBLE DAGGER                                    */
    { 0x2022, "*"   },  /* BULLET                                           */
    { 0x2023, ">"   },  /* TRIANGULAR BULLET                                */
    { 0x2024, "."   },  /* ONE DOT LEADER                                   */
    { 0x2025, ".."  },  /* TWO DOT LEADER                                   */
    { 0x2026, "..." },  /* HORIZONTAL ELLIPSIS                              */
    { 0x2027, "."   },  /* HYPHENATION POINT                                */
    { 0x202F, " "   },  /* NARROW NO-BREAK SPACE                            */
    { 0x2030, "%"   },  /* PER MILLE SIGN                                   */
    { 0x2031, "%"   },  /* PER TEN THOUSAND SIGN                            */
    { 0x2032, "'"   },  /* PRIME                                            */
    { 0x2033, "\""  },  /* DOUBLE PRIME                                     */
    { 0x2035, "'"   },  /* REVERSED PRIME                                   */
    { 0x2036, "\""  },  /* REVERSED DOUBLE PRIME                            */
    { 0x2038, "^"   },  /* CARET                                            */
    { 0x2039, "<"   },  /* SINGLE LEFT-POINTING ANGLE QUOTATION MARK        */
    { 0x203A, ">"   },  /* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK       */
    { 0x203B, "*"   },  /* REFERENCE MARK                                   */
    { 0x203C, "!!"  },  /* DOUBLE EXCLAMATION MARK                          */
    { 0x203D, "?!"  },  /* INTERROBANG                                      */
    { 0x2041, "^"   },  /* CARET INSERTION POINT                            */
    { 0x2042, "*"   },  /* ASTERISM                                         */
    { 0x2044, "/"   },  /* FRACTION SLASH                                   */
    { 0x2045, "["   },  /* LEFT SQUARE BRACKET WITH QUILL                   */
    { 0x2046, "]"   },  /* RIGHT SQUARE BRACKET WITH QUILL                  */
    { 0x2047, "??"  },  /* DOUBLE QUESTION MARK                             */
    { 0x2048, "?!"  },  /* QUESTION EXCLAMATION MARK                        */
    { 0x2049, "!?"  },  /* EXCLAMATION QUESTION MARK                        */
    { 0x204E, "*"   },  /* LOW ASTERISK                                     */
    { 0x2052, "%"   },  /* COMMERCIAL MINUS SIGN                            */
    { 0x2053, "~"   },  /* SWUNG DASH                                       */
    { 0x2054, "_"   },  /* INVERTED UNDERTIE                                */
    { 0x2057, "\"\""  },/* QUADRUPLE PRIME (→ "" — fits 3-char limit;       */
                        /*   '''' would need 5 bytes incl. NUL)             */
    { 0x205F, " "   },  /* MEDIUM MATHEMATICAL SPACE                        */
    { 0x2060, ""    },  /* WORD JOINER                                      */
    { 0x2070, "0"   },  /* SUPERSCRIPT ZERO                                 */
    { 0x2071, "i"   },  /* SUPERSCRIPT LATIN SMALL LETTER I                 */
    { 0x2074, "4"   },  /* SUPERSCRIPT FOUR                                 */
    { 0x2075, "5"   },  /* SUPERSCRIPT FIVE                                 */
    { 0x2076, "6"   },  /* SUPERSCRIPT SIX                                  */
    { 0x2077, "7"   },  /* SUPERSCRIPT SEVEN                                */
    { 0x2078, "8"   },  /* SUPERSCRIPT EIGHT                                */
    { 0x2079, "9"   },  /* SUPERSCRIPT NINE                                 */
    { 0x207A, "+"   },  /* SUPERSCRIPT PLUS SIGN                            */
    { 0x207B, "-"   },  /* SUPERSCRIPT MINUS                                */
    { 0x207C, "="   },  /* SUPERSCRIPT EQUALS SIGN                          */
    { 0x207D, "("   },  /* SUPERSCRIPT LEFT PARENTHESIS                     */
    { 0x207E, ")"   },  /* SUPERSCRIPT RIGHT PARENTHESIS                    */
    { 0x207F, "n"   },  /* SUPERSCRIPT LATIN SMALL LETTER N                 */
    { 0x2080, "0"   },  /* SUBSCRIPT ZERO                                   */
    { 0x2081, "1"   },  /* SUBSCRIPT ONE                                    */
    { 0x2082, "2"   },  /* SUBSCRIPT TWO                                    */
    { 0x2083, "3"   },  /* SUBSCRIPT THREE                                  */
    { 0x2084, "4"   },  /* SUBSCRIPT FOUR                                   */
    { 0x2085, "5"   },  /* SUBSCRIPT FIVE                                   */
    { 0x2086, "6"   },  /* SUBSCRIPT SIX                                    */
    { 0x2087, "7"   },  /* SUBSCRIPT SEVEN                                  */
    { 0x2088, "8"   },  /* SUBSCRIPT EIGHT                                  */
    { 0x2089, "9"   },  /* SUBSCRIPT NINE                                   */
    { 0x208A, "+"   },  /* SUBSCRIPT PLUS SIGN                              */
    { 0x208B, "-"   },  /* SUBSCRIPT MINUS                                  */
    { 0x208C, "="   },  /* SUBSCRIPT EQUALS SIGN                            */
    { 0x208D, "("   },  /* SUBSCRIPT LEFT PARENTHESIS                       */
    { 0x208E, ")"   },  /* SUBSCRIPT RIGHT PARENTHESIS                      */
    { 0x20AC, "E"   },  /* EURO SIGN                                        */
    { 0x2100, "a/c" },  /* ACCOUNT OF                                       */
    { 0x2101, "a/s" },  /* ADDRESSED TO THE SUBJECT                         */
    { 0x2102, "C"   },  /* DOUBLE-STRUCK CAPITAL C                          */
    { 0x2103, "C"   },  /* DEGREE CELSIUS                                   */
    { 0x2105, "c/o" },  /* CARE OF                                          */
    { 0x2106, "c/u" },  /* CADA UNA                                         */
    { 0x2109, "F"   },  /* DEGREE FAHRENHEIT                                */
    { 0x2111, "I"   },  /* BLACK-LETTER CAPITAL I                           */
    { 0x2113, "l"   },  /* SCRIPT SMALL L                                   */
    { 0x2115, "N"   },  /* DOUBLE-STRUCK CAPITAL N                          */
    { 0x2116, "No"  },  /* NUMERO SIGN                                      */
    { 0x2119, "P"   },  /* DOUBLE-STRUCK CAPITAL P                          */
    { 0x211A, "Q"   },  /* DOUBLE-STRUCK CAPITAL Q                          */
    { 0x211B, "R"   },  /* SCRIPT CAPITAL R                                 */
    { 0x211C, "R"   },  /* BLACK-LETTER CAPITAL R                           */
    { 0x211D, "R"   },  /* DOUBLE-STRUCK CAPITAL R                          */
    { 0x2120, "SM"  },  /* SERVICE MARK                                     */
    { 0x2121, "TEL" },  /* TELEPHONE SIGN                                   */
    { 0x2122, "TM"  },  /* TRADE MARK SIGN                                  */
    { 0x2124, "Z"   },  /* DOUBLE-STRUCK CAPITAL Z                          */
    { 0x2126, "Ohm" },  /* OHM SIGN                                         */
    { 0x212A, "K"   },  /* KELVIN SIGN                                      */
    { 0x212B, "A"   },  /* ANGSTROM SIGN                                    */
    { 0x2132, "F"   },  /* TURNED CAPITAL F                                 */
    { 0x213B, "FAX" },  /* FACSIMILE SIGN                                   */
    { 0x2140, "sum" },  /* DOUBLE-STRUCK N-ARY SUMMATION                    */
    { 0x2145, "D"   },  /* DOUBLE-STRUCK ITALIC CAPITAL D                   */
    { 0x2146, "d"   },  /* DOUBLE-STRUCK ITALIC SMALL D                     */
    { 0x2147, "e"   },  /* DOUBLE-STRUCK ITALIC SMALL E                     */
    { 0x2148, "i"   },  /* DOUBLE-STRUCK ITALIC SMALL I                     */
    { 0x2149, "j"   },  /* DOUBLE-STRUCK ITALIC SMALL J                     */
    { 0x2212, "-"   },  /* MINUS SIGN                                       */
    { 0x2215, "/"   },  /* DIVISION SLASH                                   */
    { 0x2260, "!="  },  /* NOT EQUAL TO                                     */
    { 0x2264, "<="  },  /* LESS-THAN OR EQUAL TO                            */
    { 0x2265, ">="  },  /* GREATER-THAN OR EQUAL TO                         */
    { 0x22EF, "..." },  /* MIDLINE HORIZONTAL ELLIPSIS                      */
    { 0x275B, "'"   },  /* HEAVY SINGLE TURNED COMMA QUOTATION MARK         */
    { 0x275C, "'"   },  /* HEAVY SINGLE COMMA QUOTATION MARK                */
    { 0x275D, "\""  },  /* HEAVY DOUBLE TURNED COMMA QUOTATION MARK         */
    { 0x275E, "\""  },  /* HEAVY DOUBLE COMMA QUOTATION MARK                */
    { 0x276A, "("   },  /* MEDIUM LEFT PARENTHESIS ORNAMENT                 */
    { 0x276B, ")"   },  /* MEDIUM RIGHT PARENTHESIS ORNAMENT                */
    { 0x29F8, "/"   },  /* BIG SOLIDUS                                      */
    { 0x3000, " "   },  /* IDEOGRAPHIC SPACE                                */
    { 0x301D, "\""  },  /* REVERSED DOUBLE PRIME QUOTATION MARK             */
    { 0x301E, "\""  },  /* DOUBLE PRIME QUOTATION MARK                      */
    { 0xFB00, "ff"  },  /* LATIN SMALL LIGATURE FF                          */
    { 0xFB01, "fi"  },  /* LATIN SMALL LIGATURE FI                          */
    { 0xFB02, "fl"  },  /* LATIN SMALL LIGATURE FL                          */
    { 0xFB03, "ffi" },  /* LATIN SMALL LIGATURE FFI                         */
    { 0xFB04, "ffl" },  /* LATIN SMALL LIGATURE FFL                         */
    { 0xFB05, "st"  },  /* LATIN SMALL LIGATURE LONG S T                    */
    { 0xFB06, "st"  },  /* LATIN SMALL LIGATURE ST                          */
    { 0xFE19, "..." },  /* PRESENTATION FORM FOR VERTICAL HORIZONTAL ELLIPSIS */
    { 0xFE41, "'"   },  /* PRESENTATION FORM FOR VERTICAL LEFT CORNER BRACKET */
    { 0xFE58, "-"   },  /* SMALL EM DASH                                    */
    { 0xFE63, "-"   },  /* SMALL HYPHEN-MINUS                               */
    { 0xFEFF, ""    },  /* ZERO WIDTH NO-BREAK SPACE / BOM                  */
    { 0xFF01, "!"   },  /* FULLWIDTH EXCLAMATION MARK                       */
    { 0xFF02, "\""  },  /* FULLWIDTH QUOTATION MARK                         */
    { 0xFF07, "'"   },  /* FULLWIDTH APOSTROPHE                             */
    { 0xFF08, "("   },  /* FULLWIDTH LEFT PARENTHESIS                       */
    { 0xFF09, ")"   },  /* FULLWIDTH RIGHT PARENTHESIS                      */
    { 0xFF0C, ","   },  /* FULLWIDTH COMMA                                  */
    { 0xFF0D, "-"   },  /* FULLWIDTH HYPHEN-MINUS                           */
    { 0xFF0E, "."   },  /* FULLWIDTH FULL STOP                              */
    { 0xFF0F, "/"   },  /* FULLWIDTH SOLIDUS                                */
    { 0xFF1A, ":"   },  /* FULLWIDTH COLON                                  */
    { 0xFF1B, ";"   },  /* FULLWIDTH SEMICOLON                              */
    { 0xFF1F, "?"   },  /* FULLWIDTH QUESTION MARK                          */
    { 0xFF20, "@"   },  /* FULLWIDTH COMMERCIAL AT                          */
    { 0xFF3B, "["   },  /* FULLWIDTH LEFT SQUARE BRACKET                    */
    { 0xFF3C, "\\"  },  /* FULLWIDTH REVERSE SOLIDUS                        */
    { 0xFF3D, "]"   },  /* FULLWIDTH RIGHT SQUARE BRACKET                   */
    { 0xFF3E, "^"   },  /* FULLWIDTH CIRCUMFLEX ACCENT                      */
    { 0xFF3F, "_"   },  /* FULLWIDTH LOW LINE                               */
    { 0xFF40, "`"   },  /* FULLWIDTH GRAVE ACCENT                           */
    { 0xFF5B, "{"   },  /* FULLWIDTH LEFT CURLY BRACKET                     */
    { 0xFF5C, "|"   },  /* FULLWIDTH VERTICAL LINE                          */
    { 0xFF5D, "}"   },  /* FULLWIDTH RIGHT CURLY BRACKET                    */
    { 0xFF5E, "~"   },  /* FULLWIDTH TILDE                                  */
};

const char *getASCIISubstitution(uint32_t cp)
{
    size_t lo = 0, hi = (sizeof(TABLE) / sizeof(TABLE[0]));

    while (lo < hi) {
        size_t mid = lo + ((hi - lo) >> 1);
        if (TABLE[mid].codepoint < cp)
			lo = mid + 1;
        else if (TABLE[mid].codepoint > cp)
			hi = mid;
        else
			return TABLE[mid].ascii;
    }

    return C_NULL;
}
