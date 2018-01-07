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

#include "xsAll.h"

//#define mxTrace 1

// COMMON

static txCharCase* fxCharCaseFind(txInteger character, txBoolean flag, txBoolean* inside);
static txInteger fxCharCaseCanonicalize(txCharCase* current, txInteger character, txBoolean flag);

enum {
	cxMatchStep,
	cxAssertionStep,
	cxAssertionCompletion,
	cxAssertionNotStep,
	cxAssertionNotCompletion,
	cxCaptureForwardStep,
	cxCaptureForwardCompletion,
	cxCaptureBackwardStep,
	cxCaptureBackwardCompletion,
	cxCaptureReferenceForwardStep,
	cxCaptureReferenceBackwardStep,
	cxCharSetForwardStep,
	cxCharSetBackwardStep,
	cxDisjunctionStep,
	cxEmptyStep,
	cxLineBeginStep,
	cxLineEndStep,
	cxQuantifierStep,
	cxQuantifierGreedyLoop,
	cxQuantifierLazyLoop,
	cxQuantifierCompletion,
	cxWordBreakStep,
	cxWordContinueStep
};

#define mxCharCaseFoldingCount 189
static const txCharCase gxCharCaseFoldings[mxCharCaseFoldingCount] ICACHE_XS6RO_ATTR = {
	{0x41,0x5A,32},{0xB5,0xB5,775},{0xC0,0xD6,32},{0xD8,0xDE,32},{0x100,0x12E,0},{0x132,0x136,0},{0x139,0x147,0},
	{0x14A,0x176,0},{0x178,0x178,-121},{0x179,0x17D,0},{0x17F,0x17F,-268},{0x181,0x181,210},{0x182,0x184,0},{0x186,0x186,206},
	{0x187,0x187,0},{0x189,0x18A,205},{0x18B,0x18B,0},{0x18E,0x18E,79},{0x18F,0x18F,202},{0x190,0x190,203},{0x191,0x191,0},
	{0x193,0x193,205},{0x194,0x194,207},{0x196,0x196,211},{0x197,0x197,209},{0x198,0x198,0},{0x19C,0x19C,211},{0x19D,0x19D,213},
	{0x19F,0x19F,214},{0x1A0,0x1A4,0},{0x1A6,0x1A6,218},{0x1A7,0x1A7,0},{0x1A9,0x1A9,218},{0x1AC,0x1AC,0},{0x1AE,0x1AE,218},
	{0x1AF,0x1AF,0},{0x1B1,0x1B2,217},{0x1B3,0x1B5,0},{0x1B7,0x1B7,219},{0x1B8,0x1B8,0},{0x1BC,0x1BC,0},{0x1C4,0x1C4,2},
	{0x1C5,0x1C5,0},{0x1C7,0x1C7,2},{0x1C8,0x1C8,0},{0x1CA,0x1CA,2},{0x1CB,0x1DB,0},{0x1DE,0x1EE,0},{0x1F1,0x1F1,2},
	{0x1F2,0x1F4,0},{0x1F6,0x1F6,-97},{0x1F7,0x1F7,-56},{0x1F8,0x21E,0},{0x220,0x220,-130},{0x222,0x232,0},{0x23A,0x23A,10795},
	{0x23B,0x23B,0},{0x23D,0x23D,-163},{0x23E,0x23E,10792},{0x241,0x241,0},{0x243,0x243,-195},{0x244,0x244,69},{0x245,0x245,71},
	{0x246,0x24E,0},{0x345,0x345,116},{0x370,0x372,0},{0x376,0x376,0},{0x37F,0x37F,116},{0x386,0x386,38},{0x388,0x38A,37},
	{0x38C,0x38C,64},{0x38E,0x38F,63},{0x391,0x3A1,32},{0x3A3,0x3AB,32},{0x3C2,0x3C2,0},{0x3CF,0x3CF,8},{0x3D0,0x3D0,-30},
	{0x3D1,0x3D1,-25},{0x3D5,0x3D5,-15},{0x3D6,0x3D6,-22},{0x3D8,0x3EE,0},{0x3F0,0x3F0,-54},{0x3F1,0x3F1,-48},{0x3F4,0x3F4,-60},
	{0x3F5,0x3F5,-64},{0x3F7,0x3F7,0},{0x3F9,0x3F9,-7},{0x3FA,0x3FA,0},{0x3FD,0x3FF,-130},{0x400,0x40F,80},{0x410,0x42F,32},
	{0x460,0x480,0},{0x48A,0x4BE,0},{0x4C0,0x4C0,15},{0x4C1,0x4CD,0},{0x4D0,0x52E,0},{0x531,0x556,48},{0x10A0,0x10C5,7264},
	{0x10C7,0x10C7,7264},{0x10CD,0x10CD,7264},{0x13F8,0x13FD,-8},{0x1C80,0x1C80,-6222},{0x1C81,0x1C81,-6221},{0x1C82,0x1C82,-6212},{0x1C83,0x1C84,-6210},
	{0x1C85,0x1C85,-6211},{0x1C86,0x1C86,-6204},{0x1C87,0x1C87,-6180},{0x1C88,0x1C88,35267},{0x1E00,0x1E94,0},{0x1E9B,0x1E9B,-58},{0x1E9E,0x1E9E,-7615},
	{0x1EA0,0x1EFE,0},{0x1F08,0x1F0F,-8},{0x1F18,0x1F1D,-8},{0x1F28,0x1F2F,-8},{0x1F38,0x1F3F,-8},{0x1F48,0x1F4D,-8},{0x1F59,0x1F59,-8},
	{0x1F5B,0x1F5B,-8},{0x1F5D,0x1F5D,-8},{0x1F5F,0x1F5F,-8},{0x1F68,0x1F6F,-8},{0x1F88,0x1F8F,-8},{0x1F98,0x1F9F,-8},{0x1FA8,0x1FAF,-8},
	{0x1FB8,0x1FB9,-8},{0x1FBA,0x1FBB,-74},{0x1FBC,0x1FBC,-9},{0x1FBE,0x1FBE,-7173},{0x1FC8,0x1FCB,-86},{0x1FCC,0x1FCC,-9},{0x1FD8,0x1FD9,-8},
	{0x1FDA,0x1FDB,-100},{0x1FE8,0x1FE9,-8},{0x1FEA,0x1FEB,-112},{0x1FEC,0x1FEC,-7},{0x1FF8,0x1FF9,-128},{0x1FFA,0x1FFB,-126},{0x1FFC,0x1FFC,-9},
	{0x2126,0x2126,-7517},{0x212A,0x212A,-8383},{0x212B,0x212B,-8262},{0x2132,0x2132,28},{0x2160,0x216F,16},{0x2183,0x2183,0},{0x24B6,0x24CF,26},
	{0x2C00,0x2C2E,48},{0x2C60,0x2C60,0},{0x2C62,0x2C62,-10743},{0x2C63,0x2C63,-3814},{0x2C64,0x2C64,-10727},{0x2C67,0x2C6B,0},{0x2C6D,0x2C6D,-10780},
	{0x2C6E,0x2C6E,-10749},{0x2C6F,0x2C6F,-10783},{0x2C70,0x2C70,-10782},{0x2C72,0x2C72,0},{0x2C75,0x2C75,0},{0x2C7E,0x2C7F,-10815},{0x2C80,0x2CE2,0},
	{0x2CEB,0x2CED,0},{0x2CF2,0x2CF2,0},{0xA640,0xA66C,0},{0xA680,0xA69A,0},{0xA722,0xA72E,0},{0xA732,0xA76E,0},{0xA779,0xA77B,0},
	{0xA77D,0xA77D,-35332},{0xA77E,0xA786,0},{0xA78B,0xA78B,0},{0xA78D,0xA78D,-42280},{0xA790,0xA792,0},{0xA796,0xA7A8,0},{0xA7AA,0xA7AA,-42308},
	{0xA7AB,0xA7AB,-42319},{0xA7AC,0xA7AC,-42315},{0xA7AD,0xA7AD,-42305},{0xA7AE,0xA7AE,-42308},{0xA7B0,0xA7B0,-42258},{0xA7B1,0xA7B1,-42282},{0xA7B2,0xA7B2,-42261},
	{0xA7B3,0xA7B3,928},{0xA7B4,0xA7B6,0},{0xAB70,0xABBF,-38864},{0x10400,0x10427,40},{0x104B0,0x104D3,40},{0x10C80,0x10CB2,64},{0x118A0,0x118BF,32}
};

// COMPILE

typedef struct sxPatternParser txPatternParser;

typedef void (*txTermMeasure)(txPatternParser*, void*, txInteger);
typedef void (*txTermCode)(txPatternParser*, void*, txInteger, txInteger);

#define txTermPart\
	void* next;\
	union {\
		txTermMeasure measure;\
		txTermCode code;\
	} dispatch;\
	txInteger step

typedef struct {
	txTermPart;
} txTerm;

typedef struct {
	txTermPart;
	txTerm* term;
	txBoolean not;
	txInteger direction;
	txInteger assertionIndex;
	txInteger completion;
} txAssertion;

typedef struct sxCapture txCapture;
struct sxCapture {
	txTermPart;
	txTerm* term;
	txInteger captureIndex;
	txInteger completion;
	txCapture* nextNamedCapture;
	char name[1];
};

typedef struct {
	txTermPart;
	txInteger captureIndex;
	char name[1];
} txCaptureReference;

typedef struct {
	txTermPart;
	txInteger characters[1];
} txCharSet;

typedef struct {
	txTermPart;
	txTerm* left;
	txTerm* right;
} txDisjunction;

typedef struct {
	txTermPart;
	txTerm* term;
	txInteger min;
	txInteger max;
	txBoolean greedy;
	txInteger captureIndex;
	txInteger captureCount;
	txInteger quantifierIndex;
	txInteger loop;
	txInteger completion;
} txQuantifier;

typedef struct {
	txTermPart;
	txTerm* left;
	txTerm* right;
} txSequence;

struct sxPatternParser {
	txTerm* first;
	
	txU1* pattern;
	txInteger offset;
	
	txUnsigned flags;
	
	txMachine* the;
	
	txInteger character;
	txInteger surrogate;
		
	txInteger assertionIndex;
	txInteger captureIndex;
	txInteger quantifierIndex;
	txCapture* firstNamedCapture;
	
	txInteger size;

