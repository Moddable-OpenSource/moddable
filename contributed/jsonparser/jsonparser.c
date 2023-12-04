
#line 1 "jsonparser.rl"
// Copyright (c) 2023 Mark Wharton
// https://opensource.org/license/mit/

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h" // for xsID_ values

enum {
	XS_VALUE = 0,
	XS_KEYS,
	XS_VPT,
	XS_COUNT
};

struct jsonparser
{
	int size, bs;
	char *buffer;

	int cs, top, sd;
	int *stack;

	int mark;

	int n, nx;
	uint16_t w1;
	uint16_t w2;
};

char *grow_buffer( char *buffer, int *pbs )
{
	int old = *pbs;
	*pbs = *pbs * 2;

	char *nb = c_malloc( *pbs * sizeof(char) );
	if ( nb != C_NULL )
		c_memcpy( nb, buffer, old * sizeof(char) );
	c_free( buffer );
	return nb;
}

int *grow_stack( int *stack, int *psd )
{
	int old = *psd;
	*psd = *psd * 2;

	int *ns = c_malloc( *psd * sizeof(int) );
	if ( ns != C_NULL )
		c_memcpy( ns, stack, old * sizeof(int) );
	c_free( stack );
	return ns;
}

static const char not_enough_memory[] ICACHE_XS6STRING_ATTR = "jsonparser out of memory";

void before_buffer_char( xsMachine *the, struct jsonparser *fsm )
{
	if ( fsm->size == fsm->bs ) {
		fsm->buffer = grow_buffer( fsm->buffer, &fsm->bs );
		if ( fsm->buffer == C_NULL )
			xsUnknownError( (char *)not_enough_memory );
	}

	// reset escape sequences
	if ( fsm->n > 0 )
		++fsm->nx;
	fsm->n = fsm->w1 = fsm->w2 = 0;
}

void buffer_char( xsMachine *the, struct jsonparser *fsm, char c )
{
	before_buffer_char( the, fsm );
	fsm->buffer[fsm->size++] = c;
}

