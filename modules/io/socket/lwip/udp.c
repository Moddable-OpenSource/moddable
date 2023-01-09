/*
 * Copyright (c) 2019-2023 Moddable Tech, Inc.
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
	UDP socket - uing lwip low level callback API

	To do:

		- multicast
*/

#include "lwip/udp.h"
#include "lwip/raw.h"

#include "modLwipSafe.h"

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"
#include "modInstrumentation.h"
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

struct UDPPacketRecord {
	struct UDPPacketRecord *next;
	struct pbuf		*pb;
	uint16_t		port;
	ip_addr_t		address;
};
typedef struct UDPPacketRecord UDPPacketRecord;
typedef struct UDPPacketRecord *UDPPacket;

struct UDPRecord {
	struct udp_pcb	*skt;
	UDPPacket		packets;
	xsSlot			obj;
	xsMachine		*the;
	xsSlot			*onReadable;
};
typedef struct UDPRecord UDPRecord;
typedef struct UDPRecord *UDP;

static void udpReceive(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static void udpDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void xs_udp_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks ICACHE_RODATA_ATTR xsUDPHooks = {
	xs_udp_destructor,
	xs_udp_mark,
	NULL
};

void xs_udp_constructor(xsMachine *the)
{
	UDP udp;
	int port = 0;
	struct udp_pcb *skt;
	xsSlot *onReadable = builtinGetCallback(the, xsID_onReadable);

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = xsmcToInteger(xsVar(0));
		if ((port < 0) || (port > 65535))
			xsRangeError("invalid port");
	}

	builtinInitializeTarget(the);
	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("invalid format");

	skt = udp_new_safe();
	if (!skt)
		xsRangeError("no socket");

	if (udp_bind_safe(skt, IP_ADDR_ANY, port)) {
		udp_remove_safe(skt);
		xsUnknownError("bind failed");
	}

	udp = c_calloc(1, sizeof(UDPRecord));
	if (!udp) {
		udp_remove_safe(skt);
		xsRangeError("no memory");
	}

	xsmcSetHostData(xsThis, udp);
	udp->skt = skt;
	udp->obj = xsThis;
	udp->the = the;
	xsRemember(udp->obj);

	udp_recv(skt, (udp_recv_fn)udpReceive, udp);

	udp->onReadable = onReadable;

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsUDPHooks);

	modInstrumentationAdjust(NetworkSockets, +1);
}

void xs_udp_destructor(void *data)
{
	UDP udp = data;
	if (!udp) return;

	udp_remove_safe(udp->skt);

	while (udp->packets) {
		UDPPacket packet = udp->packets;
		udp->packets = packet->next;
		pbuf_free_safe(packet->pb);
		c_free(packet);
	}

	c_free(data);

	modInstrumentationAdjust(NetworkSockets, -1);
}

void xs_udp_close(xsMachine *the)
{
	UDP udp = xsmcGetHostData(xsThis);
	if (udp && xsmcGetHostDataValidate(xsThis, (void *)&xsUDPHooks)) {
		xsForget(udp->obj);
		xs_udp_destructor(udp);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_udp_read(xsMachine *the)
{
	UDP udp = xsmcGetHostDataValidate(xsThis, (void *)&xsUDPHooks);
	UDPPacket packet;

	builtinCriticalSectionBegin();
	packet = udp->packets;
	if (!packet) {
		builtinCriticalSectionEnd();
		return;
	}
	builtinCriticalSectionEnd();

	xsmcSetArrayBuffer(xsResult, packet->pb->payload, packet->pb->len);

	xsmcVars(1);
	xsmcSetInteger(xsVar(0), packet->port);
	xsmcSet(xsResult, xsID_port, xsVar(0));

	xsVar(0) = xsStringBuffer(NULL, 32);
	ipaddr_ntoa_r(&packet->address, xsmcToString(xsVar(0)), 32);
	xsmcSet(xsResult, xsID_address, xsVar(0));

	builtinCriticalSectionBegin();
	udp->packets = packet->next;
	builtinCriticalSectionEnd();

	pbuf_free_safe(packet->pb);
	c_free(packet);
}

void xs_udp_write(xsMachine *the)
{
	UDP udp = xsmcGetHostDataValidate(xsThis, (void *)&xsUDPHooks);
	char temp[32];
	uint16_t port = xsmcToInteger(xsArg(1));
	ip_addr_t dst;
	xsUnsignedValue byteLength;
	err_t err;
	void *buffer;

	xsmcToStringBuffer(xsArg(0), temp, sizeof(temp));
	if (!ipaddr_aton(temp, &dst))
		xsRangeError("invalid IP address");

	xsmcGetBufferReadable(xsArg(2), &buffer, &byteLength);
	udp_sendto_safe(udp->skt, buffer, byteLength, &dst, port, &err);
	if (ERR_OK != err)
		xsUnknownError("UDP send failed");

	modInstrumentationAdjust(NetworkBytesWritten, byteLength);
}

void udpReceive(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	UDP udp = arg;
	UDPPacket packet;

#ifdef mxDebug
	if (fxInNetworkDebugLoop(udp->the)) {
		pbuf_free_safe(p);
		return;
	}
#endif

	packet = c_malloc(sizeof(UDPPacketRecord));
	if (!packet) {
		pbuf_free_safe(p);
		return;
	}

	packet->next = NULL;
	packet->pb = p;
	packet->port = port;
	packet->address = *addr;

	modInstrumentationAdjust(NetworkBytesRead, p->len);

	builtinCriticalSectionBegin();
	if (udp->packets) {
		UDPPacket walker;

		for (walker = udp->packets; walker->next; walker = walker->next)
			;
		walker->next = packet;

		builtinCriticalSectionEnd();
	}
	else {
		udp->packets = packet;
		builtinCriticalSectionEnd();

		if (udp->onReadable)
			modMessagePostToMachine(udp->the, NULL, 0, udpDeliver, udp);
	}
}

void udpDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	UDP udp = refcon;
	int count;
	UDPPacket walker;

	builtinCriticalSectionBegin();
	walker = udp->packets;
	if (!walker) {
		builtinCriticalSectionEnd();
		return;
	}
	for (count = 0; NULL != walker; count++, walker = walker->next)
		;
	builtinCriticalSectionEnd();

	xsBeginHost(the);
		xsmcSetInteger(xsResult, count);
		xsCallFunction1(xsReference(udp->onReadable), udp->obj, xsResult);
	xsEndHost(the);
}

void xs_udp_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	UDP udp = it;

	if (udp->onReadable)
		(*markRoot)(the, udp->onReadable);
}