	txInteger** code;
	txByte* buffer;
	
	c_jmp_buf jmp_buf;
	char error[256];
};

static void* fxCharSetAny(txPatternParser* parser);
static void* fxCharSetDigits(txPatternParser* parser);
static void* fxCharSetEmpty(txPatternParser* parser);
static void* fxCharSetNot(txPatternParser* parser, txCharSet* set);
static void* fxCharSetOr(txPatternParser* parser, txCharSet* set1, txCharSet* set2);
static void* fxCharSetParseEscape(txPatternParser* parser);
static void* fxCharSetParseItem(txPatternParser* parser);
static void* fxCharSetParseList(txPatternParser* parser);
static void* fxCharSetRange(txPatternParser* parser, txCharSet* set1, txCharSet* set2);
static void* fxCharSetSingle(txPatternParser* parser, txInteger character);
static void* fxCharSetSpaces(txPatternParser* parser);
static void* fxCharSetWords(txPatternParser* parser);
static void* fxDisjunctionParse(txPatternParser* parser, txInteger character);
static void* fxQuantifierParse(txPatternParser* parser, void* term, txInteger captureIndex);
static txInteger fxQuantifierParseDigits(txPatternParser* parser);
static void* fxSequenceParse(txPatternParser* parser, txInteger character);
static void* fxTermCreate(txPatternParser* parser, size_t size, txTermMeasure measure);

static void fxAssertionMeasure(txPatternParser* parser, void* it, txInteger direction);
static void fxCaptureMeasure(txPatternParser* parser, void* it, txInteger direction);
static void fxCaptureReferenceMeasure(txPatternParser* parser, void* it, txInteger direction);
static void fxCharSetMeasure(txPatternParser* parser, void* it, txInteger direction);
static void fxDisjunctionMeasure(txPatternParser* parser, void* it, txInteger direction);
static void fxEmptyMeasure(txPatternParser* parser, void* it, txInteger direction);
static void fxLineBeginMeasure(txPatternParser* parser, void* it, txInteger direction);
static void fxLineEndMeasure(txPatternParser* parser, void* it, txInteger direction);
static void fxQuantifierMeasure(txPatternParser* parser, void* it, txInteger direction);
static void fxSequenceMeasure(txPatternParser* parser, void* it, txInteger direction);
static void fxWordBreakMeasure(txPatternParser* parser, void* it, txInteger direction);
static void fxWordContinueMeasure(txPatternParser* parser, void* it, txInteger direction);

static void fxAssertionCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel);
static void fxCaptureCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel);
static void fxCaptureReferenceCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel);
static void fxCharSetCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel);
static void fxDisjunctionCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel);
static void fxEmptyCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel);
static void fxLineBeginCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel);
static void fxLineEndCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel);
static void fxQuantifierCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel);
static void fxSequenceCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel);
static void fxWordBreakCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel);
static void fxWordContinueCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel);

static void fxPatternParserInitialize(txPatternParser* parser);
static txBoolean fxPatternParserDecimal(txPatternParser* parser, txU4* value);
static void fxPatternParserError(txPatternParser* parser, txString format, ...);
static txBoolean fxPatternParserHexadecimal(txPatternParser* parser, txU4* value);
static void fxPatternParserName(txPatternParser* parser, txInteger* length);
static void fxPatternParserNamedCapture(txPatternParser* parser, txCapture* capture);
static void fxPatternParserNext(txPatternParser* parser);
static void fxPatternParserTerminate(txPatternParser* parser);
static txBoolean fxPatternParserUnicodeEscape(txPatternParser* parser, txU4* character);

#define mxCodeSize sizeof(txInteger)
#define mxStepSize sizeof(txInteger)
#define mxIndexSize sizeof(txInteger)
#define mxTermStepSize mxCodeSize + mxStepSize
#define mxAssertionNotStepSize mxTermStepSize + mxIndexSize + mxStepSize
#define mxAssertionNotCompletionSize mxCodeSize + mxIndexSize
#define mxAssertionStepSize mxTermStepSize + mxIndexSize
#define mxAssertionCompletionSize mxTermStepSize + mxIndexSize
#define mxCaptureStepSize mxTermStepSize + mxIndexSize
#define mxCaptureCompletionSize mxTermStepSize + mxIndexSize
#define mxCaptureReferenceStepSize mxTermStepSize + mxIndexSize
#define mxDisjunctionStepSize mxTermStepSize + mxStepSize
#define mxQuantifierStepSize mxTermStepSize + mxIndexSize + (2 * sizeof(txInteger))
#define mxQuantifierLoopSize mxTermStepSize + mxIndexSize + mxStepSize + mxIndexSize + mxIndexSize
#define mxQuantifierCompletionSize mxTermStepSize + mxIndexSize + mxStepSize

enum {
	mxDuplicateCapture,
	mxInvalidCharacter,
	mxInvalidEscape,
	mxInvalidFlags,
	mxInvalidGroup,
	mxInvalidName,
	mxInvalidPattern,
	mxInvalidQuantifier,
	mxInvalidRange,
	mxInvalidReferenceName,
	mxInvalidReferenceNumber,
	mxInvalidSequence,
	mxInvalidUTF8,
	mxNameOverflow,
	mxNotEnoughMemory,
	mxErrorCount
};
static const txString gxErrors[mxErrorCount] ICACHE_XS6RO_ATTR = {
	"duplicate capture",
	"invalid character",
	"invalid escape",
	"invalid flags",
	"invalid group",
	"invalid name",
	"invalid pattern",
	"invalid quantifier",
	"invalid range",
	"invalid reference name \\k<%s>",
	"invalid reference number \\%d",
	"invalid sequence",
	"invalid UTF-8",
	"name overflow",
	"not enough memory",
};

// MATCH

typedef struct sxStateData txStateData;

typedef struct {
	txInteger offset;
	txStateData* firstState;
} txAssertionData;

typedef struct {
	txInteger from;
	txInteger to;
} txCaptureData;

typedef struct {
	txInteger min;
	txInteger max;
	txInteger offset;
} txQuantifierData;

struct sxStateData {
	txMachine* the;
	txStateData* nextState;
	txInteger step;
	txInteger offset;
	txCaptureData captures[1];
};

static txInteger fxFindCharacter(txString input, txInteger offset, txInteger direction);
static txInteger fxGetCharacter(txString input, txInteger offset, txInteger flags);
static txBoolean fxMatchCharacter(txInteger* characters, txInteger character);
static txStateData* fxPopStates(txMachine* the, txStateData* fromState, txStateData* toState);
static txStateData* fxPushState(txMachine* the, txStateData* firstState, txInteger step, txInteger offset, txCaptureData* captures, txInteger captureCount);

#ifdef mxTrace
	static txString gxStepNames[cxWordContinueStep + 1] = {
		"MatchStep",
		"AssertionStep",
		"AssertionCompletion",
		"AssertionNotStep",
		"AssertionNotCompletion",
		"CaptureForwardStep",
		"CaptureForwardCompletion",
		"CaptureBackwardStep",
		"CaptureBackwardCompletion",
		"CaptureReferenceForwardStep",
		"CaptureReferenceBackwardStep",
		"CharSetForwardStep",
		"CharSetBackwardStep",
		"DisjunctionStep",
		"EmptyStep",
		"LineBeginStep",
		"LineEndStep",
		"QuantifierStep",
		"QuantifierGreedyLoop",
		"QuantifierLazyLoop",
		"QuantifierCompletion",
		"WordBreakStep",
		"WordContinueStep"
	};
#endif

static const txInteger gxLineCharacters[7] ICACHE_XS6RO_ATTR = { 6, 0x000A, 0x000A + 1, 0x000D, 0x000D + 1, 0x2028, 0x2029 + 1 };
static const txInteger gxWordCharacters[9] ICACHE_XS6RO_ATTR = { 8, '0', '9' + 1, 'A', 'Z' + 1, '_', '_' + 1, 'a', 'z' + 1 };

// COMMON

txCharCase* fxCharCaseFind(txInteger character, txBoolean flag, txBoolean* inside)
{
	const txCharCase* current = flag ? (txCharCase*)&gxCharCaseFoldings[0] : &gxCharCaseToUpper[0];
	const txCharCase* limit = flag ? &gxCharCaseFoldings[mxCharCaseFoldingCount] : &gxCharCaseToUpper[mxCharCaseToUpperCount];
	*inside = 0;
	while (current < limit) {
		if (character < current->from) {
			break;
		}
		if (character <= current->to) {
			*inside = 1;
			break;
		}
		current++;
	}
	return (txCharCase*)current;
}

txInteger fxCharCaseCanonicalize(txCharCase* current, txInteger character, txBoolean flag)
{
	const txCharCase* limit = flag ? &gxCharCaseFoldings[mxCharCaseFoldingCount] : &gxCharCaseToUpper[mxCharCaseToUpperCount];
	if ((current < limit) && (current->from <= character)) {
		if (current->delta)
			character += current->delta;
		else if (flag)
			character |= 1;
		else
			character &= ~1;
	}
	return character;
}

// COMPILE

void* fxCharSetAny(txPatternParser* parser)
{
	txCharSet* result;
	if (parser->flags & XS_REGEXP_S) {
		result = fxTermCreate(parser, sizeof(txCharSet) + (2 * sizeof(txInteger)), fxCharSetMeasure);
		result->characters[0] = 2;
		result->characters[1] = 0x0000;
		result->characters[2] = 0x7FFFFFFF;
	}
	else {
		result = fxTermCreate(parser, sizeof(txCharSet) + (8 * sizeof(txInteger)), fxCharSetMeasure);
		result->characters[0] = 8;
		result->characters[1] = 0x0000;
		result->characters[2] = 0x000A;
		result->characters[3] = 0x000B;
		result->characters[4] = 0x000D;
		result->characters[5] = 0x000E;
		result->characters[6] = 0x2028;
		result->characters[7] = 0x2030;
		result->characters[8] = 0x7FFFFFFF;
	}
	return result;
}

void* fxCharSetCanonicalizeSingle(txPatternParser* parser, txCharSet* set)
{
	if ((parser->flags & XS_REGEXP_I)&& (set->characters[0] == 2) && (set->characters[1] + 1 == set->characters[2])) {
		txBoolean flag = (parser->flags & XS_REGEXP_U) ? 1 : 0, inside;
		txCharCase* current = fxCharCaseFind(set->characters[1], flag, &inside);
		if (inside) {
			set->characters[1] = fxCharCaseCanonicalize(current, set->characters[1], flag);
			set->characters[2] = set->characters[1] + 1;
		}
	}
	return set;
}

