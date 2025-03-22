/*
 * Copyright (c) 2024-2025 Moddable Tech, Inc.
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

 #include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"

struct JSONBase64ParserRecord {
	xsIntegerValue state;
	
	xsSlot* data;
	xsSlot* target;

	xsStringValue buffer;
	xsIntegerValue bufferIndex;
	xsIntegerValue bufferLength;
	
	xsIntegerValue escapeCount;
	xsIntegerValue escapeValue;
	
	xsStringValue literal;
	
	uint8_t base64Buffer[4];
	xsIntegerValue base64BufferLength;
	xsIntegerValue base64PadLength;
	
	uint8_t* dataBuffer;
	uint32_t dataLength;
	uint32_t dataMinimum;
	uint32_t dataStart;
	uint32_t dataStep;
	uint32_t dataStop;
	uint32_t dataSkip;
};
typedef struct JSONBase64ParserRecord JSONBase64ParserRecord;
typedef struct JSONBase64ParserRecord *JSONBase64Parser;

static void JSONBase64Parser_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks JSONBase64ParserHooks = {
	JSONBase64Parser_destructor,
	JSONBase64Parser_mark,
	NULL
};

enum {
	JSON_ERROR = -1,
	JSON_WS_VALUE,
	JSON_VALUE_WS,
	JSON_WS_BRACE,
	JSON_WS_BRACKET,
	
	JSON_BASE64,
	
	JSON_STRING,
	JSON_STRING_ESCAPE,
	JSON_STRING_ESCAPE_HEX,
	
	JSON_MINUS,
	JSON_ZERO,
	JSON_INTEGER,
	JSON_FRACTION_1,
	JSON_FRACTION,
	JSON_EXPONENT_SIGN,
	JSON_EXPONENT_1,
	JSON_EXPONENT,
	
	JSON_WS_NAME,
	JSON_NAME,
	JSON_NAME_ESCAPE,
	JSON_NAME_ESCAPE_HEX,
	JSON_NAME_WS,
	
	JSON_LITERAL,
};

#define IS_JSON_SPACE(C) ((C == 9) || (C == 10) || (C == 13) || (C == 32))

void JSONBase64Parser_constructor(xsMachine* the)
{
	JSONBase64Parser parser = c_calloc(1, sizeof(JSONBase64ParserRecord));
	if (!parser)
		xsRangeError("not enough memory");
		
	parser->state = JSON_WS_VALUE;
	parser->target = xsmcToReference(xsArg(0));
	parser->data = xsmcToReference(xsArg(1));
	parser->dataStep = xsmcToInteger(xsArg(2));
	parser->dataMinimum = xsmcToInteger(xsArg(3));
		
	parser->bufferLength = 1024;	
	parser->buffer = c_malloc(parser->bufferLength);	
	if (!parser->buffer)
		xsRangeError("not enough memory");
	
	xsmcGetBufferReadable(xsArg(1), (void **)&(parser->dataBuffer), &parser->dataLength);
	
	xsmcSetHostData(xsThis, parser);
	xsSetHostHooks(xsThis, (xsHostHooks *)&JSONBase64ParserHooks);
}

void JSONBase64Parser_destructor(void *it)
{
	if (it) {
		JSONBase64Parser parser = it;
		if (parser->buffer)
			c_free(parser->buffer);
		c_free(it);
	}
}

void JSONBase64Parser_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	JSONBase64Parser parser = it;
	if (parser->data)
		(*markRoot)(the, parser->data);
	if (parser->target)
		(*markRoot)(the, parser->target);
}

static void JSONBase64ParserBuffer(xsMachine* the, JSONBase64Parser parser, char byte)
{
	if (parser->bufferIndex == parser->bufferLength) {
		parser->bufferLength += 1024;
		parser->buffer = c_realloc(parser->buffer, parser->bufferLength);
		if (!parser->buffer)
			xsRangeError("not enough memory");
	}
	parser->buffer[parser->bufferIndex] = byte;
	parser->bufferIndex++;
}

static void JSONBase64Parser_parse_number(xsMachine* the, JSONBase64Parser parser)
{
	xsNumberValue number;
	JSONBase64ParserBuffer(the, parser, 0);
	number = fxStringToNumber(the, parser->buffer, 1);
	xsCall1(xsThis, xsID_setValue, xsNumber(number));
}

static void JSONBase64ParserBeginBase64(xsMachine* the, JSONBase64Parser parser, xsUnsignedValue size)
{
	size = ((size + 3) >> 2) * 3;
	xsCall3(xsThis, xsID_wait, xsInteger(parser->dataStop), xsInteger(parser->dataLength), xsInteger(size));
}

static void JSONBase64ParserContinueBase64(xsMachine* the, JSONBase64Parser parser, txU1 byte)
{
	txU1 *buffer = parser->base64Buffer;
	xsIntegerValue bufferLength = parser->base64BufferLength;
	xsIntegerValue padLength = parser->base64PadLength;
	if (byte == 0) {
		if (bufferLength != 0)
			xsSyntaxError("invalid string");
		return;
	}
	if (byte == '=') {
		if (bufferLength < 2)	
			xsSyntaxError("invalid string");
		buffer[bufferLength] = 0;
		bufferLength++;
		padLength++;
	} 
	else {
		if (('A' <= byte) && (byte <= 'Z'))
			byte = byte - 'A';
		else if (('a' <= byte) && (byte <= 'z'))
			byte = byte - 'a' + 26;
		else if (('0' <= byte) && (byte <= '9'))
			byte = byte - '0' + 52;
		else if (byte == '+')
			byte = 62;
		else if (byte == '/')
			byte = 63;
		else
			xsSyntaxError("invalid string");
		buffer[bufferLength] = byte;
		bufferLength++;
	}
	if (bufferLength == 4) {
		txU1 *dataPointer = parser->dataBuffer + parser->dataStop;
		txU1 *dataLimit = parser->dataBuffer + parser->dataLength;
		xsIntegerValue bufferIndex;
		buffer[0] = (buffer[0] << 2) | ((buffer[1] & 0x30) >> 4);
		buffer[1] = ((buffer[1] & 0x0F) << 4) | ((buffer[2] & 0x3C) >> 2);
		buffer[2] = ((buffer[2] & 0x03) << 6) | (buffer[3] & 0x3F);
		bufferLength -= padLength;
		bufferIndex = 0;
		while ((parser->dataSkip > 0) && (bufferIndex < bufferLength)) {
			parser->dataSkip--;
			bufferIndex++;
		}		
		while (bufferIndex < bufferLength) {
			if (dataPointer == dataLimit)
				dataPointer = parser->dataBuffer;
			*dataPointer++ = buffer[bufferIndex];
			bufferIndex++;
		}
		parser->dataStop = dataPointer - parser->dataBuffer;
		
		bufferLength = 0;
		padLength = 1;
	}
	parser->base64BufferLength = bufferLength;
	parser->base64PadLength = padLength;
}

static void JSONBase64ParserEndBase64(xsMachine* the, JSONBase64Parser parser, txU1 last)
{
	uint32_t size;
	uint32_t delta;
	if (parser->dataStart > parser->dataStop) {
		size = parser->dataLength - parser->dataStart;
		xsCall2(xsReference(parser->target), xsID_onBase64, xsInteger(parser->dataStart), xsInteger(size));
		parser->dataStart = 0;
	}
	size = parser->dataStop - parser->dataStart;
	delta = size & (parser->dataStep - 1);
	size -= delta;
	if (last || (size >= parser->dataMinimum)) {
		xsCall2(xsReference(parser->target), xsID_onBase64, xsInteger(parser->dataStart), xsInteger(size));
		parser->dataStart += size;
	}
}

void JSONBase64Parser_read(xsMachine* the)
{
	JSONBase64Parser parser = xsmcGetHostDataValidate(xsThis, (void *)&JSONBase64ParserHooks);
	xsIntegerValue state = parser->state;
	char* buffer;
	xsUnsignedValue length, index;
	xsIntegerValue test;
	
	xsmcVars(3);
	xsmcGetBufferReadable(xsArg(0), (void **)&buffer, &length);
	
	if (state == JSON_BASE64)
		JSONBase64ParserBeginBase64(the, parser, length);
	for (index = 0; index < length; index++) {
		char c = buffer[index];
		if (state == JSON_WS_VALUE) {
WS_VALUE:
			if (c == '"') {
				xsmcGet(xsResult, xsThis, xsID_result);
				xsmcGet(xsVar(0), xsThis, xsID_current);
				xsmcGet(xsVar(1), xsThis, xsID_name);
				xsResult = xsCall3(xsReference(parser->target), xsID_isBase64, xsResult, xsVar(0), xsVar(1));
				if (xsmcTest(xsResult)) {
					parser->base64BufferLength = 0;
					parser->base64PadLength = 1;
					parser->dataSkip = (xsmcTypeOf(xsResult) == xsIntegerType) ? xsmcToInteger(xsResult) : 0;
					JSONBase64ParserBeginBase64(the, parser, length - index);
					state = JSON_BASE64;
				}
				else {
					parser->bufferIndex = 0;
					state = JSON_STRING;
				}
			}
			else if (c == '-') {
				parser->bufferIndex = 0;
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_MINUS;
			}
			else if (c == '0') {
				parser->bufferIndex = 0;
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_ZERO;
			}
			else if (('1' <= c) && (c <= '9')) {
				parser->bufferIndex = 0;
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_INTEGER;
			}
			else if (c == '{') {
				xsResult = xsNewObject();
				xsCall1(xsThis, xsID_setValue, xsResult);
				xsCall1(xsThis, xsID_push, xsResult);
				xsmcGetBufferReadable(xsArg(0), (void **)&buffer, &length);
				state = JSON_WS_BRACE;
			}
			else if (c == '[') {
				xsResult = xsNewArray(0);
				xsCall1(xsThis, xsID_setValue, xsResult);
				xsCall1(xsThis, xsID_push, xsResult);
				xsmcGetBufferReadable(xsArg(0), (void **)&buffer, &length);
				state = JSON_WS_BRACKET;
			}
			else if (c == 't') {
				xsResult = xsTrue;
				parser->literal = "rue";
				state = JSON_LITERAL;
			}
			else if (c == 'f') {
				xsResult = xsFalse;
				parser->literal = "alse";
				state = JSON_LITERAL;
			}
			else if (c == 'n') {
				xsResult = xsNull;
				parser->literal = "ull";
				state = JSON_LITERAL;
			}
			else if (!IS_JSON_SPACE(c)) {
				state = JSON_ERROR;
			}
		}
		else if (state == JSON_LITERAL) {
			if (c == *parser->literal) {
				parser->literal++;
				if (0 == *parser->literal) {
					xsCall1(xsThis, xsID_setValue, xsResult);
					xsmcGetBufferReadable(xsArg(0), (void **)&buffer, &length);
					state = JSON_VALUE_WS;
				}
			}
			else
				state = JSON_ERROR;
		}
		
		else if (state == JSON_MINUS) {
			if (c == '0') {
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_ZERO;
			}
			else if (('1' <= c) && (c <= '9')) {
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_INTEGER;
			}
			else
				state = JSON_ERROR;
		}
		else if (state == JSON_ZERO) {
			if (c == '.') {
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_FRACTION_1;
			}
			else if ((c == 'e') || (c =='E')) {
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_EXPONENT_SIGN;
			}
			else {
				JSONBase64Parser_parse_number(the, parser);
				state = JSON_VALUE_WS;
				goto VALUE_WS;
			}
		}
		else if (state == JSON_INTEGER) {
			if (('0' <= c) && (c <= '9')) {
				JSONBase64ParserBuffer(the, parser, c);
			}
			else if (c == '.') {
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_FRACTION_1;
			}
			else if ((c == 'e') || (c =='E')) {
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_EXPONENT_SIGN;
			}
			else {
				JSONBase64Parser_parse_number(the, parser);
				state = JSON_VALUE_WS;
				goto VALUE_WS;
			}
		}
		else if (state == JSON_FRACTION_1) {
			if (('0' <= c) && (c <= '9')) {
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_FRACTION;
			}
			else
				state = JSON_ERROR;
		}
		else if (state == JSON_FRACTION) {
			if (('0' <= c) && (c <= '9')) {
				JSONBase64ParserBuffer(the, parser, c);
			}
			else if ((c == 'e') || (c =='E')) {
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_EXPONENT_SIGN;
			}
			else {
				JSONBase64Parser_parse_number(the, parser);
				state = JSON_VALUE_WS;
				goto VALUE_WS;
			}
		}
		else if (state == JSON_EXPONENT_SIGN) {
			if (c == '-') {
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_EXPONENT_1;
			}
			else if (c == '+') {
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_EXPONENT_1;
			}
			else if (('0' <= c) && (c <= '9')) {
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_EXPONENT;
			}
			else
				state = JSON_ERROR;
		}
		else if (state == JSON_EXPONENT_1) {
			if (('0' <= c) && (c <= '9')) {
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_EXPONENT;
			}
			else
				state = JSON_ERROR;
		}
		else if (state == JSON_EXPONENT) {
			if (('0' <= c) && (c <= '9')) {
				JSONBase64ParserBuffer(the, parser, c);
				state = JSON_EXPONENT;
			}
			else {
				JSONBase64Parser_parse_number(the, parser);
				state = JSON_VALUE_WS;
				goto VALUE_WS;
			}
		}
		
		else if (state == JSON_BASE64) {
			if (c == '"') {
				JSONBase64ParserContinueBase64(the, parser, 0);
				JSONBase64ParserEndBase64(the, parser, 1);
				xsCall1(xsThis, xsID_setValue, xsNull);
				xsmcGetBufferReadable(xsArg(0), (void **)&buffer, &length);
				state = JSON_VALUE_WS;
			}
			else
				JSONBase64ParserContinueBase64(the, parser, c);
		}
		else if (state == JSON_STRING) {
			if (c == '"') {
				JSONBase64ParserBuffer(the, parser, 0);
				xsCall1(xsThis, xsID_setValue, xsString(parser->buffer));
				xsmcGetBufferReadable(xsArg(0), (void **)&buffer, &length);
				state = JSON_VALUE_WS;
			}
			else if (c == '\\')
				state = JSON_STRING_ESCAPE;
			else 
				JSONBase64ParserBuffer(the, parser, c);
		}
		else if ((state == JSON_STRING_ESCAPE) || (state == JSON_NAME_ESCAPE)) {
			state--;
			switch (c) {
			case '"':
			case '/':
			case '\\':
				JSONBase64ParserBuffer(the, parser, c);
				break;
			case 'b':
				JSONBase64ParserBuffer(the, parser, '\b');
				break;
			case 'f':
				JSONBase64ParserBuffer(the, parser, '\f');
				break;
			case 'n':
				JSONBase64ParserBuffer(the, parser, '\n');
				break;
			case 'r':
				JSONBase64ParserBuffer(the, parser, '\r');
				break;
			case 't':
				JSONBase64ParserBuffer(the, parser, '\t');
				break;
			case 'u':
				state++;
				parser->escapeCount = 0;
				parser->escapeValue = 0;	
				break;
			default:
				state = JSON_ERROR;
			}
		}
		else if ((state == JSON_STRING_ESCAPE_HEX) || (state == JSON_NAME_ESCAPE_HEX)) {
			xsIntegerValue value = parser->escapeValue;
			if (('0' <= c) && (c <= '9'))
				value = (value * 16) + (c - '0');
			else if (('a' <= c) && (c <= 'f'))
				value = (value * 16) + (10 + c - 'a');
			else if (('A' <= c) && (c <= 'F'))
				value = (value * 16) + (10 + c - 'A');
			parser->escapeCount++;
			parser->escapeValue = value;	
			if (parser->escapeCount == 4) {
				char utf8[4];
				char* p = utf8;
				char* q = fxUTF8Encode(p, parser->escapeValue);
				while (p < q)
					JSONBase64ParserBuffer(the, parser, *p++);
				state -= 2;
			}
		}
		
		else if (state == JSON_WS_NAME) {
WS_NAME:
			if (c == '"') {
				parser->bufferIndex = 0;
				state = JSON_NAME;
			}
			else if (!IS_JSON_SPACE(c))
				state = JSON_ERROR;
		}
		else if (state == JSON_NAME) {
			if (c == '"') {
				JSONBase64ParserBuffer(the, parser, 0);
				xsCall1(xsThis, xsID_setName, xsString(parser->buffer));
				xsmcGetBufferReadable(xsArg(0), (void **)&buffer, &length);
				state = JSON_NAME_WS;
			}
			else if (c == '\\')
				state = JSON_NAME_ESCAPE;
			else
				JSONBase64ParserBuffer(the, parser, c);
		}
		else if (state == JSON_NAME_WS) {
			if (c == ':') {
				state = JSON_WS_VALUE;
			}
			else if (!IS_JSON_SPACE(c))
				state = JSON_ERROR;
		}
		
		else if (state == JSON_VALUE_WS) {
VALUE_WS:
			if (c == ',') {
				xsResult = xsCall0(xsThis, xsID_test);
				test = xsmcToInteger(xsResult);
				if (test < 0) {
					state = JSON_WS_NAME;
				}
				else if (test > 0) {
					state = JSON_WS_VALUE;
				}
				else
					state = JSON_ERROR;
			}
			else if (c == '}') {
BRACE_WS:
				xsResult = xsCall0(xsThis, xsID_test);
				test = xsmcToInteger(xsResult);
				if (test < 0) {
					xsCall0(xsThis, xsID_pop);
					xsmcGetBufferReadable(xsArg(0), (void **)&buffer, &length);
					state = JSON_VALUE_WS;
				}	
				else
					state = JSON_ERROR;
			}
			else if (c == ']') {
BRACKET_WS:
				xsResult = xsCall0(xsThis, xsID_test);
				test = xsmcToInteger(xsResult);
				if (test > 0) {
					xsCall0(xsThis, xsID_pop);
					xsmcGetBufferReadable(xsArg(0), (void **)&buffer, &length);
					state = JSON_VALUE_WS;
				}	
				else
					state = JSON_ERROR;
			}
			else if (!IS_JSON_SPACE(c))
				state = JSON_ERROR;
		}
		else if (state == JSON_WS_BRACE) {
			if (c == '}')
				goto BRACE_WS;
			else
				goto WS_NAME;
		}
		else if (state == JSON_WS_BRACKET) {
			if (c == ']')
				goto BRACKET_WS;
			else
				goto WS_VALUE;
		}
		if (state == JSON_ERROR)
			xsTypeError("invalid JSON");
	}
	parser->state = state;
	if (state == JSON_BASE64)
		JSONBase64ParserEndBase64(the, parser, 0);
	xsmcGet(xsResult, xsThis, xsID_result);
}

void JSONBase64Parser_reset(xsMachine* the)
{
	JSONBase64Parser parser = xsmcGetHostDataValidate(xsThis, (void *)&JSONBase64ParserHooks);
	parser->bufferIndex = 0;
	parser->escapeCount = 0;
	parser->escapeValue = 0;
	parser->literal = NULL;
	parser->state = JSON_WS_VALUE;
	parser->base64BufferLength = 0;
	parser->base64PadLength = 1;
	xsResult = xsUndefined;
	xsmcSet(xsThis, xsID_current, xsResult);
	xsmcSet(xsThis, xsID_name, xsResult);
	xsmcSet(xsThis, xsID_stack, xsResult);
}

void JSONBase64Parser_copy(xsMachine* the)
{
	JSONBase64Parser parser = xsmcGetHostDataValidate(xsThis, (void *)&JSONBase64ParserHooks);
	char* buffer;
	uint32_t size, delta;
	xsmcGetBufferReadable(xsArg(0), (void **)&buffer, &size);
	xsCall3(xsThis, xsID_wait, xsInteger(parser->dataStop), xsInteger(parser->dataLength), xsInteger(size));
	delta = parser->dataLength - parser->dataStop;
	if (delta < size) {
		c_memcpy(parser->dataBuffer + parser->dataStop, buffer, delta);
		xsCall2(xsReference(parser->target), xsID_onBase64, xsInteger(parser->dataStart), xsInteger(parser->dataLength - parser->dataStart));
		size -= delta;
		c_memcpy(parser->dataBuffer, buffer + delta, size);
		parser->dataStart = 0;
		parser->dataStop = 0;
	}
	else
		c_memcpy(parser->dataBuffer + parser->dataStop, buffer, size);
	parser->dataStop += size;
	size = parser->dataStop - parser->dataStart;
	delta = size & (parser->dataStep - 1);
	size -= delta;
	if (size >= parser->dataMinimum) {
		xsCall2(xsReference(parser->target), xsID_onBase64, xsInteger(parser->dataStart), xsInteger(size));
		parser->dataStart += size;
	}
}

void JSONBase64Parser_done(xsMachine* the)
{
	JSONBase64Parser parser = xsmcGetHostDataValidate(xsThis, (void *)&JSONBase64ParserHooks);
	uint32_t size = parser->dataStop - parser->dataStart;
	if (size > 0) {
		xsCall2(xsReference(parser->target), xsID_onBase64, xsInteger(parser->dataStart), xsInteger(size));
		parser->dataStart += size;
	}
}

