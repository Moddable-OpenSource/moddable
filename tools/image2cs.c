/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "mc.xs.h"

typedef struct {
	uint16_t* begin;
	uint16_t* current;
	uint16_t* end;
	uint32_t delta;
	uint32_t length;
	uint16_t* from;
	uint16_t* to;
	uint16_t* first;
	uint16_t* last;
	int pass;
	int transparentIndex;
} txRow;

static int* bufferColorIndexes(int* prefixes, int* colorIndexes, int* buffer, int code, int clearCode)
{
	while (code > clearCode) {
		*buffer++ = colorIndexes[code];
		code = prefixes[code];
	}
	*buffer = code;
	return buffer;
}

static uint16_t pad(uint16_t dimension, uint16_t multiple)
{
	uint16_t overflow = dimension & (multiple - 1);
	if (overflow)
		dimension += multiple - overflow;
	return dimension;
}

static void poke(txRow* row, uint16_t* colorTable, int index)
{
    if ((row->from <= row->current) && (row->current < row->to)) {
        if (index == row->transparentIndex) {
            row->current++;
        }
        else {
            *row->current++ = colorTable[index];
        }
        if (row->current == row->end) {
          	if (row->pass == 0) {
            	row->begin += row->length;
 			}
         	else if (row->pass == 1) {
				row->begin += 8 * row->length;
				if (row->begin >= row->last) {
					row->begin = row->first + 4 * row->length;
					row->pass++;
				}
			}
			else if (row->pass == 2) {
				row->begin += 8 * row->length;
				if (row->begin >= row->last) {
					row->begin = row->first + 2 * row->length;
					row->pass++;
				}
			}
			else if (row->pass == 3) {
				row->begin += 4 * row->length;
				if (row->begin >= row->last) {
					row->begin = row->first + 1 * row->length;
					row->pass++;
				}
			}
			else if (row->pass == 4) {
				row->begin += 2 * row->length;
			}
            row->current = row->begin;
            row->end = row->begin + row->delta;
       }
    }
}

#define MAX_CODE 4096

#define BYTE ((bytes < bytesLimit) ? *bytes++ : (xsUnknownError("invalid GIF buffer"), 0))