void* fxCharSetDigits(txPatternParser* parser)
{
	txCharSet* result = fxTermCreate(parser, sizeof(txCharSet) + (2 * sizeof(txInteger)), fxCharSetMeasure);
	result->characters[0] = 2;
	result->characters[1] = '0';
	result->characters[2] = '9' + 1;
	return result;
}

void* fxCharSetEmpty(txPatternParser* parser)
{
	txCharSet* result = fxTermCreate(parser, sizeof(txCharSet), fxCharSetMeasure);
	result->characters[0] = 0;
	return result;
}

void* fxCharSetNot(txPatternParser* parser, txCharSet* set)
{
	txCharSet* result = fxTermCreate(parser, sizeof(txCharSet) + ((set->characters[0] + 2) * sizeof(txInteger)), fxCharSetMeasure);
	txInteger* current1 = set->characters + 1;
	txInteger* limit1 = current1 + set->characters[0];
	txInteger count0 = 0;
	txInteger* current0 = result->characters + 1;
	txInteger character = 0;
	while (current1 < limit1) {
		txInteger begin = *current1++;
		txInteger end = *current1++;
		if (character < begin) {
			count0++;
			*current0++ = character;
			count0++;
			*current0++ = begin;
			character = end;
		}
	}
	if (character < 0x7FFFFFFF) {
		count0++;
		*current0++ = character;
		count0++;
		*current0++ = 0x7FFFFFFF;
	}
	result->characters[0] = count0;
	return result;
}

void* fxCharSetOr(txPatternParser* parser, txCharSet* set1, txCharSet* set2)
{
	txInteger flag = 0;
	txInteger old = 0;
	txInteger* current1 = set1->characters + 1;
	txInteger* limit1 = current1 + set1->characters[0];
	txInteger* current2 = set2->characters + 1;
	txInteger* limit2 = current2 + set2->characters[0];
	txCharSet* result = fxTermCreate(parser, sizeof(txCharSet) + ((set1->characters[0] + set2->characters[0]) * sizeof(txInteger)), fxCharSetMeasure);
	txInteger count0 = 0;
	txInteger* current0 = result->characters + 1;
	
	while ((current1 < limit1) && (current2 < limit2)) {
		txInteger test = *current1 - *current2;
		txInteger character;
		if (test <= 0) {
			character = *current1;
			flag ^= 1;
			current1++;
		}
		if (test >= 0) {
			character = *current2;
			flag ^= 2;
			current2++;
		}
		if ((flag == 0) || (old == 0)) {
			count0++;
			*current0++ = character;
		}
		old = flag;
	}
	while (current1 < limit1) {
		count0++;
		*current0++ = *current1++;
	}
	while (current2 < limit2) {
		count0++;
		*current0++ = *current2++;
	}
	result->characters[0] = count0;
	return result;
}

void* fxCharSetParseEscape(txPatternParser* parser)
{
	void* result = NULL;
	switch(parser->character) {
	case C_EOF:
		break;
	// character classes
	case 'd': 
		result = fxCharSetDigits(parser); 
		fxPatternParserNext(parser);
		break;
	case 'D':
		result = fxCharSetNot(parser, fxCharSetDigits(parser));
		fxPatternParserNext(parser);
		break;
	case 's':
		result = fxCharSetSpaces(parser);
		fxPatternParserNext(parser);
		break;
	case 'S':
		result = fxCharSetNot(parser, fxCharSetSpaces(parser));
		fxPatternParserNext(parser);
		break;
	case 'w':
		result = fxCharSetWords(parser);
		fxPatternParserNext(parser);
		break;
	case 'W':
		result = fxCharSetNot(parser, fxCharSetWords(parser));
		fxPatternParserNext(parser);
		break;
	// control escapes
	case 'f':
		result = fxCharSetSingle(parser, '\f');
		fxPatternParserNext(parser);
		break;
	case 'n':
		result = fxCharSetSingle(parser, '\n');
		fxPatternParserNext(parser);
		break;
	case 'r':
		result = fxCharSetSingle(parser, '\r');
		fxPatternParserNext(parser);
		break;
	case 't':
		result = fxCharSetSingle(parser, '\t');
		fxPatternParserNext(parser);
		break;
	case 'v':
		result = fxCharSetSingle(parser, '\v');
		fxPatternParserNext(parser);
		break;
	// control letters
	case 'c': {
		txInteger value;
		fxPatternParserNext(parser);
		value = parser->character;	
		if ((('a' <= value) && (value <= 'z')) || (('A' <= value) && (value <= 'Z'))) {
			result = fxCharSetSingle(parser, value % 32);
			fxPatternParserNext(parser);
		}
	} break;
	// null
	case '0': {
		fxPatternParserNext(parser);
		if ((parser->character < '0') || ('9' < parser->character))
			result = fxCharSetSingle(parser, 0);
	} break;

	
	case 'x': {
		txU4 value = 0;
		int i;
		fxPatternParserNext(parser);
		for (i = 0; i < 2; i++) {
			if (fxPatternParserHexadecimal(parser, &value))
				fxPatternParserNext(parser);
			else
				break;
		}
		if (i == 2)
			result = fxCharSetSingle(parser, value);
	} break;

	case 'u': {
		txU4 value = 0;
		fxPatternParserNext(parser);
		if (fxPatternParserUnicodeEscape(parser, &value))
			result = fxCharSetSingle(parser, value);
	} break;
	case '^':
	case '$':
	case '\\':
	case '.':
	case '*':
	case '+':
	case '?':
	case '(':
	case ')':
	case '[':
	case ']':
	case '{':
	case '}':
	case '|':
	case '/':
		result = fxCharSetSingle(parser, parser->character);
		fxPatternParserNext(parser);
		break;
	default:
		if (!(parser->flags & XS_REGEXP_U)) {
			result = fxCharSetSingle(parser, parser->character); // UnicodeIDContinue?
			fxPatternParserNext(parser);
		}
		break;
	}
	if (result == NULL)
		fxPatternParserError(parser, gxErrors[mxInvalidEscape]);
	return result;
}

void* fxCharSetParseItem(txPatternParser* parser)
{
	void* result = NULL;
	if (parser->character == '-') {
		result = fxCharSetSingle(parser, '-');
		fxPatternParserNext(parser);
	}
	else if (parser->character == '\\') {
		fxPatternParserNext(parser);
		if (parser->character == 'b') {
			fxPatternParserNext(parser);
			result = fxCharSetSingle(parser, 8);
		}
		else if (parser->character == '-') {
			fxPatternParserNext(parser);
			result = fxCharSetSingle(parser, '-');
		}
		else
			result = fxCharSetParseEscape(parser);
	}
	else if (parser->character == ']') {
		result = fxCharSetEmpty(parser);
	}	
	else {
		result = fxCharSetSingle(parser, parser->character);
		fxPatternParserNext(parser);
	}
	return result;
}

void* fxCharSetParseList(txPatternParser* parser)
{
	txBoolean not = 0;
	void* former = NULL;
	void* result = NULL;
	if (parser->character == '^') {
		fxPatternParserNext(parser);
		not = 1;
	}
	while (parser->character != C_EOF) {
		result = fxCharSetParseItem(parser);
		if (parser->character == '-') {
			fxPatternParserNext(parser);
			result = fxCharSetRange(parser, result, fxCharSetParseItem(parser));
		}
		else
			result = fxCharSetCanonicalizeSingle(parser, result);
		if (former)
			result = fxCharSetOr(parser, former, result);
		former = result;
		if (parser->character == ']')
			break;
	}
	if (not)
		result = fxCharSetNot(parser, result);
	return result;
}

void* fxCharSetRange(txPatternParser* parser, txCharSet* set1, txCharSet* set2)
{
	txCharSet* result;
	if (set1->characters[0] == 0)
		return set2;
	if (set2->characters[0] == 0)
		return set1;
	if ((set1->characters[0] != 2) || (set2->characters[0] != 2))
		fxPatternParserError(parser, gxErrors[mxInvalidRange]);
	if ((set1->characters[1] + 1 != set1->characters[2]) || (set2->characters[1] + 1 != set2->characters[2]))
		fxPatternParserError(parser, gxErrors[mxInvalidRange]);
	if ((set1->characters[1] > set2->characters[1]))
		fxPatternParserError(parser, gxErrors[mxInvalidRange]);
	if (parser->flags & XS_REGEXP_I) {
		txBoolean flag = (parser->flags & XS_REGEXP_U) ? 1 : 0, beginInside, endInside;
		txInteger begin = set1->characters[1];
		txInteger end = set2->characters[1];
		txCharCase* beginCase = fxCharCaseFind(begin, flag, &beginInside);
		txCharCase* endCase = fxCharCaseFind(end, flag, &endInside);
		if ((beginCase == endCase) && (beginInside == endInside)) {
			result = fxTermCreate(parser, sizeof(txCharSet) + (2 * sizeof(txInteger)), fxCharSetMeasure);
			result->characters[0] = 2;
			result->characters[1] = beginInside ? fxCharCaseCanonicalize(beginCase, begin, flag) : begin;
			result->characters[2] = endInside? fxCharCaseCanonicalize(endCase, end, flag) + 1 : end + 1;
		}
		else {
			struct {
				txTermPart;
				txInteger characters[3];
			} _set;
			txCharSet* set = (txCharSet*)&_set;
			set->next = C_NULL;
			set->dispatch.code = fxCharSetCode;
			set->characters[0] = 2;
			result = fxTermCreate(parser, sizeof(txCharSet) + (2 * sizeof(txInteger)), fxCharSetMeasure);
			result->characters[0] = 2;
			result->characters[1] = set1->characters[1];
			result->characters[2] = set2->characters[2];
			if (beginInside) {
				set->characters[1] = fxCharCaseCanonicalize(beginCase, begin, flag);
				set->characters[2] = fxCharCaseCanonicalize(beginCase, beginCase->to, flag) + 1;
				result = fxCharSetOr(parser, result, set);
				beginCase++;
			}
			while (beginCase < endCase) {
				set->characters[1] = fxCharCaseCanonicalize(beginCase, beginCase->from, flag);
				set->characters[2] = fxCharCaseCanonicalize(beginCase, beginCase->to, flag) + 1;
				result = fxCharSetOr(parser, result, set);
				beginCase++;
			}
			if (endInside) {
				set->characters[1] = fxCharCaseCanonicalize(beginCase, beginCase->from, flag);
				set->characters[2] = fxCharCaseCanonicalize(beginCase, end, flag) + 1;
				result = fxCharSetOr(parser, result, set);
			}
		}
	}
	else {
		result = fxTermCreate(parser, sizeof(txCharSet) + (2 * sizeof(txInteger)), fxCharSetMeasure);
		result->characters[0] = 2;
		result->characters[1] = set1->characters[1];
		result->characters[2] = set2->characters[2];
	}
	return result;
}