void buffer_unicode_character( xsMachine *the, struct jsonparser *fsm, uint32_t character )
{
	// with thanks to fxUTF8Encode
	if ( character == 0 ) {
		buffer_char( the, fsm, 0xC0 );
		buffer_char( the, fsm, 0x80 );
	}
	else if ( character < 0x80 ) {
		buffer_char( the, fsm, (char)character );
	}
	else if ( character < 0x800 ) {
		buffer_char( the, fsm, (char)( 0xC0 | ( character >> 6 ) ) );
		buffer_char( the, fsm, (char)( 0x80 | ( character & 0x3F ) ) );
	}
	else if ( character < 0x10000 ) {
		buffer_char( the, fsm, (char)( 0xE0 | ( character >> 12 ) ) );
		buffer_char( the, fsm, (char)( 0x80 | ( ( character >> 6 ) & 0x3F ) ) );
		buffer_char( the, fsm, (char)( 0x80 | ( character & 0x3F ) ) );
	}
	else if ( character < 0x110000 ) {
#if mxCESU8
		// fxCESU8Encode
		uint32_t surrogate;
		character -= 0x00010000;
		surrogate = 0xDC00 | ( character & 0x3FF );
		character = 0xD800 | ( ( character >> 10 ) & 0x3FF );
		buffer_char( the, fsm, (char)( ( 0xE0 | ( character >> 12 ) ) );
		buffer_char( the, fsm, (char)( ( 0x80 | ( ( character >> 6 ) & 0x3F ) ) );
		buffer_char( the, fsm, (char)( ( 0x80 | ( character & 0x3F ) ) );
		buffer_char( the, fsm, (char)( ( 0xE0 | ( surrogate >> 12 ) ) );
		buffer_char( the, fsm, (char)( ( 0x80 | ( ( surrogate >> 6 ) & 0x3F ) ) );
		buffer_char( the, fsm, (char)( ( 0x80 | ( surrogate & 0x3F ) ) );
#else
		// and fxUTF8Encode again
		buffer_char( the, fsm, (char)( 0xF0 | ( character >> 18 ) ) );
		buffer_char( the, fsm, (char)( 0x80 | ( ( character >> 12 ) & 0x3F ) ) );
		buffer_char( the, fsm, (char)( 0x80 | ( ( character >> 6 ) & 0x3F ) ) );
		buffer_char( the, fsm, (char)( 0x80 | ( character & 0x3F ) ) );
#endif
	}
}

bool parse_hex_char( char c, uint16_t *value )
{
	// thanks to fxParseHex
	if ( ( '0' <= c ) && ( c <= '9' ) )
		*value = ( *value * 16 ) + ( c - '0' );
	else if ( ( 'a' <= c ) && ( c <= 'f' ) )
		*value = ( *value * 16 ) + ( 10 + c - 'a' );
	else if ( ( 'A' <= c ) && ( c <= 'F' ) )
		*value = ( *value * 16 ) + ( 10 + c - 'A' );
	else
		return false;
	return true;
}

bool parse_unicode_char( xsMachine *the, struct jsonparser *fsm, char c )
{
	// parse up to 8 characters, accepting a single character on each invocation
	if ( fsm->n < 4 ) {
		if ( !parse_hex_char( c, &fsm->w1 ) )
			return false;
	}
	else if ( fsm->n < 8 ) {
		if ( !parse_hex_char( c, &fsm->w2 ) )
			return false;
	}
	else
		return false;

	// advance n, the number of parsed characters
	++fsm->n;

	// https://datatracker.ietf.org/doc/html/rfc2781#section-2.2
	if ( fsm->n == 4 ) {
		// decode single character, continue with surrogate pair, or error
		if ( fsm->w1 < 0xD800 || fsm->w1 > 0xDFFF ) {
			uint32_t character = fsm->w1;
			fsm->n = 0;
			buffer_unicode_character( the, fsm, character );
		}
		else if ( fsm->w1 > 0xDBFF )
			return false;
	}
	else if ( fsm->n == 8 ) {
		// decode surrogate pair, or error
		if ( fsm->w2 >= 0xDC00 && fsm->w2 <= 0xDFFF ) {
			uint32_t character = 0x10000 + ( ( fsm->w1 & 0x03FF ) << 10 ) + ( fsm->w2 & 0x03FF );
			fsm->n = 0;
			buffer_unicode_character( the, fsm, character );
		}
		else
			return false;
	}
	return true;
}

bool pop( xsMachine *the, struct jsonparser *fsm, xsSlot *slot )
{
	if ( fsm->mark == 0 ) {
		xsmcCall( xsResult, xsVar(XS_VPT), xsID_pop, slot, C_NULL );
		return xsmcToBoolean( xsResult );
	}
	--fsm->mark;
	return true;
}

void push( xsMachine *the, struct jsonparser *fsm, xsSlot *slot )
{
	if ( fsm->mark == 0 )
		xsmcCall( xsResult, xsVar(XS_VPT), xsID_push, slot, C_NULL );
	else
		++fsm->mark;
}

bool pop_null( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(1);
	return pop( the, fsm, &xsResult );
}

void push_null( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(1);
	push( the, fsm, &xsResult );
}

bool pop_false( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(2);
	return pop( the, fsm, &xsResult );
}

void push_false( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(2);
	push( the, fsm, &xsResult );
}

bool pop_true( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(3);
	return pop( the, fsm, &xsResult );
}

void push_true( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(3);
	push( the, fsm, &xsResult );
}

bool pop_number( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(4);
	return pop( the, fsm, &xsResult );
}

void push_number( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(4);
	push( the, fsm, &xsResult );
}

bool pop_string( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(5);
	return pop( the, fsm, &xsResult );
}

void push_string( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(5);
	push( the, fsm, &xsResult );
}

bool pop_array( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(6);
	return pop( the, fsm, &xsResult );
}

void push_array( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(6);
	push( the, fsm, &xsResult );
}

bool pop_object( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(7);
	return pop( the, fsm, &xsResult );
}

void push_object( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(7);
	push( the, fsm, &xsResult );
}

bool pop_field( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(8);
	return pop( the, fsm, &xsResult );
}

void push_field( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(8);
	push( the, fsm, &xsResult );
}

void set_text( xsMachine *the, struct jsonparser *fsm, char *buffer, int size )
{
	if ( fsm->mark == 0 ) {
		if ( fsm->buffer != C_NULL )
			xsmcSetStringBuffer( xsResult, fsm->buffer, fsm->size );
		xsmcCall( xsResult, xsVar(XS_VPT), xsID_setText, &xsResult, C_NULL );
	}
}


#line 437 "jsonparser.rl"



#line 304 "jsonparser.c"
static const short _JSON_key_offsets[] ICACHE_XS6RO_ATTR = {
	0, 0, 4, 15, 21, 27, 33, 39, 
	42, 47, 49, 54, 58, 60, 65, 70, 
	71, 72, 73, 74, 75, 76, 77, 78, 
	79, 80, 86, 88, 93, 107, 111, 117, 
	122, 133, 139, 145, 151, 157, 160, 165, 
	167, 172, 176, 178, 183, 188, 189, 190, 
	191, 192, 193, 194, 195, 196, 197, 198, 
	200, 215, 219, 225, 239, 242, 247, 249, 
	254, 258, 260, 265, 270, 271, 272, 273, 
	274, 275, 276, 277, 278, 279, 280, 291, 
	297, 303, 309, 315, 329, 333, 333
};

static const char _JSON_trans_keys[] ICACHE_XS6RO_ATTR = {
	34, 92, 0, 31, 34, 47, 92, 98, 
	102, 110, 114, 116, 117, 0, 31, 48, 
	57, 65, 70, 97, 102, 48, 57, 65, 
	70, 97, 102, 48, 57, 65, 70, 97, 
	102, 48, 57, 65, 70, 97, 102, 48, 
	49, 57, 46, 69, 101, 48, 57, 48, 
	57, 46, 69, 101, 48, 57, 43, 45, 
	48, 57, 48, 57, 46, 69, 101, 48, 
	57, 46, 69, 101, 48, 57, 97, 108, 
	115, 101, 117, 108, 108, 114, 117, 101, 
	13, 32, 34, 125, 9, 10, 34, 92, 
	13, 32, 58, 9, 10, 13, 32, 34, 
	45, 48, 91, 102, 110, 116, 123, 9, 
	10, 49, 57, 34, 92, 0, 31, 13, 
	32, 44, 125, 9, 10, 13, 32, 34, 
	9, 10, 34, 47, 92, 98, 102, 110, 
	114, 116, 117, 0, 31, 48, 57, 65, 
	70, 97, 102, 48, 57, 65, 70, 97, 
	102, 48, 57, 65, 70, 97, 102, 48, 
	57, 65, 70, 97, 102, 48, 49, 57, 
	46, 69, 101, 48, 57, 48, 57, 46, 
	69, 101, 48, 57, 43, 45, 48, 57, 
	48, 57, 46, 69, 101, 48, 57, 46, 
	69, 101, 48, 57, 97, 108, 115, 101, 
	117, 108, 108, 114, 117, 101, 34, 92, 
	13, 32, 34, 45, 48, 91, 93, 102, 
	110, 116, 123, 9, 10, 49, 57, 34, 
	92, 0, 31, 13, 32, 44, 93, 9, 
	10, 13, 32, 34, 45, 48, 91, 102, 
	110, 116, 123, 9, 10, 49, 57, 48, 
	49, 57, 46, 69, 101, 48, 57, 48, 
	57, 46, 69, 101, 48, 57, 43, 45, 
	48, 57, 48, 57, 46, 69, 101, 48, 
	57, 46, 69, 101, 48, 57, 97, 108, 
	115, 101, 117, 108, 108, 114, 117, 101, 
	34, 47, 92, 98, 102, 110, 114, 116, 
	117, 0, 31, 48, 57, 65, 70, 97, 
	102, 48, 57, 65, 70, 97, 102, 48, 
	57, 65, 70, 97, 102, 48, 57, 65, 
	70, 97, 102, 13, 32, 34, 45, 48, 
	91, 102, 110, 116, 123, 9, 10, 49, 
	57, 13, 32, 9, 10, 0
};

static const char _JSON_single_lengths[] ICACHE_XS6RO_ATTR = {
	0, 2, 9, 0, 0, 0, 0, 1, 
	3, 0, 3, 2, 0, 3, 3, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 4, 2, 3, 10, 2, 4, 3, 
	9, 0, 0, 0, 0, 1, 3, 0, 
	3, 2, 0, 3, 3, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 2, 
	11, 2, 4, 10, 1, 3, 0, 3, 
	2, 0, 3, 3, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 9, 0, 
	0, 0, 0, 10, 2, 0, 0
};

static const char _JSON_range_lengths[] ICACHE_XS6RO_ATTR = {
	0, 1, 1, 3, 3, 3, 3, 1, 
	1, 1, 1, 1, 1, 1, 1, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 0, 1, 2, 1, 1, 1, 
	1, 3, 3, 3, 3, 1, 1, 1, 
	1, 1, 1, 1, 1, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	2, 1, 1, 2, 1, 1, 1, 1, 
	1, 1, 1, 1, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 1, 3, 
	3, 3, 3, 2, 1, 0, 0
};

static const short _JSON_index_offsets[] ICACHE_XS6RO_ATTR = {
	0, 0, 4, 15, 19, 23, 27, 31, 
	34, 39, 41, 46, 50, 52, 57, 62, 
	64, 66, 68, 70, 72, 74, 76, 78, 
	80, 82, 88, 91, 96, 109, 113, 119, 
	124, 135, 139, 143, 147, 151, 154, 159, 
	161, 166, 170, 172, 177, 182, 184, 186, 
	188, 190, 192, 194, 196, 198, 200, 202, 
	205, 219, 223, 229, 242, 245, 250, 252, 
	257, 261, 263, 268, 273, 275, 277, 279, 
	281, 283, 285, 287, 289, 291, 293, 304, 
	308, 312, 316, 320, 333, 337, 338
};

static const unsigned char _JSON_indicies[] ICACHE_XS6RO_ATTR = {
	2, 3, 1, 0, 5, 6, 7, 8, 
	9, 10, 11, 12, 13, 1, 4, 14, 
	14, 14, 1, 15, 15, 15, 1, 16, 
	16, 16, 1, 17, 17, 17, 1, 18, 
	19, 1, 21, 22, 22, 1, 20, 23, 
	1, 1, 22, 22, 23, 20, 24, 24, 
	25, 1, 25, 1, 1, 1, 1, 25, 
	20, 21, 22, 22, 19, 20, 26, 1, 
	27, 1, 28, 1, 29, 1, 30, 1, 
	31, 1, 32, 1, 33, 1, 34, 1, 
	35, 1, 36, 36, 37, 38, 36, 1, 
	40, 41, 39, 42, 42, 43, 42, 1, 
	43, 43, 44, 45, 46, 48, 49, 50, 
	51, 52, 43, 47, 1, 54, 55, 1, 
	53, 56, 56, 57, 38, 56, 1, 58, 
	58, 37, 58, 1, 60, 61, 62, 63, 
	64, 65, 66, 67, 68, 1, 59, 69, 
	69, 69, 1, 70, 70, 70, 1, 71, 
	71, 71, 1, 72, 72, 72, 1, 73, 
	74, 1, 76, 77, 77, 1, 75, 78, 
	1, 1, 77, 77, 78, 75, 79, 79, 
	80, 1, 80, 1, 1, 1, 1, 80, 
	75, 76, 77, 77, 74, 75, 81, 1, 
	82, 1, 83, 1, 84, 1, 85, 1, 
	86, 1, 87, 1, 88, 1, 89, 1, 
	90, 1, 39, 39, 1, 91, 91, 92, 
	93, 94, 96, 97, 98, 99, 100, 101, 
	91, 95, 1, 103, 104, 1, 102, 105, 
	105, 106, 97, 105, 1, 106, 106, 92, 
	93, 94, 96, 98, 99, 100, 101, 106, 
	95, 1, 107, 108, 1, 110, 111, 111, 
	1, 109, 112, 1, 1, 111, 111, 112, 
	109, 113, 113, 114, 1, 114, 1, 1, 
	1, 1, 114, 109, 110, 111, 111, 108, 
	109, 115, 1, 116, 1, 117, 1, 118, 
	1, 119, 1, 120, 1, 121, 1, 122, 
	1, 123, 1, 124, 1, 126, 127, 128, 
	129, 130, 131, 132, 133, 134, 1, 125, 
	135, 135, 135, 1, 136, 136, 136, 1, 
	137, 137, 137, 1, 138, 138, 138, 1, 
	139, 139, 140, 141, 142, 144, 145, 146, 
	147, 148, 139, 143, 1, 139, 139, 139, 
	1, 1, 1, 0
};

static const char _JSON_trans_targs[] ICACHE_XS6RO_ATTR = {
	1, 0, 83, 2, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 3, 4, 5, 
	6, 1, 8, 14, 83, 9, 11, 10, 
	12, 13, 16, 17, 18, 83, 20, 21, 
	83, 23, 24, 83, 25, 26, 85, 26, 
	27, 55, 27, 28, 29, 37, 38, 44, 
	30, 45, 49, 52, 30, 29, 30, 32, 
	30, 31, 31, 29, 29, 29, 29, 29, 
	29, 29, 29, 29, 33, 34, 35, 36, 
	29, 38, 44, 30, 39, 41, 40, 42, 
	43, 46, 47, 48, 30, 50, 51, 30, 
	53, 54, 30, 56, 57, 60, 61, 67, 
	58, 86, 68, 72, 75, 58, 57, 58, 
	78, 58, 59, 61, 67, 58, 62, 64, 
	63, 65, 66, 69, 70, 71, 58, 73, 
	74, 58, 76, 77, 58, 57, 57, 57, 
	57, 57, 57, 57, 57, 57, 79, 80, 
	81, 82, 57, 84, 1, 7, 8, 14, 
	83, 15, 19, 22, 83
};

static const char _JSON_trans_actions[] ICACHE_XS6RO_ATTR = {
	1, 0, 2, 0, 0, 3, 4, 5, 
	6, 7, 8, 9, 10, 0, 11, 11, 
	11, 11, 1, 1, 12, 1, 1, 1, 
	1, 1, 0, 0, 0, 13, 0, 0, 
	14, 0, 0, 15, 0, 16, 17, 1, 
	18, 1, 0, 0, 19, 20, 20, 20, 
	21, 0, 0, 0, 22, 1, 2, 0, 
	0, 23, 0, 0, 3, 4, 5, 6, 
	7, 8, 9, 10, 0, 11, 11, 11, 
	11, 1, 1, 12, 1, 1, 1, 1, 
	1, 0, 0, 0, 13, 0, 0, 14, 
	0, 0, 15, 0, 19, 20, 20, 20, 
	21, 24, 0, 0, 0, 22, 1, 2, 
	0, 0, 0, 1, 1, 12, 1, 1, 
	1, 1, 1, 0, 0, 0, 13, 0, 
	0, 14, 0, 0, 15, 0, 3, 4, 
	5, 6, 7, 8, 9, 10, 0, 11, 
	11, 11, 11, 0, 19, 20, 20, 20, 
	21, 0, 0, 0, 22
};

static const int JSON_start = 83;
static const int JSON_first_final = 83;
static const int JSON_error = 0;

static const int JSON_en_members = 25;
static const int JSON_en_elements = 56;
static const int JSON_en_main = 83;


#line 440 "jsonparser.rl"

#pragma unused (JSON_en_main)
#pragma unused (JSON_en_members)
#pragma unused (JSON_en_elements)

int arg_to_index(xsMachine *the, int argi, int index, int length)
{
	// thanks to fx_String_prototype_slice
	if (xsmcArgc > argi && xsmcTypeOf(xsArg(argi)) != xsUndefinedType) {
		float i = c_trunc(xsmcToNumber(xsArg(argi)));
		if (c_isnan(i) || (i == 0))
			i = 0;
		if (i < 0) {
			i = length + i;
			if (i < 0)
				i = 0;
		}
		else if (i > length)
			i = length;
		index = i;
	}
	return index;
}

void xs_jsonparser_constructor(xsMachine *the)
{
	struct jsonparser *fsm = C_NULL;

	int initialBufferSize = 64;
	int initialStackDepth = 8;

	xsmcVars(XS_COUNT);
	if (xsmcArgc >= 1 && xsmcTypeOf(xsArg(0)) != xsUndefinedType) {
		if (xsmcHas(xsArg(0), xsID_initialBufferSize)) {
			xsmcGet(xsVar(XS_VALUE), xsArg(0), xsID_initialBufferSize);
			initialBufferSize = xsmcToInteger(xsVar(XS_VALUE));
		}
		if (xsmcHas(xsArg(0), xsID_initialStackDepth)) {
			xsmcGet(xsVar(XS_VALUE), xsArg(0), xsID_initialStackDepth);
			initialStackDepth = xsmcToInteger(xsVar(XS_VALUE));
		}
		if (xsmcHas(xsArg(0), xsID_keys)) {
			xsmcGet(xsVar(XS_KEYS), xsArg(0), xsID_keys);
			xsmcSet(xsThis, xsID_keys, xsVar(XS_KEYS));
		}
		if (xsmcHas(xsArg(0), xsID_matcher)) {
			if (xsmcHas(xsArg(0), xsID_vpt))
				xsUnknownError("cannot specify vpt and matcher together");
			xsmcGet(xsVar(XS_VALUE), xsArg(0), xsID_matcher);
			xsmcCall(xsVar(XS_VPT), xsThis, xsID_makeVPT, &xsVar(XS_VALUE), C_NULL);
			xsmcSet(xsThis, xsID_vpt, xsVar(XS_VPT));
		}
		else if (xsmcHas(xsArg(0), xsID_vpt)) {
			xsmcGet(xsVar(XS_VPT), xsArg(0), xsID_vpt);
			xsmcSet(xsThis, xsID_vpt, xsVar(XS_VPT));
		}
		else {
			xsmcCall(xsVar(XS_VPT), xsThis, xsID_makeJSONTree, C_NULL);
			xsmcSet(xsThis, xsID_vpt, xsVar(XS_VPT));
		}
	}
	else {
		xsmcCall(xsVar(XS_VPT), xsThis, xsID_makeJSONTree, C_NULL);
		xsmcSet(xsThis, xsID_vpt, xsVar(XS_VPT));
	}

	xsTry {
		fsm = c_malloc(sizeof(struct jsonparser));
		if (fsm == C_NULL)
			xsUnknownError((char *)not_enough_memory);
		fsm->buffer = C_NULL;
		fsm->stack = C_NULL;

		fsm->bs = initialBufferSize;
		fsm->buffer = c_malloc(fsm->bs * sizeof(char));
		if (fsm->buffer == C_NULL)
			xsUnknownError((char *)not_enough_memory);
		fsm->size = 0;

		fsm->sd = initialStackDepth;
		fsm->stack = c_malloc(fsm->sd * sizeof(int));
		if (fsm->stack == C_NULL)
			xsUnknownError((char *)not_enough_memory);

		
#line 591 "jsonparser.c"
	{
	 fsm->cs = JSON_start;
	 fsm->top = 0;
	}

#line 525 "jsonparser.rl"

		fsm->mark = 0;
		fsm->n = fsm->nx = 0;

		xsmcSetHostData(xsThis, fsm);
	}
	xsCatch {
		if (fsm != C_NULL) {
			if (fsm->buffer != C_NULL)
				c_free(fsm->buffer);
			if (fsm->stack != C_NULL)
				c_free(fsm->stack);
			c_free(fsm);
		}
		xsThrow(xsException);
	}
}

void xs_jsonparser_destructor(void *data)
{
	struct jsonparser *fsm = data;

	if (fsm != C_NULL) {
		if (fsm->buffer != C_NULL)
			c_free(fsm->buffer);
		if (fsm->stack != C_NULL)
			c_free(fsm->stack);
		c_free(fsm);
	}
}

void xs_jsonparser_close(xsMachine *the)
{
	struct jsonparser *fsm = xsmcGetHostData(xsThis);

	if (fsm == C_NULL)
		xsUnknownError("close on closed jsonparser");

	xsmcSetHostData(xsThis, C_NULL);
	xs_jsonparser_destructor(fsm);
}

void xs_jsonparser_receive(xsMachine *the)
{
	struct jsonparser *fsm = xsmcGetHostData(xsThis);

	if (fsm == C_NULL)
		xsUnknownError("receive on closed jsonparser");

	int count = 0;
	if (fsm->cs != JSON_error) {

		xsmcVars(XS_COUNT);
		xsmcGet(xsVar(XS_KEYS), xsThis, xsID_keys);
		xsmcGet(xsVar(XS_VPT), xsThis, xsID_vpt);

		// be careful with XS macros between here and copying our string into the buffer
		char *string = xsmcToString(xsArg(0)); // required string
		int length = c_strlen(string);
		int start = arg_to_index(the, 1, 0, length); // optional string slice start (default value is 0)
		int end = arg_to_index(the, 2, length, length); // optional string slice end (default value is length)
		string = xsmcToString(xsArg(0)); // reset the string after evaluating the start/end args
		if (start < end) {
			int offset = start;
			int size = end - start;
			if (offset >= 0 && size > 0) {
				char *buffer = C_NULL;
				xsTry {
					buffer = c_malloc(size);
					if (buffer == C_NULL)
						xsUnknownError((char *)not_enough_memory);
					c_memcpy(buffer, string + offset, size);

					const char *p = buffer;
					const char *pe = buffer + size;

					
#line 675 "jsonparser.c"
	{
	int _klen;
	const char *_keys;
	int _trans;

	if ( p == pe )
		goto _test_eof;
	if (  fsm->cs == 0 )
		goto _out;
_resume:
	_keys = _JSON_trans_keys + c_read16( &_JSON_key_offsets[ fsm->cs] );
	_trans = c_read16( &_JSON_index_offsets[ fsm->cs] );

	_klen = c_read8( &_JSON_single_lengths[ fsm->cs] );
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( ( c_read8( p ) ) < c_read8( _mid ))
				_upper = _mid - 1;
			else if ( ( c_read8( p ) ) > c_read8( _mid ))
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = c_read8( &_JSON_range_lengths[ fsm->cs] );
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( ( c_read8( p ) ) < c_read8( &_mid[0] ) )
				_upper = _mid - 2;
			else if ( ( c_read8( p ) ) > c_read8( &_mid[1] ) )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = c_read8( &_JSON_indicies[_trans] );
	 fsm->cs = c_read8( &_JSON_trans_targs[_trans] );

	if ( c_read8( &_JSON_trans_actions[_trans] ) == 0 )
		goto _again;

	switch ( c_read8( &_JSON_trans_actions[_trans] ) ) {
	case 1:
#line 309 "jsonparser.rl"
	{ buffer_char( the, fsm, c_read8( p ) ); }
	break;
	case 22:
#line 311 "jsonparser.rl"
	{
		push_object( the, fsm );
		{
		if ( fsm->top == fsm->sd ) {
			fsm->stack = grow_stack( fsm->stack, &fsm->sd );
			if ( fsm->stack == C_NULL )
				xsUnknownError( (char *)not_enough_memory );
		}
	{ fsm->stack[ fsm->top++] =  fsm->cs;  fsm->cs = 25;goto _again;}}
	}
	break;
	case 21:
#line 321 "jsonparser.rl"
	{
		push_array( the, fsm );
		{
		if ( fsm->top == fsm->sd ) {
			fsm->stack = grow_stack( fsm->stack, &fsm->sd );
			if ( fsm->stack == C_NULL )
				xsUnknownError( (char *)not_enough_memory );
		}
	{ fsm->stack[ fsm->top++] =  fsm->cs;  fsm->cs = 56;goto _again;}}
	}
	break;
	case 24:
#line 326 "jsonparser.rl"
	{
		if ( !pop_array( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
		{ fsm->cs =  fsm->stack[-- fsm->top]; goto _again;}
	}
	break;
	case 16:
#line 351 "jsonparser.rl"
	{
		push_field( the, fsm );
	}
	break;
	case 23:
#line 355 "jsonparser.rl"
	{
		if ( !pop_field( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
	}
	break;
	case 18:
#line 359 "jsonparser.rl"
	{
		xsmcSetStringBuffer( xsResult, fsm->buffer, fsm->size );
		if ( fsm->mark == 0 ) {
			if ( xsmcTypeOf( xsVar(XS_KEYS) ) != xsUndefinedType ) {
				xsmcCall( xsVar(XS_VALUE), xsVar(XS_KEYS), xsID_includes, &xsResult, C_NULL );
				if ( !xsmcToBoolean( xsVar(XS_VALUE) ) ) {
					// fields are pushed before the name is known, so we have to check and balance the tree
					// prune field node that was rejected because it failed to match any of the keys
					if ( !pop_field( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
					++fsm->mark; // this mark cancels post value separator and pre object exit field pops
				}
			}
		}
		set_text( the, fsm, C_NULL, 0 );
		fsm->size = 0;
	}
	break;
	case 15:
#line 376 "jsonparser.rl"
	{
		push_true( the, fsm ); pop_true( the, fsm );
	}
	break;
	case 13:
#line 380 "jsonparser.rl"
	{
		push_false( the, fsm ); pop_false( the, fsm );
	}
	break;
	case 14:
#line 384 "jsonparser.rl"
	{
		push_null( the, fsm ); pop_null( the, fsm );
	}
	break;
	case 3:
#line 405 "jsonparser.rl"
	{ buffer_char( the, fsm, '"' ); }
	break;
	case 5:
#line 406 "jsonparser.rl"
	{ buffer_char( the, fsm, '\\' ); }
	break;
	case 4:
#line 407 "jsonparser.rl"
	{ buffer_char( the, fsm, '/' ); }
	break;
	case 6:
#line 408 "jsonparser.rl"
	{ buffer_char( the, fsm, '\b' ); }
	break;
	case 7:
#line 409 "jsonparser.rl"
	{ buffer_char( the, fsm, '\f' ); }
	break;
	case 8:
#line 410 "jsonparser.rl"
	{ buffer_char( the, fsm, '\n' ); }
	break;
	case 9:
#line 411 "jsonparser.rl"
	{ buffer_char( the, fsm, '\r' ); }
	break;
	case 10:
#line 412 "jsonparser.rl"
	{ buffer_char( the, fsm, '\t' ); }
	break;
	case 11:
#line 414 "jsonparser.rl"
	{ if ( !parse_unicode_char( the, fsm, c_read8( p ) ) ) { fsm->cs = (JSON_error); goto _again;} }
	break;
	case 20:
#line 331 "jsonparser.rl"
	{
		push_number( the, fsm );
	}
#line 309 "jsonparser.rl"
	{ buffer_char( the, fsm, c_read8( p ) ); }
	break;
	case 19:
#line 341 "jsonparser.rl"
	{
		push_string( the, fsm );
	}
#line 401 "jsonparser.rl"
	{ before_buffer_char( the, fsm ); }
	break;
	case 17:
#line 355 "jsonparser.rl"
	{
		if ( !pop_field( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
	}
#line 316 "jsonparser.rl"
	{
		if ( !pop_object( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
		{ fsm->cs =  fsm->stack[-- fsm->top]; goto _again;}
	}
	break;
	case 12:
#line 397 "jsonparser.rl"
	{ p--; }
#line 335 "jsonparser.rl"
	{
		set_text( the, fsm, fsm->buffer, fsm->size );
		fsm->size = 0;
		if ( !pop_number( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
	}
	break;
	case 2:
#line 418 "jsonparser.rl"
	{ if ( fsm->n || fsm->nx ) { fsm->cs = (JSON_error); goto _again;} }
#line 345 "jsonparser.rl"
	{
		set_text( the, fsm, fsm->buffer, fsm->size );
		fsm->size = 0;
		if ( !pop_string( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
	}
	break;
#line 911 "jsonparser.c"
	}

_again:
	if (  fsm->cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

#line 602 "jsonparser.rl"

					c_free(buffer);
					count = size;
				}
				xsCatch {
					if (buffer != C_NULL)
						c_free(buffer);
					xsThrow(xsException);
				}
				buffer = C_NULL;
			}
		}
	}
	xsResult = xsInteger(count);
}

void xs_jsonparser_status(xsMachine *the)
{
	struct jsonparser *fsm = xsmcGetHostData(xsThis);

	if (fsm == C_NULL)
		xsUnknownError("status on closed jsonparser");

	xsmcGet(xsResult, xsThis, xsID_constructor);
	if (fsm->cs == JSON_error)
		xsmcGet(xsResult, xsResult, xsID_failure);
	else if (fsm->cs >= JSON_first_final)
		xsmcGet(xsResult, xsResult, xsID_success);
	else
		xsmcGet(xsResult, xsResult, xsID_receive);
}
