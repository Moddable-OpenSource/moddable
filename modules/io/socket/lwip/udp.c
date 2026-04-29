/*
 * Copyright (c) 2019-2026 Moddable Tech, Inc.
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
*/

#include "lwip/udp.h"
#include "lwip/raw.h"
#include "lwip/igmp.h"

#include "modLwipSafe.h"

#include "xsmc.h"			// xs bindings for microcontroller
#include "xsHost.h"
#include "modInstrumentation.h"
#include "mc.xs.h"			// for xsID_* values

#include "builtinCommon.h"

#if ESP32
	#include "esp_wifi.h"
#endif

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
	uint8_t			readablePending;
	uint8_t			closed;
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
	int ttl = -1;
	struct udp_pcb *skt;
	xsSlot *onReadable = builtinGetCallback(the, xsID_onReadable);

	CHECK_NETWORK_SAFE();

	xsmcVars(1);

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = xsmcToInteger(xsVar(0));
		if ((port < 0) || (port > 65535))
			xsRangeError("invalid port");
	}

	if (xsmcHas(xsArg(0), xsID_timeToLive)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_timeToLive);
		ttl = xsmcToInteger(xsVar(0));
		if ((ttl <= 0) || (ttl > 255))
			xsRangeError("invalid timeToLive");
	}

	builtinInitIO();
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

	if (ttl > 0)
		skt->ttl = ttl;

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
	if (!packet) {
		builtinCriticalSectionEnd();
		return;
	}
	udp->packets = packet->next;
	builtinCriticalSectionEnd();

	xsmcSetArrayBuffer(xsResult, packet->pb->payload, packet->pb->len);

	xsmcVars(1);
	xsmcSetInteger(xsVar(0), packet->port);
	xsmcSet(xsResult, xsID_port, xsVar(0));

	xsVar(0) = xsStringBuffer(NULL, 32);
	ipaddr_ntoa_r(&packet->address, xsmcToString(xsVar(0)), 32);
	xsmcSet(xsResult, xsID_address, xsVar(0));

	pbuf_free_safe(packet->pb);
	c_free(packet);
}

static void resolveIP(xsMachine *the, char *src, ip_addr_t *dst)
{
#if ESP32
	if (!ipaddr_aton(src, dst))
		xsRangeError("invalid IP address");
#else
	char ipAddress[40];
	if (c_strlen(src) >= sizeof(ipAddress))
		xsRangeError("invalid IP address");

	c_strcpy(ipAddress, src);
	if (!ipaddr_aton(ipAddress, dst))
		xsRangeError("invalid IP address");
#endif
}

void xs_udp_write(xsMachine *the)
{
	UDP udp = xsmcGetHostDataValidate(xsThis, (void *)&xsUDPHooks);
	uint16_t port;
	ip_addr_t dst;
	xsUnsignedValue byteLength;
	err_t err;
	void *buffer;

	resolveIP(the, xsmcToString(xsArg(1)), &dst);
	port = xsmcToInteger(xsArg(2));

	xsmcGetBufferReadable(xsArg(0), &buffer, &byteLength);
	udp_sendto_safe(udp->skt, buffer, byteLength, &dst, port, &err);
	if (ERR_OK != err)
		xsUnknownError("UDP send failed");

	modInstrumentationAdjust(NetworkBytesWritten, byteLength);
}

void xs_udp_add(xsMachine *the)
{
	UDP udp = xsmcGetHostDataValidate(xsThis, (void *)&xsUDPHooks);
	ip_addr_t multicastIP;

	resolveIP(the, xsmcToString(xsArg(0)), &multicastIP);

#if LWIP_IPV4 && LWIP_IPV6
	if (!IP_IS_V4(&multicastIP))
		xsUnknownError("invalid IP address");
#endif

#if ESP32
	if (0xffffffff == *(uint32_t *)&multicastIP.u_addr.ip4)
		xsUnknownError("broadcast is not multicast");

	esp_netif_t *ifc = NULL;
	while ((ifc = esp_netif_next_unsafe(ifc))) {
		esp_netif_ip_info_t info = {0};
		if (ESP_OK == esp_netif_get_ip_info(ifc, &info))
			igmp_joingroup((ip4_addr_t *)&info.ip, &multicastIP.u_addr.ip4);
	}
#elif CYW43_LWIP
	//@@ MDK - multicast
	ip_addr_t ifaddr;
	struct netif *netif;
	netif = netif_get_by_index(0);
	ifaddr.addr = netif->ip_addr.addr;
	igmp_joingroup(&ifaddr, &multicastIP);
#else
	if (0xffffffff == multicastIP.addr)
		xsUnknownError("broadcast is not multicast");

	ip_addr_t ifaddr;
	struct ip_info staIpInfo;
	wifi_get_ip_info(0, &staIpInfo);		// 0 == STATION_IF
	ifaddr.addr = staIpInfo.ip.addr;
	igmp_joingroup(&ifaddr, &multicastIP);
#endif
}

void xs_udp_remove(xsMachine *the)
{
	UDP udp = xsmcGetHostDataValidate(xsThis, (void *)&xsUDPHooks);
	ip_addr_t multicastIP;

	resolveIP(the, xsmcToString(xsArg(0)), &multicastIP);

#if ESP32
	esp_netif_t *ifc = NULL;
	while ((ifc = esp_netif_next_unsafe(ifc))) {
		esp_netif_ip_info_t info = {0};
		if (ESP_OK == esp_netif_get_ip_info(ifc, &info))
			igmp_leavegroup((ip4_addr_t *)&info.ip, &multicastIP.u_addr.ip4);
	}
#elif CYW43_LWIP
	//@@ MDK - multicast
	ip_addr_t ifaddr;
	struct netif *netif;
	netif = netif_get_by_index(0);
	ifaddr.addr = netif->ip_addr.addr;
	igmp_leavegroup(&ifaddr, &multicastIP);
#else
	ip_addr_t ifaddr;
	struct ip_info staIpInfo;
	wifi_get_ip_info(0, &staIpInfo);		// 0 == STATION_IF
	ifaddr.addr = staIpInfo.ip.addr;
	igmp_leavegroup(&ifaddr, &multicastIP);
#endif
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
	uint8_t readablePending = udp->readablePending;
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
	}

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
