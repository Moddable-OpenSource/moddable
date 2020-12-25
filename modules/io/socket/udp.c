/*
 * Copyright (c) 2019-2020  Moddable Tech, Inc.
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
		unsafe on ESP32 - assumes single thread
*/

#include "lwip/udp.h"
#include "lwip/raw.h"

#include "xsmc.h"			// xs bindings for microcontroller
#ifdef __ets__
	#include "xsHost.h"		// esp platform support
#else
	#error - unsupported platform
#endif
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
	uint8_t			hasOnReadable;
	xsMachine		*the;
	xsSlot			onReadable;
};
typedef struct UDPRecord UDPRecord;
typedef struct UDPRecord *UDP;

static void udpReceive(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static void udpDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

void xs_udp_constructor(xsMachine *the)
{
	UDP udp;
	int port = 0;
	struct udp_pcb *skt;

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

	skt = udp_new();
	if (!skt)
		xsRangeError("no socket");

	if (udp_bind(skt, IP_ADDR_ANY, port)) {
		udp_remove(skt);
		xsUnknownError("bind failed");
	}

	udp = c_calloc(1, sizeof(UDPRecord));
	if (!udp) {
		udp_remove(skt);
		xsRangeError("no memory");
	}

	xsmcSetHostData(xsThis, udp);
	udp->skt = skt;
	udp->obj = xsThis;
	udp->the = the;
	xsRemember(udp->obj);

	udp_recv(skt, (udp_recv_fn)udpReceive, udp);

	if (builtinGetCallback(the, xsID_onReadable, &udp->onReadable)) {
		udp->hasOnReadable = 1;
		xsRemember(udp->onReadable);
	}
}

void xs_udp_destructor(void *data)
{
	UDP udp = data;
	if (!udp) return;

	while (udp->packets) {
		UDPPacket packet = udp->packets;
		udp->packets = packet->next;
		pbuf_free(packet->pb);
		c_free(packet);
	}

	udp_remove(udp->skt);

	c_free(data);
}

void xs_udp_close(xsMachine *the)
{
	UDP udp = xsmcGetHostData(xsThis);
	if (!udp) return;

	xsmcSetHostData(xsThis, NULL);
	xsForget(udp->obj);
	if (udp->hasOnReadable)
		xsForget(udp->onReadable);
	xs_udp_destructor(udp);
}

void xs_udp_read(xsMachine *the)
{
	UDP udp = xsmcGetHostData(xsThis);
	UDPPacket packet = udp->packets;

	if (!packet)
		return;

	xsmcVars(1);

	xsmcSetArrayBuffer(xsResult, packet->pb->payload, packet->pb->len);
	pbuf_free(packet->pb);
	udp->packets = packet->next;
	c_free(packet);

	xsmcSetInteger(xsVar(0), packet->port);
	xsmcSet(xsResult, xsID_port, xsVar(0));

	xsVar(0) = xsStringBuffer(NULL, 32);
	ipaddr_ntoa_r(&packet->address, xsmcToString(xsVar(0)), 32);
	xsmcSet(xsResult, xsID_address, xsVar(0));
}

void xs_udp_write(xsMachine *the)
{
	UDP udp = xsmcGetHostData(xsThis);
	char temp[32];
	uint16 port = xsmcToInteger(xsArg(1));
	ip_addr_t dst;
	int byteLength;
	err_t err;
	struct pbuf *pb;

	xsmcToStringBuffer(xsArg(0), temp, sizeof(temp));
	if (!ipaddr_aton(temp, &dst))
		xsRangeError("invalid IP address");

	if (xsmcIsInstanceOf(xsArg(2), xsTypedArrayPrototype))
		xsmcGet(xsArg(2), xsArg(2), xsID_buffer);
	byteLength = xsmcGetArrayBufferLength(xsArg(2));

	pb = pbuf_alloc(PBUF_TRANSPORT, byteLength, PBUF_RAM);
	if (!pb)
		xsRangeError("no memory");

	c_memcpy(pb->payload, xsmcToArrayBuffer(xsArg(2)), byteLength);
	err = udp_sendto(udp->skt, pb, &dst, port);
	pbuf_free(pb);
	if (ERR_OK != err)
		xsUnknownError("UDP send failed");
}

void udpReceive(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	UDP udp = arg;
	UDPPacket packet;

#ifdef mxDebug
	if (fxInNetworkDebugLoop(udp->the)) {
		pbuf_free(p);
		return;
	}
#endif

	packet = c_malloc(sizeof(UDPPacketRecord));
	if (!packet) {	//@@ report dropped error?
		pbuf_free(p);
		return;
	}

	packet->next = NULL;
	packet->pb = p;
	packet->port = port;
	packet->address = *addr;

	if (udp->packets) {
		UDPPacket walker;

		for (walker = udp->packets; walker->next; walker = walker->next)
			;
		walker->next = packet;
	}
	else {
		udp->packets = packet;
		if (udp->hasOnReadable)
			modMessagePostToMachine(udp->the, NULL, 0, udpDeliver, udp);
	}
}

void udpDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	UDP udp = refcon;
	int count;
	UDPPacket walker = udp->packets;

	if (!walker)
		return;

	for (count = 0; NULL != walker; count++, walker = walker->next)
		;

	xsBeginHost(the);
		xsmcSetInteger(xsResult, count);
		xsCallFunction1(udp->onReadable, udp->obj, xsResult);
	xsEndHost(the);
}