void* fxCharSetSingle(txPatternParser* parser, txInteger character)
{
	txCharSet* result = fxTermCreate(parser, sizeof(txCharSet) + (2 * sizeof(txInteger)), fxCharSetMeasure);
	result->characters[0] = 2;
	result->characters[1] = character;
	result->characters[2] = character + 1;
	return result;
}

void* fxCharSetSpaces(txPatternParser* parser)
{
	txCharSet* result = fxTermCreate(parser, sizeof(txCharSet) + (20 * sizeof(txInteger)), fxCharSetMeasure);
	result->characters[0] = 20;
	result->characters[1] = 0x0009;
	result->characters[2] = 0x000D + 1;
	result->characters[3] = 0x0020;
	result->characters[4] = 0x0020 + 1;
	result->characters[5] = 0x00A0;
	result->characters[6] = 0x00A0 + 1;
	result->characters[7] = 0x1680;
	result->characters[8] = 0x1680 + 1;
	result->characters[9] = 0x2000;
	result->characters[10] = 0x200A + 1;
	result->characters[11] = 0x2028;
	result->characters[12] = 0x2029 + 1;
	result->characters[13] = 0x202F;
	result->characters[14] = 0x202F + 1;
	result->characters[15] = 0x205F;
	result->characters[16] = 0x205F + 1;
	result->characters[17] = 0x3000;
	result->characters[18] = 0x3000 + 1;
	result->characters[19] = 0xFEFF;
	result->characters[20] = 0xFEFF + 1;
	return result;
}

void* fxCharSetWords(txPatternParser* parser)
{
	txCharSet* result;
	if (parser->flags & XS_REGEXP_I) {
		if (parser->flags & XS_REGEXP_U) {
			result = fxTermCreate(parser, sizeof(txCharSet) + (6 * sizeof(txInteger)), fxCharSetMeasure);
			result->characters[0] = 6;
			result->characters[1] = '0';
			result->characters[2] = '9' + 1;
			result->characters[3] = '_';
			result->characters[4] = '_' + 1;
			result->characters[5] = 'a';
			result->characters[6] = 'z' + 1;
		}
		else {
			result = fxTermCreate(parser, sizeof(txCharSet) + (6 * sizeof(txInteger)), fxCharSetMeasure);
			result->characters[0] = 6;
			result->characters[1] = '0';
			result->characters[2] = '9' + 1;
			result->characters[3] = 'A';
			result->characters[4] = 'Z' + 1;
			result->characters[5] = '_';
			result->characters[6] = '_' + 1;
		}
	}
	else {
		result = fxTermCreate(parser, sizeof(txCharSet) + (8 * sizeof(txInteger)), fxCharSetMeasure);
		result->characters[0] = 8;
		result->characters[1] = '0';
		result->characters[2] = '9' + 1;
		result->characters[3] = 'A';
		result->characters[4] = 'Z' + 1;
		result->characters[5] = '_';
		result->characters[6] = '_' + 1;
		result->characters[7] = 'a';
		result->characters[8] = 'z' + 1;
	}
	return result;
}

void* fxDisjunctionParse(txPatternParser* parser, txInteger character)
{
	txDisjunction* result = NULL;
	txTerm* left = NULL;
	txTerm* right = NULL;
	result = fxSequenceParse(parser, character);
	if (parser->character == '|') {
		fxPatternParserNext(parser);
		left = (txTerm*)result;
		right = fxDisjunctionParse(parser, character);
		result = fxTermCreate(parser, sizeof(txDisjunction), fxDisjunctionMeasure);
		result->left = left;
		result->right = right;
	}
	if (parser->character != character)
		fxPatternParserError(parser, gxErrors[mxInvalidSequence]);
	return result;
}

void* fxQuantifierParse(txPatternParser* parser, void* term, txInteger captureIndex)
{
	txInteger min, max;
	txBoolean greedy;
	txQuantifier* quantifier;
	switch (parser->character) {
	case '*':
		min = 0;
		max = 0x7FFFFFFF;
		fxPatternParserNext(parser);
		break;
	case '+':
		min = 1;
		max = 0x7FFFFFFF;
		fxPatternParserNext(parser);
		break;
	case '?':
		min = 0;
		max = 1;
		fxPatternParserNext(parser);
		break;
	case '{':
		fxPatternParserNext(parser);
		min = fxQuantifierParseDigits(parser);
		if (parser->character == ',') {
			fxPatternParserNext(parser);
			if (parser->character == '}')
				max = 0x7FFFFFFF;
			else
				max = fxQuantifierParseDigits(parser);
		}
		else
			max = min;
		if (parser->character != '}')
			fxPatternParserError(parser, gxErrors[mxInvalidQuantifier]);
		if (min > max)
			fxPatternParserError(parser, gxErrors[mxInvalidQuantifier]);
		fxPatternParserNext(parser);
		break;
	default:
		return term;
	}
	if (parser->character == '?') {
		greedy = 0;
		fxPatternParserNext(parser);
	}
	else
		greedy = 1;
	quantifier = fxTermCreate(parser, sizeof(txQuantifier), fxQuantifierMeasure);
	quantifier->term = term;
	quantifier->min = min;
	quantifier->max = max;
	quantifier->greedy = greedy;
	quantifier->captureIndex = captureIndex;
	quantifier->captureCount = parser->captureIndex - captureIndex;
	quantifier->quantifierIndex = parser->quantifierIndex++;
	return quantifier;
}

txInteger fxQuantifierParseDigits(txPatternParser* parser)
{
	txU4 value = 0;
	if (fxPatternParserDecimal(parser, &value)) {
		fxPatternParserNext(parser);
		while (fxPatternParserDecimal(parser, &value))
			fxPatternParserNext(parser);
	}
	else
		fxPatternParserError(parser, gxErrors[mxInvalidQuantifier]);
	if (value > 0x7FFFFFFF)
		value = 0x7FFFFFFF;
	return (txInteger)value;
}