void Tool_readGIF(xsMachine* the)
{
	static uint8_t byteMasks[8] = { 
		0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 
	};
	static int codeMasks[16] = { 
		0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
		0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000
	};
    uint16_t globalColorTable[256];
    uint16_t localColorTable[256];

	uint8_t* bytesInitial;
	uint8_t* bytesLimit;
	uint8_t* bytes;
	uint32_t bufferSize;
	uint32_t bufferOffset;
	
	txRow row;
	
	uint8_t byte;
	uint16_t globalWidth;
	uint16_t globalHeight;
//	uint8_t backgroundColorIndex;
	
	uint8_t count;
	
	uint16_t delay = 0;
//	uint8_t disposalMethod = 0;
	uint8_t transparentColorFlag = 0;
	uint8_t transparentColorIndex = 0;
	
	uint16_t* screen;
	uint16_t* colorTable;
	
	xsVars(2);
	bufferSize = xsToInteger(xsGet(xsArg(0), xsID_byteLength));
	if (xsIsInstanceOf(xsArg(0), xsArrayBufferPrototype))
		bytesInitial = xsToArrayBuffer(xsArg(0));
	else
		bytesInitial = xsGetHostData(xsArg(0));
	bytesLimit = bytesInitial + bufferSize;
	bytes = bytesInitial;
	
	if (bufferSize < 6)
		xsUnknownError("invalid GIF buffer");
	if ((bytes[0] != 'G') || (bytes[1] != 'I') ||(bytes[2] != 'F'))
		xsUnknownError("invalid GIF signature");
	if ((bytes[3] != '8') || ((bytes[4] != '7') && (bytes[4] != '9')) || (bytes[5] != 'a'))
		xsUnknownError("invalid GIF version");
	bytes += 6;
	globalWidth = BYTE;
	globalWidth += BYTE << 8;
	globalHeight = BYTE;
	globalHeight += BYTE << 8;

	globalWidth = pad(globalWidth, 4);
	globalHeight = pad(globalHeight, 4);
	
	byte = BYTE;
	/* backgroundColorIndex = */ BYTE;
	bytes++;
	if (byte & 0x80) {
		int c = 2 << (byte & 0x07), i;
        for (i = 0; i < c; i++) {
        	uint16_t r = BYTE;
        	uint16_t g = BYTE;
        	uint16_t b = BYTE;
        	globalColorTable[i] = (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        }
	}
		
	bufferOffset = bytes - bytesInitial;
	xsResult = xsNewArray(0);
	xsDefine(xsResult, xsID_width, xsInteger((xsIntegerValue)globalWidth), xsDefault);
	xsDefine(xsResult, xsID_height, xsInteger((xsIntegerValue)globalHeight), xsDefault);
	if (xsIsInstanceOf(xsArg(0), xsArrayBufferPrototype))
		bytesInitial = xsToArrayBuffer(xsArg(0));
	else
		bytesInitial = xsGetHostData(xsArg(0));
	bytesLimit = bytesInitial + bufferSize;
	bytes = bytesInitial + bufferOffset;
		
	for (;;) {
		byte = BYTE;
		if (byte == 0x21) {
			byte = BYTE;
			if (byte == 0xF9) {
				bytes++;
				byte = BYTE;
//				disposalMethod = ((byte & 0x1C) >> 2);
				transparentColorFlag = byte & 0x01;
				delay = BYTE;
				delay += BYTE << 8;
				transparentColorIndex = BYTE;
				bytes++;
			}
			else if ((byte == 0x01) || (byte == 0xFE) || (byte == 0xFF)) {
				while ((count = BYTE))
					bytes += count;
			}
			else {
				xsUnknownError("invalid GIF extension");
			}
			
		}
		else if (byte== 0x2C) {
			int prefixes[MAX_CODE];
			int colorIndexes[MAX_CODE];
			int buffer[MAX_CODE];
			int* first = buffer;
			int* last;

			uint16_t x, y, width, height;
			uint8_t interlaceFlag;
	
			int colorIndexBitCount, codeBitCount, codeBitIndex, byteBitIndex;
			int codeAvailable, clearCode, endCode, nextCode, newCode, oldCode, colorIndex;
		
			bufferOffset = bytes - bytesInitial;
			xsVar(0) = xsArrayBuffer(NULL, (globalWidth * globalHeight * 2));
			xsDefine(xsVar(0), xsID_delay, xsInteger((xsIntegerValue)delay * 10), xsDefault);
			
			xsCall1(xsResult, xsID_push, xsVar(0));
			if (xsIsInstanceOf(xsArg(0), xsArrayBufferPrototype))
				bytesInitial = xsToArrayBuffer(xsArg(0));
			else
				bytesInitial = xsGetHostData(xsArg(0));
			bytesLimit = bytesInitial + bufferSize;
			bytes = bytesInitial + bufferOffset;
		
			screen = xsToArrayBuffer(xsVar(0));
			if (xsTest(xsVar(1))) {
				uint16_t* former = xsToArrayBuffer(xsVar(1));
				c_memcpy(screen, former, (globalWidth * globalHeight * 2));
			}
			else
				c_memset(screen, 0xFF, (globalWidth * globalHeight * 2));
			xsVar(1) = xsVar(0);
		
			x = BYTE;
			x += BYTE << 8;
			y = BYTE;
			y += BYTE << 8;
			width = BYTE;
			width += BYTE << 8;
			height = BYTE;
			height += BYTE << 8;
			byte = BYTE;
			interlaceFlag = (byte & 0x40) >> 6;
			if (byte & 0x80) {
				int c = 2 << (byte & 0x07), i;
				for (i = 0; i < c; i++) {
					uint16_t r = BYTE;
					uint16_t g = BYTE;
					uint16_t b = BYTE;
					localColorTable[i] = (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
				}
				colorTable = localColorTable;
			}
			else
				colorTable = globalColorTable;
			
			row.begin = screen + (((y * globalWidth) + x));
			row.current = row.begin;
			row.end = row.begin + width;
			row.delta = width;
			row.length = globalWidth;
			row.from = screen;
			row.to = screen + (globalWidth * globalHeight);
			row.transparentIndex = (transparentColorFlag) ? transparentColorIndex : -1;
			if (interlaceFlag) {
				row.pass = 1;
				row.first = row.begin;
				row.last = row.begin + (height * row.length);
			}
			else
				row.pass = 0;

			colorIndexBitCount = BYTE;
			clearCode = 1 << colorIndexBitCount;
			endCode = clearCode + 1;
			
            codeAvailable = 1 << colorIndexBitCount;
			codeBitCount = colorIndexBitCount + 1;
			nextCode = clearCode + 2;
			oldCode = -1;
			
			newCode = 0;
			codeBitIndex = 0;
			while ((count = BYTE)) {
				while (count) {
					byte = BYTE;
					count--;
					for (byteBitIndex = 0; byteBitIndex < 8; byteBitIndex++) {
						if (byte & byteMasks[byteBitIndex])
							newCode |= codeMasks[codeBitIndex];
						codeBitIndex++;
						if (codeBitIndex == codeBitCount) {
							if (newCode == endCode) {
								bytes += count;
								count = 0;
								break;
							}
							else if (newCode == clearCode) {
								codeAvailable = (1 << colorIndexBitCount) - 1;
								codeBitCount = colorIndexBitCount + 1;
                                nextCode = clearCode + 2;
								oldCode = -1;
							}
                            else {
                                codeAvailable--;
                                if (oldCode == -1) {
                                    colorIndex = oldCode = newCode;
                                    poke(&row, colorTable, colorIndex);
                                }
                                else {
                                    if (newCode >= nextCode) {
                                        *first = colorIndex;
                                        last = bufferColorIndexes(prefixes, colorIndexes, first + 1, oldCode, clearCode);
                                    }
                                    else {
                                        last = bufferColorIndexes(prefixes, colorIndexes, first, newCode, clearCode);
                                    }
                                    colorIndex = *last;
                                    while (last >= first)
                                        poke(&row, colorTable, *last--);
                                    if (nextCode < MAX_CODE) {
                                        prefixes[nextCode] = oldCode;
                                        colorIndexes[nextCode] = colorIndex;
                                        nextCode++;
                                    }
                                    oldCode = newCode;
                                }
                            }
							if ((codeAvailable == 0) && (codeBitCount < 12)) {
								codeAvailable = 1 << codeBitCount;
								codeBitCount++;
							}
							codeBitIndex = 0;
                            newCode = 0;
						}
					}
				}
			}
		}
		else if (byte == 0x3B) {
			break;
		}
		else
			xsUnknownError("invalid GIF extension");
	}
}

