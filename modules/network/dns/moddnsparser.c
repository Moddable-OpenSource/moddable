/*
 * Copyright (c) 2018-2022  Moddable Tech, Inc.
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

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values

#include <string.h>

static void *getPacket(xsMachine *the, uint8_t **end)
{
	void *result;
	xsUnsignedValue length;

	xsmcGet(xsResult, xsThis, xsID_buffer);
	xsmcGetBufferReadable(xsResult, (void **)&result, &length);
	if (end)
		*end = length + (uint8_t *)result;
	return result;
}

static void *getQuestionByIndex(xsMachine *the, uint8_t index)
{
	uint8_t *packetEnd;
	uint8_t *position = getPacket(the, &packetEnd);
	uint16_t questions = (position[4] << 8) | position[5];

	if ((index != 0xff) && (index >= questions))
		return NULL;

	if (0xff == index)
		index = questions;

	position += 12;
	while (index--) {
		while (position < packetEnd) {
			uint8_t tag = *position++;
			if (0 == tag)
				break;
			if (0xC0 == (tag & 0xC0)) {
				position += 1;
				break;
			}
			position += tag;
		}
		position += 4;
		if (position >= packetEnd)
			return NULL;
	}

	return position;
}

static void *getAnswerByIndex(xsMachine *the, uint8_t index)
{
	uint8_t *packetEnd;
	uint8_t *position = getPacket(the, &packetEnd);
	uint16_t answers = ((position[6] << 8) | position[7]) + ((position[8] << 8) | position[9]) + ((position[10] << 8) | position[11]);		// including authority and additional records
	position = getQuestionByIndex(the, 0xff);

	if (NULL == position)
		return NULL;

	if ((0xff != index) && (index >= answers))
		return NULL;

	if (0xff == index)
		index = answers;

	while (index--) {
		uint16_t rdlength;

		while (position < packetEnd) {
			uint8_t tag = *position++;
			if (0 == tag)
				break;
			if (0xC0 == (tag & 0xC0)) {
				position += 1;
				break;
			}
			position += tag;
		}
		position += 10;
		if (position >= packetEnd)
			return NULL;

		rdlength = (position[-2] << 8) | position[-1];
		position += rdlength;
		if (position >= packetEnd)
			return NULL;
	}

	return position;
}

#define kDNSQnameMaxHops 128

static int parseQname(xsMachine *the, int offset)
{
	uint8_t *packetEnd;
	uint8_t *packetStart;
	xsUnsignedValue packetLength;
	uint8_t *position;
	int qnameEndPosition = 0;
	int hops = 0;

	xsVar(1) = xsNewArray(0);	// do this before getPacket: xsNewArray can trigger GC
	packetStart = getPacket(the, &packetEnd);
	packetLength = (xsUnsignedValue)(packetEnd - packetStart);
	position = offset + packetStart;
	while (true) {
		char tmp[64];
		uint8_t tag;

		if (position >= packetEnd)
			xsUnknownError("bad name");
		tag = *position++;
		offset += 1;
		if (0 == tag) {
			if (0 == qnameEndPosition)
				qnameEndPosition = offset;
			break;
		}
		if (0xC0 == (tag & 0xC0)) {
			int ptr;
			if (position >= packetEnd)
				xsUnknownError("bad name");
			if (0 == qnameEndPosition)
				qnameEndPosition = offset + 1;
			ptr = ((tag << 8) | *position) & 0x3FFF;
			if ((xsUnsignedValue)ptr >= packetLength)
				xsUnknownError("bad name");
			if (++hops > kDNSQnameMaxHops)
				xsUnknownError("bad name");
			offset = ptr;
			position = offset + packetStart;
			continue;
		}

		if (tag > 63)
			xsUnknownError("bad name");
		if (position + tag > packetEnd)
			xsUnknownError("bad name");
		c_memcpy(tmp, position, tag);
		offset += tag;
		xsmcSetStringBuffer(xsVar(2), tmp, tag);	// can trigger GC
		xsCall1(xsVar(1), xsID_push, xsVar(2));	// can trigger GC
		// re-fetch packet pointer since GC may have moved memory
		packetStart = getPacket(the, &packetEnd);
		packetLength = (xsUnsignedValue)(packetEnd - packetStart);
		position = offset + packetStart;
	}

	return qnameEndPosition;
}

static void parseQuestionOrAnswer(xsMachine *the, uint8_t *position, uint8_t answer)
{
	int offset;
	uint16_t qtype, qclass;
	char tmp[256];

	if (NULL == position) {
		xsmcSetNull(xsResult);
		return;
	}

	offset = position - (uint8_t *)getPacket(the, NULL);

	xsmcVars(4);

	xsVar(0) = xsNewObject();

	offset = parseQname(the, offset);
	xsmcSet(xsVar(0), xsID_qname, xsVar(1));

	position = offset + (uint8_t *)getPacket(the, NULL);

	qtype = (position[0] << 8) | position[1];
	qclass = (position[2] << 8) | position[3];
	position += 4;
	offset += 4;

	if (answer) {
		uint32_t ttl;
		uint16_t rdlength;

		ttl = (position[0] << 24) | (position[1] << 16) | (position[2] << 8) | position[3];
		rdlength = (position[4] << 8) | position[5];

		position += 6;
		offset += 6;

		if (rdlength) {
			if (0x0001 == qtype) {	// A
				uint8_t ip[4];
				char *out;
				c_memcpy(ip, position, 4);
				xsVar(1) = xsStringBuffer(NULL, 4 * 5);
				out = xsmcToString(xsVar(1));
#ifndef __ets__
				c_snprintf(out, 4 * 5, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
#else
				itoa(ip[0], out, 10); out += c_strlen(out); *out++ = '.';
				itoa(ip[1], out, 10); out += c_strlen(out); *out++ = '.';
				itoa(ip[2], out, 10); out += c_strlen(out); *out++ = '.';
				itoa(ip[3], out, 10); out += c_strlen(out); *out = 0;
#endif
				xsmcSet(xsVar(0), xsID_rdata, xsVar(1));
			}
			else
			if (0x000C == qtype) {	// PTR
				parseQname(the, offset);
				xsmcSet(xsVar(0), xsID_rdata, xsVar(1));
			}
			else
#if 0
			if (0x001C == qtype) {	// AAAA
				uint8_t ip[16];
				char *out;
				c_memcpy(ip, position, 16);
				xsVar(1) = xsStringBuffer(NULL, 8 * 5);
				out = xsmcToString(xsVar(1));
				itoa((ip[0] << 8) | ip[1], out, 16); out += c_strlen(out); *out++ = ':';
				itoa((ip[2] << 8) | ip[3], out, 16); out += c_strlen(out); *out++ = ':';
				itoa((ip[4] << 8) | ip[5], out, 16); out += c_strlen(out); *out++ = ':';
				itoa((ip[6] << 8) | ip[7], out, 16); out += c_strlen(out); *out++ = ':';
				itoa((ip[8] << 8) | ip[9], out, 16); out += c_strlen(out); *out++ = ':';
				itoa((ip[10] << 8) | ip[11], out, 16); out += c_strlen(out); *out++ = ':';
				itoa((ip[12] << 8) | ip[13], out, 16); out += c_strlen(out); *out++ = ':';
				itoa((ip[14] << 8) | ip[15], out, 16); out += c_strlen(out); *out++ = '0';
				xsmcSet(xsVar(0), xsID_rdata, xsVar(1));
			}
			else
#endif
			if (0x0021 == qtype) {	// SRV
				uint16_t priority = (position[0] << 8) | position[1];
				uint16_t weight = (position[2] << 8) | position[3];
				uint16_t port = (position[4] << 8) | position[5];
				offset += 6;

				xsVar(3) = xsNewObject();
				xsmcSet(xsVar(0), xsID_rdata, xsVar(3));
				xsmcSetInteger(xsVar(1), priority);
				xsmcSet(xsVar(3), xsID_priority, xsVar(1));
				xsmcSetInteger(xsVar(1), weight);
				xsmcSet(xsVar(3), xsID_weight, xsVar(1));
				xsmcSetInteger(xsVar(1), port);
				xsmcSet(xsVar(3), xsID_port, xsVar(1));

				parseQname(the, offset);
				xsmcSet(xsVar(3), xsID_target, xsVar(1));
			}
			else
			if (0x0010 == qtype) {	// TXT
				if (rdlength > 1) {
					uint8_t *rdataEnd;
					xsVar(1) = xsNewArray(0);
					position = offset + (uint8_t *)getPacket(the, NULL);
					rdataEnd = position + rdlength;
					while (rdlength > 1) {
						uint8_t tag;
						uint16_t advance;

						if (position >= rdataEnd)
							xsUnknownError("bad txt");
						tag = *position++;
						advance = (uint16_t)tag + 1;
						if (advance > rdlength)
							xsUnknownError("bad txt");
						if (position + tag > rdataEnd)
							xsUnknownError("bad txt");
						offset += advance;
						rdlength -= advance;
						c_memcpy(tmp, position, tag);
						xsmcSetStringBuffer(xsVar(2), tmp, tag);
						xsCall1(xsVar(1), xsID_push, xsVar(2));
						position = offset + (uint8_t *)getPacket(the, NULL);
					}
					xsmcSet(xsVar(0), xsID_rdata, xsVar(1));
				}
			}
			else {
				xsmcSetString(xsVar(1), "UNHANDLED");
				xsmcSet(xsVar(0), xsID_rdata, xsVar(1));
			}
		}

		xsmcSetInteger(xsVar(1), ttl);
		xsmcSet(xsVar(0), xsID_ttl, xsVar(1));
	}

	xsmcSetInteger(xsVar(1), qtype);
	xsmcSet(xsVar(0), xsID_qtype, xsVar(1));
	xsmcSetInteger(xsVar(1), qclass);
	xsmcSet(xsVar(0), xsID_qclass, xsVar(1));

	xsResult = xsVar(0);
}

void xs_dnspacket_get_id(xsMachine *the)
{
	uint8_t *header = getPacket(the, NULL);
	xsmcSetInteger(xsResult, (header[0] << 8) | header[1]);
}

void xs_dnspacket_get_flags(xsMachine *the)
{
	uint8_t *header = getPacket(the, NULL);
	xsmcSetInteger(xsResult, (header[2] << 8) | header[3]);
}

void xs_dnspacket_get_questions(xsMachine *the)
{
	uint8_t *header = getPacket(the, NULL);
	xsmcSetInteger(xsResult, (header[4] << 8) | header[5]);
}

void xs_dnspacket_get_answers(xsMachine *the)
{
	uint8_t *header = getPacket(the, NULL);
	xsmcSetInteger(xsResult, (header[6] << 8) | header[7]);
}

void xs_dnspacket_get_authorities(xsMachine *the)
{
	uint8_t *header = getPacket(the, NULL);
	xsmcSetInteger(xsResult, (header[8] << 8) | header[9]);
}

void xs_dnspacket_get_additionals(xsMachine *the)
{
	uint8_t *header = getPacket(the, NULL);
	xsmcSetInteger(xsResult, (header[8] << 10) | header[11]);
}

void xs_dnspacket_question(xsMachine *the)
{
	parseQuestionOrAnswer(the, getQuestionByIndex(the, xsmcToInteger(xsArg(0))), false);
}

void xs_dnspacket_answer(xsMachine *the)
{
	parseQuestionOrAnswer(the, getAnswerByIndex(the, xsmcToInteger(xsArg(0))), true);
}