void* fxSequenceParse(txPatternParser* parser, txInteger character)
{
	void* result = NULL;
	void* current = NULL;
	txSequence* currentBranch = NULL;
	void* former = NULL;
	txSequence* formerBranch = NULL;
	txInteger length;
	while ((parser->character != C_EOF) && (parser->character != character)) {
		txInteger currentIndex = parser->captureIndex;
		void* term = NULL;
		if (parser->character == '^') {
			fxPatternParserNext(parser);
			current = fxTermCreate(parser, sizeof(txAssertion), fxLineBeginMeasure);
		}
		else if (parser->character == '$') {
			fxPatternParserNext(parser);
			current = fxTermCreate(parser, sizeof(txAssertion), fxLineEndMeasure);
		}
		else if (parser->character == '\\') {
			fxPatternParserNext(parser);
			if (parser->character == 'b') {
				fxPatternParserNext(parser);
				current = fxTermCreate(parser, sizeof(txAssertion), fxWordBreakMeasure);
			}
			else if (parser->character == 'B') {
				fxPatternParserNext(parser);
				current = fxTermCreate(parser, sizeof(txAssertion), fxWordContinueMeasure);
			}
			else if ((parser->flags & (XS_REGEXP_U | XS_REGEXP_N)) && (parser->character == 'k')) {
				fxPatternParserNext(parser);
				if (parser->character != '<')
					fxPatternParserError(parser, gxErrors[mxInvalidName]);
				fxPatternParserNext(parser);
				fxPatternParserName(parser, &length);
				current = fxTermCreate(parser, sizeof(txCaptureReference) + length, fxCaptureReferenceMeasure);
				((txCaptureReference*)current)->captureIndex = -1;
				c_memcpy(((txCaptureReference*)current)->name, parser->error, length + 1);
				current = fxQuantifierParse(parser, current, currentIndex);
			}
			else if (('1' <= parser->character) && (parser->character <= '9')) {
				txU4 value = (txU4)(parser->character - '0');
				fxPatternParserNext(parser);
				while (fxPatternParserDecimal(parser, &value))
					fxPatternParserNext(parser);
				current = fxTermCreate(parser, sizeof(txCaptureReference), fxCaptureReferenceMeasure);
				((txCaptureReference*)current)->captureIndex = value;
				current = fxQuantifierParse(parser, current, currentIndex);
			}
			else {
				current = fxCharSetCanonicalizeSingle(parser, fxCharSetParseEscape(parser));
				current = fxQuantifierParse(parser, current, currentIndex);
			}
		}
		else if (parser->character == '.') {
			current = fxCharSetAny(parser);
			fxPatternParserNext(parser);
			current = fxQuantifierParse(parser, current, currentIndex);
		}
		else if (parser->character == '*') {
			fxPatternParserError(parser, gxErrors[mxInvalidCharacter]);
		}
		else if (parser->character == '+') {
			fxPatternParserError(parser, gxErrors[mxInvalidCharacter]);
		}
		else if (parser->character == '?') {
			fxPatternParserError(parser, gxErrors[mxInvalidCharacter]);
		}
		else if (parser->character == '(') {
			fxPatternParserNext(parser);
			if (parser->character == '?') {
				fxPatternParserNext(parser);
				if (parser->character == '=') {
					fxPatternParserNext(parser);
					term = fxDisjunctionParse(parser, ')');
					fxPatternParserNext(parser);
					current = fxTermCreate(parser, sizeof(txAssertion), fxAssertionMeasure);
					((txAssertion*)current)->term = term;
					((txAssertion*)current)->not = 0;
					((txAssertion*)current)->direction = 1;
					((txAssertion*)current)->assertionIndex = parser->assertionIndex++;
				}
				else if (parser->character == '!') {
					fxPatternParserNext(parser);
					term = fxDisjunctionParse(parser, ')');
					fxPatternParserNext(parser);
					current = fxTermCreate(parser, sizeof(txAssertion), fxAssertionMeasure);
					((txAssertion*)current)->term = term;
					((txAssertion*)current)->not = 1;
					((txAssertion*)current)->direction = 1;
					((txAssertion*)current)->assertionIndex = parser->assertionIndex++;
				}
				else if (parser->character == ':') {
					fxPatternParserNext(parser);
					current = fxDisjunctionParse(parser, ')');
					fxPatternParserNext(parser);
					current = fxQuantifierParse(parser, current, currentIndex);
				}
				else if (parser->character == '<') {
					fxPatternParserNext(parser);
					if (parser->character == '=') {
						fxPatternParserNext(parser);
						term = fxDisjunctionParse(parser, ')');
						fxPatternParserNext(parser);
						current = fxTermCreate(parser, sizeof(txAssertion), fxAssertionMeasure);
						((txAssertion*)current)->term = term;
						((txAssertion*)current)->not = 0;
						((txAssertion*)current)->direction = -1;
						((txAssertion*)current)->assertionIndex = parser->assertionIndex++;
					}
					else if (parser->character == '!') {
						fxPatternParserNext(parser);
						term = fxDisjunctionParse(parser, ')');
						fxPatternParserNext(parser);
						current = fxTermCreate(parser, sizeof(txAssertion), fxAssertionMeasure);
						((txAssertion*)current)->term = term;
						((txAssertion*)current)->not = 1;
						((txAssertion*)current)->direction = -1;
						((txAssertion*)current)->assertionIndex = parser->assertionIndex++;
					}
					else {
						parser->captureIndex++;
						currentIndex++;
						fxPatternParserName(parser, &length);
						current = fxTermCreate(parser, sizeof(txCapture) + length, fxCaptureMeasure);
						((txCapture*)current)->captureIndex = currentIndex;
						c_memcpy(((txCapture*)current)->name, parser->error, length + 1);
						fxPatternParserNamedCapture(parser, (txCapture*)current);
						((txCapture*)current)->term = fxDisjunctionParse(parser, ')');	
						fxPatternParserNext(parser);
						current = fxQuantifierParse(parser, current, currentIndex - 1);
					}
				}
				else {
					fxPatternParserError(parser, gxErrors[mxInvalidGroup]);
				}
			}
			else {
				parser->captureIndex++;
				currentIndex++;
				term = fxDisjunctionParse(parser, ')');
				fxPatternParserNext(parser);
				current = fxTermCreate(parser, sizeof(txCapture), fxCaptureMeasure);
				((txCapture*)current)->term = term;
				((txCapture*)current)->captureIndex = currentIndex;
				current = fxQuantifierParse(parser, current, currentIndex - 1);
			}
		}
		else if (parser->character == ')') {
			fxPatternParserError(parser, gxErrors[mxInvalidCharacter]);
		}
		else if (parser->character == '[') {
			fxPatternParserNext(parser);
			current = fxCharSetParseList(parser);
			if (parser->character != ']')
				fxPatternParserError(parser, gxErrors[mxInvalidRange]);
			fxPatternParserNext(parser);
			current = fxQuantifierParse(parser, current, currentIndex);
		}
		else if (parser->character == ']') {
			fxPatternParserError(parser, gxErrors[mxInvalidCharacter]);
		}
		else if (parser->character == '{') {
			fxPatternParserError(parser, gxErrors[mxInvalidCharacter]);
		}
		else if (parser->character == '}') {
			fxPatternParserError(parser, gxErrors[mxInvalidCharacter]);
		}
		else if (parser->character == '|') {
			break;
		}
		else {
			current = fxCharSetCanonicalizeSingle(parser, fxCharSetSingle(parser, parser->character));
			fxPatternParserNext(parser);
			current = fxQuantifierParse(parser, current, currentIndex);
		}
		
		if (former) {
			currentBranch = fxTermCreate(parser, sizeof(txSequence), fxSequenceMeasure);
			currentBranch->left = former;
			currentBranch->right = current;
			if (formerBranch)
				formerBranch->right = (txTerm*)currentBranch;
			else
				result = currentBranch;
			formerBranch = currentBranch;	
		}
		else
			result = current;
		former = current;
	}
	if (result == NULL)
		result = fxTermCreate(parser, sizeof(txTerm), fxEmptyMeasure);
	return result;
}

void* fxTermCreate(txPatternParser* parser, size_t size, txTermMeasure measure)
{
	txTerm* term = c_malloc(size);
	if (!term)
		fxPatternParserError(parser, gxErrors[mxNotEnoughMemory]);
	term->next = parser->first;
	term->dispatch.measure = measure;
	parser->first = term;
	return term;
}
	
void fxAssertionMeasure(txPatternParser* parser, void* it, txInteger direction)
{
	txAssertion* self = it;
	self->step = parser->size;
	if (self->not)
		parser->size += mxAssertionNotStepSize;
	else		
		parser->size += mxAssertionStepSize;
	(*self->term->dispatch.measure)(parser, self->term, self->direction);
	self->completion = parser->size;
	if (self->not)
		parser->size += mxAssertionNotCompletionSize;
	else		
		parser->size += mxAssertionCompletionSize;
	self->dispatch.code = fxAssertionCode;
}

void fxCaptureMeasure(txPatternParser* parser, void* it, txInteger direction)
{
	txCapture* self = it;
	self->step = parser->size;
	parser->size += mxCaptureStepSize;
	(*self->term->dispatch.measure)(parser, self->term, direction);
	self->completion = parser->size;
	parser->size += mxCaptureCompletionSize;
	self->dispatch.code = fxCaptureCode;
}

void fxCaptureReferenceMeasure(txPatternParser* parser, void* it, txInteger direction)
{
	txCaptureReference* self = it;
	if (self->captureIndex < 0) {
		txCapture* capture = parser->firstNamedCapture;
		while (capture) {
			if (!c_strcmp(self->name, capture->name))
				break;
			capture = capture->nextNamedCapture;
		}
		if (capture)
			self->captureIndex = capture->captureIndex;
		else
			fxPatternParserError(parser, gxErrors[mxInvalidReferenceName], self->name);
	}
	else if (self->captureIndex >= parser->captureIndex)
		fxPatternParserError(parser, gxErrors[mxInvalidReferenceNumber], self->captureIndex);
	self->step = parser->size;
	parser->size += mxCaptureReferenceStepSize;
	self->dispatch.code = fxCaptureReferenceCode;
}

void fxCharSetMeasure(txPatternParser* parser, void* it, txInteger direction)
{
	txCharSet* self = it;
	self->step = parser->size;
	parser->size += mxTermStepSize + ((1 + self->characters[0]) * sizeof(txInteger));
	self->dispatch.code = fxCharSetCode;
}

void fxDisjunctionMeasure(txPatternParser* parser, void* it, txInteger direction)
{
	txDisjunction* self = it;
	self->step = parser->size;
	parser->size += mxDisjunctionStepSize;
	(*self->left->dispatch.measure)(parser, self->left, direction);
	(*self->right->dispatch.measure)(parser, self->right, direction);
	self->dispatch.code = fxDisjunctionCode;
}
	
void fxEmptyMeasure(txPatternParser* parser, void* it, txInteger direction)
{
	txTerm* self = it;
	self->step = parser->size;
	parser->size += mxTermStepSize;
	self->dispatch.code = fxEmptyCode;
}

void fxLineBeginMeasure(txPatternParser* parser, void* it, txInteger direction)
{
	txTerm* self = it;
	self->step = parser->size;
	parser->size += mxTermStepSize;
	self->dispatch.code = fxLineBeginCode;
}

void fxLineEndMeasure(txPatternParser* parser, void* it, txInteger direction)
{
	txTerm* self = it;
	self->step = parser->size;
	parser->size += mxTermStepSize;
	self->dispatch.code = fxLineEndCode;
}

void fxQuantifierMeasure(txPatternParser* parser, void* it, txInteger direction)
{
	txQuantifier* self = it;
	self->step = parser->size;
	parser->size += mxQuantifierStepSize;
	self->loop = parser->size;
	parser->size += mxQuantifierLoopSize;
	(*self->term->dispatch.measure)(parser, self->term, direction);
	self->completion = parser->size;
	parser->size += mxQuantifierCompletionSize;
	self->dispatch.code = fxQuantifierCode;
}

void fxSequenceMeasure(txPatternParser* parser, void* it, txInteger direction)
{
	txSequence* self = it;
	if (direction == 1) {
		(*self->left->dispatch.measure)(parser, self->left, direction);
		self->step = self->left->step;
		(*self->right->dispatch.measure)(parser, self->right, direction);
	}
	else {
		(*self->right->dispatch.measure)(parser, self->right, direction);
		self->step = self->right->step;
		(*self->left->dispatch.measure)(parser, self->left, direction);
	}
	self->dispatch.code = fxSequenceCode;
}

void fxWordBreakMeasure(txPatternParser* parser, void* it, txInteger direction)
{
	txTerm* self = it;
	self->step = parser->size;
	parser->size += mxTermStepSize;
	self->dispatch.code = fxWordBreakCode;
}

void fxWordContinueMeasure(txPatternParser* parser, void* it, txInteger direction)
{
	txTerm* self = it;
	self->step = parser->size;
	parser->size += mxTermStepSize;
	self->dispatch.code = fxWordContinueCode;
}
	
void fxAssertionCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel)
{
	txAssertion* self = it;
	txInteger* buffer = (txInteger*)(((txByte*)*(parser->code)) + self->step);
	if (self->not) {
		*buffer++ = cxAssertionNotStep;
		*buffer++ = self->term->step;
		*buffer++ = self->assertionIndex;
		*buffer++ = sequel;
	}
	else {
		*buffer++ = cxAssertionStep;
		*buffer++ = self->term->step;
		*buffer++ = self->assertionIndex;
	}
	(*self->term->dispatch.code)(parser, self->term, self->direction, self->completion);
	buffer = (txInteger*)(((txByte*)*(parser->code)) + self->completion);
	if (self->not) {
		*buffer++ = cxAssertionNotCompletion;
		*buffer++ = self->assertionIndex;
	}
	else {
		*buffer++ = cxAssertionCompletion;
		*buffer++ = sequel;
		*buffer++ = self->assertionIndex;
	}
}

void fxCaptureCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel)
{
	txCapture* self = it;
	txInteger* buffer = (txInteger*)(((txByte*)*(parser->code)) + self->step);
	if (direction == 1)
		*buffer++ = cxCaptureForwardStep;
	else
		*buffer++ = cxCaptureBackwardStep;
	*buffer++ = self->term->step;
	*buffer++ = self->captureIndex;
	(*self->term->dispatch.code)(parser, self->term, direction, self->completion);
	buffer = (txInteger*)(((txByte*)*(parser->code)) + self->completion);
	if (direction == 1)
		*buffer++ = cxCaptureForwardCompletion;
	else
		*buffer++ = cxCaptureBackwardCompletion;
	*buffer++ = sequel;
	*buffer++ = self->captureIndex;
#ifdef mxRun
	if (parser->the && self->name[0])
		(*parser->code)[2 + self->captureIndex] = fxNewNameC(parser->the, self->name);
	else
#endif
		(*parser->code)[2 + self->captureIndex] = XS_NO_ID;
}

void fxCaptureReferenceCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel)
{
	txCaptureReference* self = it;
	txInteger* buffer = (txInteger*)(((txByte*)*(parser->code)) + self->step);
	if (direction == 1)
		*buffer++ = cxCaptureReferenceForwardStep;
	else
		*buffer++ = cxCaptureReferenceBackwardStep;
	*buffer++ = sequel;
	*buffer++ = self->captureIndex;
}

void fxCharSetCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel)
{
	txCharSet* self = it;
	txInteger* buffer = (txInteger*)(((txByte*)*(parser->code)) + self->step);
	txInteger count, index;
	if (direction == 1)
		*buffer++ = cxCharSetForwardStep;
	else
		*buffer++ = cxCharSetBackwardStep;
	*buffer++ = sequel;
	count = *buffer++ = self->characters[0];
	index = 1;
	while (index <= count) {
		*buffer++ = self->characters[index++];
	}
}

void fxDisjunctionCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel)
{
	txDisjunction* self = it;
	txInteger* buffer = (txInteger*)(((txByte*)*(parser->code)) + self->step);
	*buffer++ = cxDisjunctionStep;
	*buffer++ = self->left->step;
	*buffer++ = self->right->step;
	(*self->left->dispatch.code)(parser, self->left, direction, sequel);
	(*self->right->dispatch.code)(parser, self->right, direction, sequel);
}
	
void fxEmptyCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel)
{
	txTerm* self = it;
	txInteger* buffer = (txInteger*)(((txByte*)*(parser->code)) + self->step);
	*buffer++ = cxEmptyStep;
	*buffer++ = sequel;
}

void fxLineBeginCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel)
{
	txTerm* self = it;
	txInteger* buffer = (txInteger*)(((txByte*)*(parser->code)) + self->step);
	*buffer++ = cxLineBeginStep;
	*buffer++ = sequel;
}

void fxLineEndCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel)
{
	txTerm* self = it;
	txInteger* buffer = (txInteger*)(((txByte*)*(parser->code)) + self->step);
	*buffer++ = cxLineEndStep;
	*buffer++ = sequel;
}

void fxQuantifierCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel)
{
	txQuantifier* self = it;
	txInteger* buffer = (txInteger*)(((txByte*)*(parser->code)) + self->step);
	*buffer++ = cxQuantifierStep;
	*buffer++ = self->loop;
	*buffer++ = self->quantifierIndex;
	*buffer++ = self->min;
	*buffer++ = self->max;
	buffer = (txInteger*)(((txByte*)*(parser->code)) + self->loop);
	if (self->greedy)
		*buffer++ = cxQuantifierGreedyLoop;
	else
		*buffer++ = cxQuantifierLazyLoop;
	*buffer++ = self->term->step;
	*buffer++ = self->quantifierIndex;
	*buffer++ = sequel;
	*buffer++ = self->captureIndex + 1;
	*buffer++ = self->captureIndex + self->captureCount;
	(*self->term->dispatch.code)(parser, self->term, direction, self->completion);
	buffer = (txInteger*)(((txByte*)*(parser->code)) + self->completion);
	*buffer++ = cxQuantifierCompletion;
	*buffer++ = self->loop;
	*buffer++ = self->quantifierIndex;
	*buffer++ = sequel;
}

void fxSequenceCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel)
{
	txSequence* self = it;
	if (direction == 1) {
		(*self->left->dispatch.code)(parser, self->left, direction, self->right->step);
		(*self->right->dispatch.code)(parser, self->right, direction, sequel);
	}
	else {
		(*self->right->dispatch.code)(parser, self->right, direction, self->left->step);
		(*self->left->dispatch.code)(parser, self->left, direction, sequel);
	}
}

void fxWordBreakCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel)
{
	txTerm* self = it;
	txInteger* buffer = (txInteger*)(((txByte*)*(parser->code)) + self->step);
	*buffer++ = cxWordBreakStep;
	*buffer++ = sequel;
}

void fxWordContinueCode(txPatternParser* parser, void* it, txInteger direction, txInteger sequel)
{
	txTerm* self = it;
	txInteger* buffer = (txInteger*)(((txByte*)*(parser->code)) + self->step);
	*buffer++ = cxWordContinueStep;
	*buffer++ = sequel;
}


void fxPatternParserInitialize(txPatternParser* parser)
{
	c_memset(parser, 0, sizeof(txPatternParser));
}

txBoolean fxPatternParserDecimal(txPatternParser* parser, txU4* value)
{
	txInteger c = parser->character;
	if (('0' <= c) && (c <= '9'))
		*value = (*value * 10) + (c - '0');
	else
		return 0;
	return 1;
}

void fxPatternParserError(txPatternParser* parser, txString format, ...)
{
	c_va_list arguments;
	txU1* pattern = parser->pattern;
	txString error = parser->error;
	txInteger offset = parser->offset;
	while (offset) {
		*error++ = c_read8(pattern++);
		offset--;
	}
	*error++ = ' ';
	c_va_start(arguments, format);
	vsnprintf(error, sizeof(parser->error) - (error - parser->error), format, arguments);
	c_va_end(arguments);
	c_longjmp(parser->jmp_buf, 1);
}

txBoolean fxPatternParserHexadecimal(txPatternParser* parser, txU4* value)
{
	txInteger c = parser->character;
	if (('0' <= c) && (c <= '9'))
		*value = (*value * 16) + (c - '0');
	else if (('a' <= c) && (c <= 'f'))
		*value = (*value * 16) + (10 + c - 'a');
	else if (('A' <= c) && (c <= 'F'))
		*value = (*value * 16) + (10 + c - 'A');
	else
		return 0;
	return 1;
}

void fxPatternParserName(txPatternParser* parser, txInteger* length)
{
	txU4 character;
	txString p = parser->error;
	txString q = p + 255;
	if (fxIsIdentifierFirst(parser->character)) {
		p = (txString)fsX2UTF8(parser->character, (txU1*)p, q - p);				
		fxPatternParserNext(parser);
	}
	else if (parser->character == '\\') {
		fxPatternParserNext(parser);
		if (parser->character != 'u')
			fxPatternParserError(parser, gxErrors[mxInvalidName]);			
		fxPatternParserNext(parser);
		if (!fxPatternParserUnicodeEscape(parser, &character))
			fxPatternParserError(parser, gxErrors[mxInvalidName]);			
		if (!fxIsIdentifierFirst(character))
			fxPatternParserError(parser, gxErrors[mxInvalidName]);			
		p = (txString)fsX2UTF8(character, (txU1*)p, q - p);				
	}
	else
		fxPatternParserError(parser, gxErrors[mxInvalidName]);			
	while (parser->character != '>') {
		if (p == q) {
			fxPatternParserError(parser, gxErrors[mxNameOverflow]);			
			break;
		}
		if (fxIsIdentifierNext(parser->character)) {
			p = (txString)fsX2UTF8(parser->character, (txU1*)p, q - p);				
			fxPatternParserNext(parser);
		}
		else if (parser->character == '\\') {
			fxPatternParserNext(parser);
			if (parser->character != 'u')
				fxPatternParserError(parser, gxErrors[mxInvalidName]);			
			fxPatternParserNext(parser);
			if (!fxPatternParserUnicodeEscape(parser, &character))
				fxPatternParserError(parser, gxErrors[mxInvalidName]);			
			if (!fxIsIdentifierNext(character))
				fxPatternParserError(parser, gxErrors[mxInvalidName]);			
			p = (txString)fsX2UTF8(character, (txU1*)p, q - p);				
		}
		else
			fxPatternParserError(parser, gxErrors[mxInvalidName]);			
	}
	fxPatternParserNext(parser);
	*p = 0;
	*length = p - parser->error;
}

void fxPatternParserNamedCapture(txPatternParser* parser, txCapture* capture)
{
	txCapture* check = parser->firstNamedCapture;
	while (check) {
		if (!strcmp(check->name, capture->name))
			fxPatternParserError(parser, gxErrors[mxDuplicateCapture]);
		check = check->nextNamedCapture;
	}
	capture->nextNamedCapture = parser->firstNamedCapture;
	parser->firstNamedCapture = capture;
}

void fxPatternParserNext(txPatternParser* parser)
{
	txU1* p = parser->pattern + parser->offset;
	txU4 character;
	txUTF8Sequence const *aSequence = NULL;
	txInteger aSize;
	
	if (parser->surrogate) {
		parser->character = parser->surrogate;
		parser->surrogate = 0;
	}
	else {
		character = c_read8(p++);
		if (character) {
			for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
				if ((character & aSequence->cmask) == aSequence->cval)
					break;
			}
			if (aSequence->size == 0)
				fxPatternParserError(parser, gxErrors[mxInvalidUTF8]);
			aSize = aSequence->size - 1;
			while (aSize) {
				aSize--;
				character = (character << 6) | (c_read8(p++) & 0x3F);
			}
			character &= aSequence->lmask;
			parser->offset = p - parser->pattern;
			if (character == 0x110000)
				character = 0;
			if (!(parser->flags & XS_REGEXP_U) && (character > 0xFFFF)) {
				character -= 0x10000;
				parser->surrogate = 0xDC00 | (character & 0x3FF);
				character = 0xD800 | (character >> 10);
			}
			parser->character = (txInteger)character;
		}
		else
			parser->character = C_EOF;
	}
}

