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

%%{
	machine JSON;
	access fsm->;
	getkey c_read8( p ) ;

	prepush {
		if ( fsm->top == fsm->sd ) {
			fsm->stack = grow_stack( fsm->stack, &fsm->sd );
			if ( fsm->stack == C_NULL )
				xsUnknownError( (char *)not_enough_memory );
		}
	}

	action buf { buffer_char( the, fsm, c_read8( p ) ); }

	action object_enter {
		push_object( the, fsm );
		fcall members;
	}

	action object_exit {
		if ( !pop_object( the, fsm ) ) fgoto *JSON_error;
		fret;
	}

	action array_enter {
		push_array( the, fsm );
		fcall elements;
	}

	action array_exit {
		if ( !pop_array( the, fsm ) ) fgoto *JSON_error;
		fret;
	}

	action number_enter {
		push_number( the, fsm );
	}

	action number_exit {
		set_text( the, fsm, fsm->buffer, fsm->size );
		fsm->size = 0;
		if ( !pop_number( the, fsm ) ) fgoto *JSON_error;
	}

	action string_enter {
		push_string( the, fsm );
	}

	action string_exit {
		set_text( the, fsm, fsm->buffer, fsm->size );
		fsm->size = 0;
		if ( !pop_string( the, fsm ) ) fgoto *JSON_error;
	}

	action field_enter {
		push_field( the, fsm );
	}

	action field_exit {
		if ( !pop_field( the, fsm ) ) fgoto *JSON_error;
	}

	action field_name {
		xsmcSetStringBuffer( xsResult, fsm->buffer, fsm->size );
		if ( fsm->mark == 0 ) {
			if ( xsmcTypeOf( xsVar(XS_KEYS) ) != xsUndefinedType ) {
				xsmcCall( xsVar(XS_VALUE), xsVar(XS_KEYS), xsID_includes, &xsResult, C_NULL );
				if ( !xsmcToBoolean( xsVar(XS_VALUE) ) ) {
					// fields are pushed before the name is known, so we have to check and balance the tree
					// prune field node that was rejected because it failed to match any of the keys
					if ( !pop_field( the, fsm ) ) fgoto *JSON_error;
					++fsm->mark; // this mark cancels post value separator and pre object exit field pops
				}
			}
		}
		set_text( the, fsm, C_NULL, 0 );
		fsm->size = 0;
	}

	action true {
		push_true( the, fsm ); pop_true( the, fsm );
	}

	action false {
		push_false( the, fsm ); pop_false( the, fsm );
	}

	action null {
		push_null( the, fsm ); pop_null( the, fsm );
	}

	ws = [ \t\r\n];

	number = (
		(
			'-'?
			( '0' | [1-9][0-9]* )
			( '.' [0-9]+ )?
			( [Ee] [+\-]? [0-9]+ )?
		) $buf
		[^.Ee0-9] @{ fhold; }
	) >number_enter @number_exit;

	string = (
		'"' @{ before_buffer_char( the, fsm ); }
		(
			^( [\"\\] | 0..0x1f ) $buf |

			'\\"'  @{ buffer_char( the, fsm, '"' ); } |
			'\\\\' @{ buffer_char( the, fsm, '\\' ); } |
			'\\/'  @{ buffer_char( the, fsm, '/' ); } |
			'\\b'  @{ buffer_char( the, fsm, '\b' ); } |
			'\\f'  @{ buffer_char( the, fsm, '\f' ); } |
			'\\n'  @{ buffer_char( the, fsm, '\n' ); } |
			'\\r'  @{ buffer_char( the, fsm, '\r' ); } |
			'\\t'  @{ buffer_char( the, fsm, '\t' ); } |

			( '\\u' [0-9a-fA-F]{4} ${ if ( !parse_unicode_char( the, fsm, c_read8( p ) ) ) fgoto *JSON_error; } ) |

			( '\\'^([\"\\/bfnrtu]|0..0x1f) )
		)*
		'"' @{ if ( fsm->n || fsm->nx ) fgoto *JSON_error; } # TODO: review
	) >string_enter @string_exit;

	primitive = ( number | string | 'true' @true | 'false' @false | 'null' @null );

	value = ( '{' @object_enter | '[' @array_enter | primitive );

	name = ( '"' ( '\\"' | '\\\\' | [^\"\\]+ )* $buf '"' ) >field_enter @field_name;

	member = ( ws* name ws* ':' ws* value );

	members := ( ( member ( ws* ',' @field_exit member )* )? ws* '}' @field_exit @object_exit | ws* '}' @object_exit );

	element = ( ws* value );

	elements := ( element ( ws* ',' element )* )? ws* ']' @array_exit;

	main := ( value* ws* );

}%%

%% write data;

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

		%% write init;

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

					%% write exec;

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
