/*
 * Copyright (c) 2019-2025 Moddable Tech, Inc.
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
	UDP socket - uing zephyr native API

		try with CONFIG_NET_CONTEXT_SYNC_RECV set to n
		multicast group net_ipv4_igmp_join & net_ipv4_igmp_leave
*/

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"
#include "modInstrumentation.h"
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

#include <zephyr/net/udp.h>

struct UDPPacketRecord {
	struct UDPPacketRecord	*next;
	struct net_pkt				*pkt;
	uint16_t						port;
	struct net_addr			address;
};
typedef struct UDPPacketRecord UDPPacketRecord;
typedef struct UDPPacketRecord *UDPPacket;

struct UDPRecord {
	struct net_context	*context;
	UDPPacket				packets;
	xsSlot					obj;
	xsMachine				*the;
	xsSlot					*onReadable;
	uint8_t					readablePending:1;
	uint8_t					closed:1;
};
typedef struct UDPRecord UDPRecord;
typedef struct UDPRecord *UDP;

static void udpReceive(struct net_context *context, struct net_pkt *pkt, union net_ip_header *ip_hdr, union net_proto_header *proto_hdr, int status, void *user_data);
static void udpDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void xs_udp_mark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks ICACHE_RODATA_ATTR xsUDPHooks = {
	xs_udp_destructor,
	xs_udp_mark,
	NULL
};

void xs_udp_constructor(xsMachine *the)
{
	xsSlot *onReadable = builtinGetCallback(the, xsID_onReadable);

	xsmcVars(1);

	int port = 0;
	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = xsmcToInteger(xsVar(0));
		if ((port < 0) || (port > 65535))
			xsRangeError("invalid port");
	}

	builtinInitializeTarget(the);
	if (kIOFormatBuffer != builtinInitializeFormat(the, kIOFormatBuffer))
		xsRangeError("invalid format");

	struct net_context *context;
	int result = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &context);
	if (result < 0)
		xsUnknownError("no context");

	struct sockaddr_in bind_addr;
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = INADDR_ANY;
	bind_addr.sin_port = htons(port);
	result = net_context_bind(context, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
	if (result < 0) {
		net_context_put(context);
		xsUnknownError("bind failed");
	}

	UDP udp = c_calloc(1, sizeof(UDPRecord));
	if (!udp) {
		net_context_put(context);
		xsRangeError("no memory");
	}

	xsmcSetHostData(xsThis, udp);
	udp->context = context;
	udp->obj = xsThis;
	udp->the = the;
	xsRemember(udp->obj);

	result = net_context_recv(context, udpReceive, K_NO_WAIT, udp);
	if (result < 0)
		xsUnknownError("error");

	udp->onReadable = onReadable;

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsUDPHooks);

	modInstrumentationAdjust(NetworkSockets, +1);
}

void xs_udp_destructor(void *data)
{
	UDP udp = data;
	if (!udp) return;

	net_context_put(udp->context);

	while (udp->packets) {
		UDPPacket packet = udp->packets;
		udp->packets = packet->next;
		net_pkt_unref(packet->pkt);
		c_free(packet);
	}

	if (!udp->closed || !udp->readablePending)		// if closed is not set, was called from destructor when killing VM
		c_free(data);

	modInstrumentationAdjust(NetworkSockets, -1);
}

void xs_udp_close(xsMachine *the)
{
	UDP udp = xsmcGetHostData(xsThis);
	if (udp && xsmcGetHostDataValidate(xsThis, (void *)&xsUDPHooks)) {
		udp->closed = true;
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
	if (packet)
		udp->packets = packet->next;
	builtinCriticalSectionEnd();
	if (!packet)
		return;

	uint16_t packetLength = net_pkt_remaining_data(packet->pkt);	
	void *buffer = xsmcSetArrayBuffer(xsResult, C_NULL, packetLength);
	if (0 != net_pkt_read(packet->pkt, buffer, packetLength)) {
		net_pkt_unref(packet->pkt);
		c_free(packet);
		xsUnknownError("net_pkt_read failed");
	}

	xsmcVars(1);
	xsmcSetInteger(xsVar(0), packet->port);
	xsmcSet(xsResult, xsID_port, xsVar(0));

	xsVar(0) = xsStringBuffer(NULL, NET_IPV4_ADDR_LEN + 1);
	net_addr_ntop(AF_INET, &packet->address, xsmcToString(xsVar(0)), NET_IPV4_ADDR_LEN);
	xsmcSet(xsResult, xsID_address, xsVar(0));

	net_pkt_unref(packet->pkt);
	c_free(packet);
}

void xs_udp_write(xsMachine *the)
{
	UDP udp = xsmcGetHostDataValidate(xsThis, (void *)&xsUDPHooks);

	uint16_t port = xsmcToInteger(xsArg(2));
	struct sockaddr_in dst;
	dst.sin_family = AF_INET;
	dst.sin_port = htons(port);
	if (net_addr_pton(AF_INET, xsmcToString(xsArg(1)), &dst.sin_addr) < 0)
		xsRangeError("invalid IP dst");

	xsUnsignedValue byteLength;
	void *buffer;
	xsmcGetBufferReadable(xsArg(0), &buffer, &byteLength);
	int result = net_context_sendto(udp->context, buffer, byteLength, (struct sockaddr *)&dst, sizeof(dst), NULL, K_NO_WAIT, NULL);
	if (result < 0)
		xsUnknownError("UDP send failed");

	modInstrumentationAdjust(NetworkBytesWritten, byteLength);
}

static void udpReceive(struct net_context *context, struct net_pkt *pkt, union net_ip_header *ip_hdr, union net_proto_header *proto_hdr, int status, void *user_data)
{
	UDP udp = user_data;
	UDPPacket packet;

#ifdef mxDebug
	if (fxInNetworkDebugLoop(udp->the)) {
		net_pkt_unref(pkt);
		return;
	}
#endif

	packet = c_malloc(sizeof(UDPPacketRecord));
	if (!packet) {
		net_pkt_unref(pkt);
		return;
	}

	packet->next = NULL;
	packet->pkt = pkt;
	packet->port = ntohs(proto_hdr->udp->src_port);
	packet->address = *(struct net_addr *)ip_hdr->ipv4->src;

	modInstrumentationAdjust(NetworkBytesRead, net_pkt_get_len(pkt));

	builtinCriticalSectionBegin();
	uint8_t readablePending = udp->readablePending;
	if (udp->packets) {
		UDPPacket walker;

		for (walker = udp->packets; walker->next; walker = walker->next)
			;
		walker->next = packet;
	}
	else
		udp->packets = packet;
	builtinCriticalSectionEnd();

	if (!readablePending && udp->onReadable) {
		udp->readablePending = true;
		modMessagePostToMachine(udp->the, NULL, 0, udpDeliver, udp);
	}
}

void udpDeliver(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	UDP udp = refcon;
	int count;

	if (udp->closed) {
		c_free(udp);
		return;
	}

	builtinCriticalSectionBegin();
	udp->readablePending = false;
	UDPPacket walker = udp->packets;
	for (count = 0; NULL != walker; count++, walker = walker->next)
		;
	builtinCriticalSectionEnd();
	if (!count)
		return;

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