void fxPatternParserTerminate(txPatternParser* parser)
{
	txTerm* term = parser->first;
	while (term) {
		txTerm* next = term->next;
		c_free(term);
		term = next;
	}
	parser->first = NULL;
}

txBoolean fxPatternParserUnicodeEscape(txPatternParser* parser, txU4* character)
{
	txBoolean result = 0;
	int i;
	txU4 value = 0;
	if ((parser->flags & XS_REGEXP_U) && (parser->character == '{')) {
		fxPatternParserNext(parser);
		for (i = 0; value < 0x00110000; i++) {
			if (fxPatternParserHexadecimal(parser, &value))
				fxPatternParserNext(parser);
			else
				break;
		}
		if ((parser->character == '}') && (i > 0) && (value < 0x00110000)) {
			*character = value;
			fxPatternParserNext(parser);
			result = 1;
		}
	}
	else {
		for (i = 0; i < 4; ++i) {
			if (fxPatternParserHexadecimal(parser, &value))
				fxPatternParserNext(parser);
			else
				break;
		}
		if (i == 4) {
			if ((parser->flags & XS_REGEXP_U) && (0x0000D800 <= value) && (value < 0x0000DBFF)) {
				if (parser->character == '\\') {
					txInteger offset = parser->offset;
					fxPatternParserNext(parser);
					if (parser->character == 'u') {
						txU4 trail = 0;
						fxPatternParserNext(parser);
						for (i = 0; i < 4; ++i) {
							if (fxPatternParserHexadecimal(parser, &trail))
								fxPatternParserNext(parser);
							else
								break;
						}
						if (i == 4) {
							if ((0x0000DC00 <= trail) && (trail <= 0x0000DFFF))
								value = 0x00010000 + ((value & 0x03FF) << 10) + (trail & 0x03FF);
							else
								parser->offset = offset;
						}
						else
							parser->offset = offset;
					}
					else
						parser->offset = offset;
				}
			}
			*character = value;
			result = 1;
		}
	}
	return result;
}

txBoolean fxCompileRegExp(void* the, txString pattern, txString modifier, txInteger** code, txInteger** data, txString messageBuffer, txInteger messageSize)
{
	txBoolean result = 1;
	txPatternParser _parser;
	txPatternParser* parser = &_parser;
	txTerm* term;

	fxPatternParserInitialize(parser);
	if (c_setjmp(parser->jmp_buf) == 0) {
		char c;
		while ((c = c_read8(modifier++))) {
			if ((c == 'g') && !(parser->flags & XS_REGEXP_G))
				parser->flags |= XS_REGEXP_G;
			else if ((c == 'i') && !(parser->flags & XS_REGEXP_I))
				parser->flags |= XS_REGEXP_I;
			else if ((c == 'm') && !(parser->flags & XS_REGEXP_M))
				parser->flags |= XS_REGEXP_M;
			else if ((c == 's') && !(parser->flags & XS_REGEXP_S))
				parser->flags |= XS_REGEXP_S;
			else if ((c == 'u') && !(parser->flags & XS_REGEXP_U))
				parser->flags |= XS_REGEXP_U;
			else if ((c == 'y') && !(parser->flags & XS_REGEXP_Y))
				parser->flags |= XS_REGEXP_Y;
			else
				break;
		}
		if (c)
			fxPatternParserError(parser, gxErrors[mxInvalidFlags]);
		parser->pattern = (txU1*)pattern;
		parser->the = the;
		
		fxPatternParserNext(parser);
		term = fxDisjunctionParse(parser, C_EOF);
		if (parser->firstNamedCapture)
			parser->flags |= XS_REGEXP_N;
		if (!(parser->flags & XS_REGEXP_U) && (parser->flags & XS_REGEXP_N)) {
			fxPatternParserTerminate(parser);
			parser->offset = 0;
			parser->surrogate = 0;
			parser->assertionIndex = 0;
			parser->captureIndex = 0;
			parser->quantifierIndex = 0;
			parser->firstNamedCapture = NULL;
			fxPatternParserNext(parser);
			term = fxDisjunctionParse(parser, C_EOF);
		}	
		parser->captureIndex++;
		if (!term) 
			fxPatternParserError(parser, gxErrors[mxInvalidPattern]);
		parser->size = (4 + parser->captureIndex) * sizeof(txInteger);
		(*term->dispatch.measure)(parser, term, 1);
			
		if (data) {
			txInteger size = parser->captureIndex * sizeof(txCaptureData)
					+ parser->assertionIndex * sizeof(txAssertionData)
					+ parser->quantifierIndex * sizeof(txQuantifierData);
		#ifdef mxRun
			if (the)
				*data = fxNewChunk(the, size);
			else
		#endif
				*data = c_malloc(size);
			if (!*data)
				fxPatternParserError(parser, gxErrors[mxNotEnoughMemory]);
		}
		if (code) {
			txInteger offset;
			txInteger* buffer;
			offset = parser->size;
			parser->size += sizeof(txInteger);
		#ifdef mxRun
			if (the) {
				*code = fxNewChunk(the, parser->size);
			}
			else
		#endif
				*code = c_malloc(parser->size);
			if (!*code)
				fxPatternParserError(parser, gxErrors[mxNotEnoughMemory]);
			parser->code = code;
			buffer = *code;
			buffer[0] = parser->flags;
			buffer[1] = parser->captureIndex;
			buffer[2 + parser->captureIndex] = parser->assertionIndex;
			buffer[2 + parser->captureIndex + 1] = parser->quantifierIndex;
			(*term->dispatch.code)(parser, term, 1, offset);
			buffer = (txInteger*)(((txByte*)*code) + offset);
			*buffer = cxMatchStep;
		}
	}
	else {
		if (messageBuffer) {
			c_strncpy(messageBuffer, parser->error, messageSize - 1);
			messageBuffer[messageSize - 1] = 0;
		}
		result = 0;
		
	}
	fxPatternParserTerminate(parser);
	return result;
}

void fxDeleteRegExp(void* the, txInteger* code, txInteger* data)
{
	if (!the) {
		if (code)
			c_free(code);
		if (data)
			c_free(data);
	}
}

// MATCH

txInteger fxFindCharacter(txString input, txInteger offset, txInteger direction)
{
	txU1* p = (txU1*)input + offset;
	txU1 c;
	p += direction;
	while ((c = c_read8(p))) {
		if ((c & 0xC0) != 0x80)
			break;
		p += direction;
	}
	return p - (txU1*)input;
}

txInteger fxGetCharacter(txString input, txInteger offset, txInteger flags)
{
	txInteger character = fxUnicodeCharacter(input + offset);
	if (character == 0x110000)
		character = 0;
	if (flags & XS_REGEXP_I) {
		txBoolean flag = (flags & XS_REGEXP_U) ? 1 : 0, inside;
		txCharCase* current = fxCharCaseFind(character, flag, &inside);
		if (inside)
			character = fxCharCaseCanonicalize(current, character, flag);
	}
	return character;
}

txBoolean fxMatchCharacter(txInteger* characters, txInteger character)
{
	txInteger* current1 = characters + 1;
	txInteger* limit1 = current1 + characters[0];
	while (current1 < limit1) {
		txInteger begin = *current1++;
		txInteger end = *current1++;
		#ifdef mxTrace
			fprintf(stderr, " ");
			if ((32 <= begin) && (begin < 128))
				fprintf(stderr, "%c", begin);
			else
				fprintf(stderr, "%4.4X", begin);
			if (begin != (end - 1)) {
				fprintf(stderr, "-");
				if ((32 <= (end - 1)) && ((end - 1) < 128))
					fprintf(stderr, "%c", (end - 1));
				else
					fprintf(stderr, "%4.4X", (end - 1));
			}
		#endif
		if ((begin <= character) && (character < end))
			return 1;
	}
	return 0;
}

txStateData* fxPopStates(txMachine* the, txStateData* fromState, txStateData* toState)
{
	txStateData* state = fromState;
	while (state != toState) {
		fromState = state->nextState;
		if (!state->the)
			c_free(state);
		state = fromState;
	}
	return toState;
}

txStateData* fxPushState(txMachine* the, txStateData* firstState, txInteger step, txInteger offset, txCaptureData* captures, txInteger captureCount)
{
	txInteger size = sizeof(txStateData) + ((captureCount - 1) * sizeof(txCaptureData));
	txStateData* state = C_NULL;
	if (the && ((firstState == C_NULL) || (firstState->the != C_NULL))) {
		txByte* current = (txByte*)firstState;
		if (current)
			current += size;
		else
			current = (txByte*)(the->stackBottom);
		if ((current + size) < (txByte*)(the->stack)) {
			state = (txStateData*)current;
			state->the = the;
		}
	}
	if (!state) {
		state = c_malloc(size);
		if (!state) {
			fxPopStates(the, firstState, C_NULL);
			return C_NULL;
		}
		state->the = C_NULL;
	}	
	state->nextState = firstState;
	state->step = step;
	state->offset = offset;
	c_memcpy(state->captures, captures, captureCount * sizeof(txCaptureData));
	return state;
}

#if defined(__GNUC__)
	#define mxBreak continue
	#define mxCase(WHICH) WHICH
	#define mxSwitch(WHICH) goto *steps[WHICH];
#else
	#define mxBreak \
		break
	#define mxCase(WHICH) \
		case WHICH
	#define mxSwitch(WHICH) \
		switch(WHICH)
#endif

txBoolean fxMatchRegExp(void* the, txInteger* code, txInteger* data, txString subject, txInteger start)
{
#if defined(__GNUC__)
	static void *const ICACHE_RAM_ATTR gxSteps[] = {
		&&cxMatchStep,
		&&cxAssertionStep,
		&&cxAssertionCompletion,
		&&cxAssertionNotStep,
		&&cxAssertionNotCompletion,
		&&cxCaptureForwardStep,
		&&cxCaptureForwardCompletion,
		&&cxCaptureBackwardStep,
		&&cxCaptureBackwardCompletion,
		&&cxCaptureReferenceForwardStep,
		&&cxCaptureReferenceBackwardStep,
		&&cxCharSetForwardStep,
		&&cxCharSetBackwardStep,
		&&cxDisjunctionStep,
		&&cxEmptyStep,
		&&cxLineBeginStep,
		&&cxLineEndStep,
		&&cxQuantifierStep,
		&&cxQuantifierGreedyLoop,
		&&cxQuantifierLazyLoop,
		&&cxQuantifierCompletion,
		&&cxWordBreakStep,
		&&cxWordContinueStep
	};
	register void * const *steps = gxSteps;
#endif
	txInteger stop = c_strlen(subject);
	txInteger flags = code[0];
	txCaptureData* captures = (txCaptureData*)data;
	txCaptureData* capture;
	txInteger captureCount = code[1];
	txAssertionData* assertions = (txAssertionData*)(captures + captureCount);
	txAssertionData* assertion;
	txQuantifierData* quantifiers = (txQuantifierData*)(assertions + code[2 + captureCount]);
	txQuantifierData* quantifier;
	txStateData* firstState = C_NULL;
	txInteger from, to, e, f, g;
	txBoolean result = 0;
	
	while (!result && (0 <= start) && (start <= stop)) {
		txInteger step = (2 + captureCount + 2) * sizeof(txInteger), sequel;
		txInteger offset = start;
		c_memset(captures, -1, captureCount * sizeof(txCaptureData));
		while (step) {
			txInteger* pointer = (txInteger*)(((txByte*)code) + step);
			txInteger which = *pointer++;
			#ifdef mxTrace 
			{
				txInteger captureIndex;
	 			fprintf(stderr, "\n[%d,%d]", start, offset);
	 			for (captureIndex = 1; captureIndex < captureCount; captureIndex++) 
		 			fprintf(stderr, " [%d,%d]", captures[captureIndex].from, captures[captureIndex].to);
		 		fprintf(stderr, " %s", gxStepNames[which]);
 			}
			#endif
			mxSwitch(which) {
				mxCase(cxMatchStep):
					capture = captures;
					capture->from = start;
					capture->to = offset;
					step = 0;
					result = 1;
					mxBreak;
				mxCase(cxAssertionStep):
					step = *pointer++;
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					assertion = assertions + *pointer;
					assertion->offset = offset;
					assertion->firstState = firstState;
					mxBreak;
				mxCase(cxAssertionCompletion):
					step = *pointer++;
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					assertion = assertions + *pointer;
					offset = assertion->offset;
					firstState = fxPopStates(the, firstState, assertion->firstState);
					mxBreak;
				mxCase(cxAssertionNotStep):
					step = *pointer++;
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					assertion = assertions + *pointer++;
					assertion->offset = offset;
					assertion->firstState = firstState;
					sequel = *pointer;
					firstState = fxPushState(the, firstState, sequel, offset, captures, captureCount);
					if (!firstState)
						return 0;
					mxBreak;
				mxCase(cxAssertionNotCompletion):
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					assertion = assertions + *pointer;
					offset = assertion->offset;
					firstState = fxPopStates(the, firstState, assertion->firstState);
					goto mxPopState;
				mxCase(cxCaptureBackwardStep):
					step = *pointer++;
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					capture = captures + *pointer;
					capture->to = offset;
					mxBreak;
				mxCase(cxCaptureBackwardCompletion):
					step = *pointer++;
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					capture = captures + *pointer;
					capture->from = offset;
					mxBreak;
				mxCase(cxCaptureForwardStep):
					step = *pointer++;
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					capture = captures + *pointer;
					capture->from = offset;
					mxBreak;
				mxCase(cxCaptureForwardCompletion):
					step = *pointer++;
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					capture = captures + *pointer;
					capture->to = offset;
					mxBreak;
				mxCase(cxCaptureReferenceBackwardStep):
					step = *pointer++;
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					capture = captures + *pointer;
					from = capture->from;
					to = capture->to;
					if ((from >= 0) && (to >= 0)) {
						e = offset;
						f = e - (to - from);
						if (f < 0)
							goto mxPopState;
						g = f;
						while (from < to) {
							if (fxGetCharacter(subject, g, flags) != fxGetCharacter(subject, from, flags))
								goto mxPopState;
							g = fxFindCharacter(subject, g, 1);
							from = fxFindCharacter(subject, from, 1);
						}
						offset = f;
					}
					mxBreak;
				mxCase(cxCaptureReferenceForwardStep):
					step = *pointer++;
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					capture = captures + *pointer;
					from = capture->from;
					to = capture->to;
					if ((from >= 0) && (to >= 0)) {
						e = offset;
						f = e + (to - from);
						if (f > stop)
							goto mxPopState;
						g = e;
						while (from < to) {
							if (fxGetCharacter(subject, g, flags) != fxGetCharacter(subject, from, flags))
								goto mxPopState;
							g = fxFindCharacter(subject, g, 1);
							from = fxFindCharacter(subject, from, 1);
						}
						offset = f;
					}
					mxBreak;
				mxCase(cxCharSetBackwardStep):
					step = *pointer++;
					e = offset;
					if (e == 0)
						goto mxPopState;
					e = fxFindCharacter(subject, e, -1);
					if (!fxMatchCharacter(pointer, fxGetCharacter(subject, e, flags)))
						goto mxPopState;
					offset = e;
					mxBreak;
				mxCase(cxCharSetForwardStep):
					step = *pointer++;
					e = offset;
					if (e == stop)
						goto mxPopState;
					if (!fxMatchCharacter(pointer, fxGetCharacter(subject, e, flags)))
						goto mxPopState;
					e = fxFindCharacter(subject, e, 1);
					offset = e;
					mxBreak;
				mxCase(cxDisjunctionStep):
					step = *pointer++;
					sequel = *pointer;
					firstState = fxPushState(the, firstState, sequel, offset, captures, captureCount);
					if (!firstState)
						return 0;
					mxBreak;
				mxCase(cxEmptyStep):
					step = *pointer;
					mxBreak;
				mxCase(cxLineBeginStep):
					step = *pointer;
					if (offset == 0)
						mxBreak;
					if ((flags & XS_REGEXP_M) && fxMatchCharacter((txInteger*)gxLineCharacters, fxGetCharacter(subject, fxFindCharacter(subject, offset, -1), flags)))
						mxBreak;
					goto mxPopState;
				mxCase(cxLineEndStep):
					step = *pointer;
					if (offset == stop)
						mxBreak;
					if ((flags & XS_REGEXP_M) && fxMatchCharacter((txInteger*)gxLineCharacters, fxGetCharacter(subject, offset, flags)))
						mxBreak;
					goto mxPopState;
				mxCase(cxQuantifierStep):
					step = *pointer++;
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					quantifier = quantifiers + *pointer++;
					quantifier->min = *pointer++;
					quantifier->max = *pointer;
					quantifier->offset = offset;
					mxBreak;
				mxCase(cxQuantifierGreedyLoop):
					step = *pointer++;
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					quantifier = quantifiers + *pointer++;
					sequel = *pointer++;
					from = *pointer++;
					to = *pointer;
					#ifdef mxTrace 
						fprintf(stderr, " min=%d", quantifier->min);
						if (quantifier->max != 0x7FFFFFFF)
							fprintf(stderr, " max=%d", quantifier->max);
					#endif
					if (quantifier->max == 0) {
						step = sequel;
						mxBreak;
					}
					else {
						if (quantifier->min == 0) {
							firstState = fxPushState(the, firstState, sequel, offset, captures, captureCount);
							if (!firstState)
								return 0;
						}
						if (from < to)
							c_memset(captures + from, -1, (to - from) * sizeof(txCaptureData));
					}
					mxBreak;
				mxCase(cxQuantifierLazyLoop):
					step = *pointer++;
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					quantifier = quantifiers + *pointer++;
					sequel = *pointer++;
					from = *pointer++;
					to = *pointer;
					#ifdef mxTrace 
						fprintf(stderr, " min=%d", quantifier->min);
						if (quantifier->max != 0x7FFFFFFF)
							fprintf(stderr, " max=%d", quantifier->max);
					#endif
					if (quantifier->max == 0) {
						step = sequel;
						mxBreak;
					}
					if (quantifier->min == 0) {
						firstState = fxPushState(the, firstState, step, offset, captures, captureCount);
						if (!firstState)
							return 0;
						if (from < to)
							c_memset(captures + from, -1, (to - from) * sizeof(txCaptureData));
						step = sequel;
					}
					else {
						if (from < to)
							c_memset(captures + from, -1, (to - from) * sizeof(txCaptureData));
					}
					mxBreak;
				mxCase(cxQuantifierCompletion):
					step = *pointer++;
					#ifdef mxTrace 
						fprintf(stderr, " #%d", *pointer);
					#endif
					quantifier = quantifiers + *pointer++;
					sequel = *pointer;
					if ((quantifier->min == 0) && (quantifier->offset == offset)) {
						step = sequel;
						mxBreak;
					}
					quantifier->min = (quantifier->min == 0) ? 0 : quantifier->min - 1;
					quantifier->max = (quantifier->max == 0x7FFFFFFF) ? 0x7FFFFFFF : quantifier->max - 1;
					mxBreak;
				mxCase(cxWordBreakStep):
					step = *pointer;
					if (fxMatchCharacter((txInteger*)gxWordCharacters, fxGetCharacter(subject, fxFindCharacter(subject, offset, -1), flags)) 
							!= fxMatchCharacter((txInteger*)gxWordCharacters, fxGetCharacter(subject, offset, flags)))
						mxBreak;
					goto mxPopState;
				mxCase(cxWordContinueStep):
					step = *pointer;
					if (fxMatchCharacter((txInteger*)gxWordCharacters, fxGetCharacter(subject, fxFindCharacter(subject, offset, -1), flags)) 
							== fxMatchCharacter((txInteger*)gxWordCharacters, fxGetCharacter(subject, offset, flags)))
						mxBreak;
					goto mxPopState;
				
				mxPopState:
					if (!firstState) {
						step = 0;
						mxBreak;
					}
					#ifdef mxTrace 
						fprintf(stderr, " <<<");
					#endif
					step = firstState->step;
					offset = firstState->offset;
					c_memcpy(captures, firstState->captures, captureCount * sizeof(txCaptureData));
					if (firstState->the)
						firstState = firstState->nextState;
					else {
						txStateData* state = firstState;
						firstState = state->nextState;
						c_free(state);
					}
					mxBreak;
			}
		}
		#ifdef mxTrace
			fprintf(stderr, "\n###\n");
		#endif
		firstState = fxPopStates(the, firstState, C_NULL);
		if (flags & XS_REGEXP_Y)
			break;
		start = fxFindCharacter(subject, start, 1);
	}
	return result;
}




