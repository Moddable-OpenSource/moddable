/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
#include "xsesp.h"
#include "modInstrumentation.h"
#include "mc.xs.h"			// for xsID_ values

#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/udp.h"

#include "modSocket.h"
#include "modTimer.h"

#if ESP32
	#include "lwip/priv/tcpip_priv.h"
	#include "esp_wifi.h"
	typedef int8_t int8;
	typedef uint8_t uint8;
	typedef int16_t int16;
	typedef uint16_t uint16;

	typedef ip_addr_t ip_addr;
#else
	#define IP_ADDR4 IP4_ADDR
#endif

typedef struct xsSocketRecord xsSocketRecord;
typedef xsSocketRecord *xsSocket;

#define kTCP (0)
#define kUDP (1)
#define kTCPListener (2)

#define kPendingConnect (1 << 0)
#define kPendingError (1 << 1)
#define kPendingDisconnect (1 << 2)
#define kPendingReceive (1 << 3)
#define kPendingSent (1 << 4)
#define kPendingAcceptListener (1 << 5)
#define kPendingClose (1 << 6)
#define kPendingOutput (1 << 7)

struct xsSocketUDPRemoteRecord {
	uint16_t			port;
	ip_addr_t			address;
};
typedef struct xsSocketUDPRemoteRecord xsSocketUDPRemoteRecord;
typedef xsSocketUDPRemoteRecord *xsSocketUDPRemote;

#define kReadQueueLength (8)
struct xsSocketRecord {
	xsSocket			next;
	xsMachine			*the;

	xsSlot				obj;
	struct tcp_pcb		*skt;

	int8				useCount;
	uint8				kind;
	uint8				pending;
	uint8				writeDisabled;

	// above here same as xsListenerRecord

	struct udp_pcb		*udp;

	struct pbuf			*reader[kReadQueueLength];

	uint32_t			outstandingSent;

	unsigned char		*buf;
	struct pbuf			*pb;
	uint16				bufpos;
	uint16				buflen;
	uint16				port;
	uint8				constructed;
	uint8				remoteCount;
	uint8				suspended;

	uint8				suspendedError;		// could overload suspended
	uint8				suspendedDisconnect;
	uint16				suspendedBufpos;
	struct pbuf			*suspendedBuf;
	struct pbuf			*suspendedFragment;

	xsSocketUDPRemoteRecord
						remote[1];
};

typedef struct xsListenerRecord xsListenerRecord;
typedef xsListenerRecord *xsListener;

#define kListenerPendingSockets (4)
struct xsListenerRecord {
	xsListener			next;
	xsMachine			*the;

	xsSlot				obj;
	struct tcp_pcb		*skt;

	int8				useCount;
	uint8				kind;
	uint8				pending;
	uint8				writeDisabled;

	// above here same as xsSocketRecord

	xsSocket			accept[kListenerPendingSockets];
};

static xsSocket gSockets;		// N.B. this list contains both sockets and listeners

void xs_socket_destructor(void *data);

static void socketSetPending(xsSocket xss, uint8_t pending);
static void socketClearPending(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

static void socketMsgConnect(xsSocket xss);
static void socketMsgDisconnect(xsSocket xss);
static void socketMsgError(xsSocket xss);
static void socketMsgDataReceived(xsSocket xss);
static void socketMsgDataSent(xsSocket xss);

#if ESP32
	static void didFindDNS(const char *name, const ip_addr_t *ipaddr, void *arg);
#else
	static void didFindDNS(const char *name, ip_addr_t *ipaddr, void *arg);
#endif
static void didError(void *arg, err_t err);
static err_t didConnect(void * arg, struct tcp_pcb * tpcb, err_t err);
static err_t didReceive(void * arg, struct tcp_pcb * pcb, struct pbuf * p, err_t err);
static err_t didSend(void *arg, struct tcp_pcb *pcb, u16_t len);
static void didReceiveUDP(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

static uint8 parseAddress(char *address, uint8 *ip);

/*
	shims to call lwip in its own thread
*/

#if ESP32

typedef struct {
	struct tcpip_api_call		call;

	err_t						err;
	xsSocket					xss;
	struct tcp_pcb				*tcpPCB;
	struct udp_pcb				*udpPCB;
	ip_addr_t					*ipaddr;
	ip_addr_t					addr;
	u16_t						port;
	tcp_connected_fn			connected;
	struct pbuf					*pbuf;
	const void					*data;
	u16_t						len;
	u8_t						flags;
	char	 					*hostname;
	dns_found_callback 			found;
	void						*callback_arg;
} LwipMsgRecord, *LwipMsg;

static err_t tcp_new_INLWIP(struct tcpip_api_call *tcpMsg)
{
	LwipMsg msg = (LwipMsg)tcpMsg;
	msg->tcpPCB = tcp_new();
	return ERR_OK;
}

struct tcp_pcb *tcp_new_safe(void)
{
	LwipMsgRecord msg;
	tcpip_api_call(tcp_new_INLWIP, &msg.call);
	return msg.tcpPCB;
}

static void tcp_connect_INLWIP(void *ctx)
{
	LwipMsg msg = (LwipMsg)ctx;
	msg->err = tcp_connect(msg->tcpPCB, &msg->addr, msg->port, msg->connected);
	c_free(msg);
}

err_t tcp_connect_safe(xsSocket xss, const ip_addr_t *ipaddr, u16_t port, tcp_connected_fn connected)
{
	LwipMsg msg = c_malloc(sizeof(LwipMsgRecord));
	msg->tcpPCB = xss->skt,
	msg->addr = *ipaddr,
	msg->port = port,
	msg->connected = connected,
	tcpip_callback_with_block(tcp_connect_INLWIP, msg, 0);

	return ERR_OK;
}

static void pbuf_free_INLWIP(void *ctx)
{
	pbuf_free(ctx);
}

u8_t pbuf_free_safe(struct pbuf *p)
{
	tcpip_callback_with_block(pbuf_free_INLWIP, p, 1);
	return 0;
}

static err_t tcp_bind_INLWIP(struct tcpip_api_call *tcpMsg)
{
	LwipMsg msg = (LwipMsg)tcpMsg;
	msg->err = tcp_bind(msg->tcpPCB, &msg->addr, msg->port);
	return ERR_OK;
}

err_t tcp_bind_safe(struct tcp_pcb *tcpPCB, const ip_addr_t *ipaddr, u16_t port)
{
	LwipMsgRecord msg = {
		.tcpPCB = tcpPCB,
		.addr = *ipaddr,
		.port = port
	};
	tcpip_api_call(tcp_bind_INLWIP, &msg.call);
	return msg.err;
}

static err_t tcp_close_INLWIP(struct tcpip_api_call *tcpMsg)
{
	LwipMsg msg = (LwipMsg)tcpMsg;
	tcp_close(msg->tcpPCB);
}

void tcp_close_safe(struct tcp_pcb *tcpPCB)
{
	LwipMsgRecord msg = {
		.tcpPCB = tcpPCB,
	};
	tcpip_api_call(tcp_close_INLWIP, &msg.call);
}

static void tcp_output_INLWIP(void *ctx)
{
	xsSocket xss = ctx;
	if (xss->skt)
		tcp_output(xss->skt);
}

void tcp_output_safe(xsSocket xss)
{
	tcpip_callback_with_block(tcp_output_INLWIP, xss, 0);
}

static err_t tcp_write_INLWIP(struct tcpip_api_call *tcpMsg)
{
	LwipMsg msg = (LwipMsg)tcpMsg;
	if (msg->xss->skt)
		msg->err = tcp_write(msg->xss->skt, msg->data, msg->len, msg->flags);
	return ERR_OK;
}

err_t tcp_write_safe(xsSocket xss, const void *data, u16_t len, u8_t flags)
{
	LwipMsgRecord msg = {
		.xss = xss,
		.data = data,
		.len = len,
		.flags = flags
	};
	tcpip_api_call(tcp_write_INLWIP, &msg.call);
	return msg.err;
}

static void tcp_recved_INLWIP(void *ctx)
{
	LwipMsg msg = (LwipMsg)ctx;
	if (msg->xss->skt)
		tcp_recved(msg->xss->skt, msg->len);
	c_free(msg);
}

void tcp_recved_safe(xsSocket xss, u16_t len)
{
	LwipMsg msg = c_malloc(sizeof(LwipMsgRecord));
	msg->xss = xss;
	msg->len = len;
	tcpip_callback_with_block(tcp_recved_INLWIP, msg, 1);
}

static err_t tcp_listen_INLWIP(struct tcpip_api_call *tcpMsg)
{
	LwipMsg msg = (LwipMsg)tcpMsg;
	msg->tcpPCB = tcp_listen(msg->tcpPCB);
	return ERR_OK;
}

struct tcp_pcb * tcp_listen_safe(struct tcp_pcb *pcb)
{
	LwipMsgRecord msg = {
		.tcpPCB = pcb,
	};
	tcpip_api_call(tcp_listen_INLWIP, &msg.call);
	return msg.tcpPCB;
}

static err_t udp_new_INLWIP(struct tcpip_api_call *tcpMsg)
{
	LwipMsg msg = (LwipMsg)tcpMsg;
	msg->udpPCB = udp_new();
	return ERR_OK;
}

struct udp_pcb *udp_new_safe(void)
{
	LwipMsgRecord msg;
	tcpip_api_call(udp_new_INLWIP, &msg.call);
	return msg.udpPCB;
}

static err_t udp_bind_INLWIP(struct tcpip_api_call *tcpMsg)
{
	LwipMsg msg = (LwipMsg)tcpMsg;
	msg->err = udp_bind(msg->udpPCB, &msg->addr, msg->port);
	return ERR_OK;
}

err_t udp_bind_safe(struct udp_pcb *udpPCB, const ip_addr_t *ipaddr, u16_t port)
{
	LwipMsgRecord msg = {
		.udpPCB = udpPCB,
		.addr = *ipaddr,
		.port = port
	};
	tcpip_api_call(udp_bind_INLWIP, &msg.call);
	return msg.err;
}

static err_t udp_remove_INLWIP(struct tcpip_api_call *tcpMsg)
{
	LwipMsg msg = (LwipMsg)tcpMsg;
	udp_remove(msg->udpPCB);
}

void udp_remove_safe(struct udp_pcb *udpPCB)
{
	LwipMsgRecord msg = {
		.udpPCB = udpPCB,
	};
	tcpip_api_call(udp_remove_INLWIP, &msg.call);
}

static err_t udp_sendto_INLWIP(struct tcpip_api_call *tcpMsg)
{
	LwipMsg msg = (LwipMsg)tcpMsg;
	struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, msg->len, PBUF_RAM);
	c_memcpy(p->payload, msg->data, msg->len);
	msg->err = udp_sendto(msg->udpPCB, p, &msg->addr, msg->port);
	pbuf_free_safe(p);
}

void udp_sendto_safe(struct udp_pcb *udpPCB, const void *data, uint16 len, ip_addr_t *dst, uint16_t port, err_t *err)
{
	LwipMsgRecord msg = {
		.udpPCB = udpPCB,
		.data = data,
		.len = len,
		.addr = *dst,
		.port = port,
	};
	tcpip_api_call(udp_sendto_INLWIP, &msg.call);
	*err = msg.err;
}

static void dns_gethostbyname_INLWIP(void *ctx)
{
	LwipMsg msg = (LwipMsg)ctx;
	err_t err = dns_gethostbyname(msg->hostname, &msg->addr, msg->found, msg->callback_arg);
	if (ERR_INPROGRESS != err)
		(msg->found)(msg->hostname, (ERR_OK == err) ? &msg->addr : NULL, msg->callback_arg);
	c_free(msg);
}

err_t dns_gethostbyname_safe(const char *hostname, ip_addr_t *addr, dns_found_callback found, void *callback_arg)
{
	LwipMsg msg = c_malloc(sizeof(LwipMsgRecord) + c_strlen(hostname) + 1);
	msg->hostname = (char *)(msg + 1);
	c_strcpy(msg->hostname, hostname);
	msg->found = found;
	msg->callback_arg = callback_arg;
	tcpip_callback_with_block(dns_gethostbyname_INLWIP, msg, 1);
	return ERR_INPROGRESS;
}

#else
	#define tcp_new_safe tcp_new
	#define tcp_bind_safe tcp_bind
	#define tcp_listen_safe tcp_listen
	#define tcp_connect_safe(xss, ipaddr, port, connected) tcp_connect(xss->skt, ipaddr, port, connected)
	// for some reason, ESP8266 has a memory leak when using tcp_close instead of tcp_abort
	#define tcp_close_safe tcp_abort
	#define tcp_output_safe(xss) tcp_output(xss->skt)
	#define tcp_write_safe(xss, data, len, flags) tcp_write(xss->skt, data, len, flags)
	#define tcp_recved_safe(xss, len) tcp_recved(xss->skt, len)
	#define udp_new_safe udp_new
	#define udp_bind_safe udp_bind
	#define udp_remove_safe udp_remove
	#define udp_sendto_safe(skt, data, size, dst, port, err) \
		{ \
		struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM); \
		c_memcpy(p->payload, data, size); \
		*err = udp_sendto(xss->udp, p, dst, port); \
		pbuf_free_safe(p); \
		}
	#define pbuf_free_safe pbuf_free
	#define dns_gethostbyname_safe dns_gethostbyname
#endif

static void forgetSocket(xsSocket xss)
{
	xsSocket walker, prev = NULL;

	modCriticalSectionBegin();
	for (walker = gSockets; NULL != walker; prev = walker, walker = walker->next) {
		if (walker != xss)
			continue;

		if (!prev)
			gSockets = walker->next;
		else
			prev->next = walker->next;

		break;
	}
	modCriticalSectionEnd();
}

#define socketUpUseCount(the, xss) (xss->useCount += 1)

static void socketDownUseCount(xsMachine *the, xsSocket xss)
{
	xsDestructor destructor;

	modCriticalSectionBegin();
	int8 useCount = --xss->useCount;
	if (useCount > 0) {
		modCriticalSectionEnd();
		return;
	}

	xss->pending |= kPendingClose;
	modCriticalSectionEnd();
	destructor = xsGetHostDestructor(xss->obj);
	xsmcSetHostData(xss->obj, NULL);
	xsForget(xss->obj);
	(*destructor)(xss);
}

void xs_socket(xsMachine *the)
{
	xsSocket xss;
	ip_addr_t ipaddr;
	int port = 0;
	err_t err;
	char temp[DNS_MAX_NAME_LENGTH];
	unsigned char ip[4];
	int len, i;
	unsigned char waiting = 0;
	unsigned char multicastIP[4];
	int ttl = 0;

	xsmcVars(1);
	if (xsmcHas(xsArg(0), xsID_listener)) {
		xsListener xsl;
		xsmcGet(xsVar(0), xsArg(0), xsID_listener);
		xsl = xsmcGetHostData(xsVar(0));

		modCriticalSectionBegin();

		for (i = 0; i < kListenerPendingSockets; i++) {
			if (xsl->accept[i]) {
				uint8 j;

				xss = xsl->accept[i];
				xsl->accept[i] = NULL;

				xss->next = gSockets;
				gSockets = xss;

				modCriticalSectionEnd();

				xss->obj = xsThis;
				xsmcSetHostData(xsThis, xss);
				xss->constructed = true;
				xsRemember(xss->obj);

				socketUpUseCount(the, xss);

				if (xss->pending) {
					uint8_t pending;
					modCriticalSectionBegin();
						pending = xss->pending;
						xss->pending = 0;
					modCriticalSectionEnd();
					socketSetPending(xss, pending);
				}

				socketDownUseCount(xss->the, xss);
				return;
			}
		}

		modCriticalSectionEnd();
		xsUnknownError("no socket avaiable from listener");
	}

	// allocate socket
	xss = c_calloc(1, sizeof(xsSocketRecord) + (sizeof(xsSocketUDPRemoteRecord) * (kReadQueueLength - 1)));
	if (!xss)
		xsUnknownError("no memory for socket record");

	xss->the = the;
	xss->obj = xsThis;
	xss->constructed = true;
	xss->useCount = 1;
	xsmcSetHostData(xsThis, xss);
	xsRemember(xss->obj);

	modCriticalSectionBegin();
	xss->next = gSockets;
	gSockets = xss;
	modCriticalSectionEnd();

	modInstrumentationAdjust(NetworkSockets, 1);

	// determine socket kind
	xss->kind = kTCP;
	if (xsmcHas(xsArg(0), xsID_kind)) {
		char *kind;

		xsmcGet(xsVar(0), xsArg(0), xsID_kind);
		kind = xsmcToString(xsVar(0));
		if (0 == c_strcmp(kind, "TCP"))
			;
		else if (0 == c_strcmp(kind, "UDP")) {
			xss->kind = kUDP;
			if (xsmcHas(xsArg(0), xsID_multicast)) {
				xsmcGet(xsVar(0), xsArg(0), xsID_multicast);
				xsmcToStringBuffer(xsVar(0), temp, sizeof(temp));
				if (!parseAddress(temp, multicastIP))
					xsUnknownError("invalid multicast IP address");

				ttl = 1;
				if (xsmcHas(xsArg(0), xsID_ttl)) {
					xsmcGet(xsVar(0), xsArg(0), xsID_ttl);
					ttl = xsmcToInteger(xsVar(0));
				}
			}
		}
		else
			xsUnknownError("invalid socket kind");
	}

	// prepare inputs
	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = xsmcToInteger(xsVar(0));
	}

	xss->port = port;
	if (kTCP == xss->kind)
		xss->skt = tcp_new_safe();
	else
		xss->udp = udp_new_safe();

	if (!xss->skt && !xss->udp)
		xsUnknownError("failed to allocate socket");

	if (kTCP == xss->kind)
		err = tcp_bind_safe(xss->skt, IP_ADDR_ANY, 0);
	else
		err = udp_bind_safe(xss->udp, IP_ADDR_ANY, xss->port);
	if (err)
		xsUnknownError("socket bind failed");

	if (kTCP == xss->kind) {
		tcp_arg(xss->skt, xss);
		tcp_recv(xss->skt, didReceive);
		tcp_sent(xss->skt, didSend);
		tcp_err(xss->skt, didError);
	}
	else {
		udp_recv(xss->udp, (udp_recv_fn)didReceiveUDP, xss);

		if (ttl) {
			ip_addr_t ifaddr;
	#if ESP32
			tcpip_adapter_ip_info_t info;
			tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &info);
			ifaddr.u_addr.ip4 = info.ip;
	#else
			struct ip_info staIpInfo;
			wifi_get_ip_info(0, &staIpInfo);		// 0 == STATION_IF
			ifaddr.addr = staIpInfo.ip.addr;
	#endif
			ip_addr_t multicast_addr;
			IP_ADDR4(&multicast_addr, multicastIP[0], multicastIP[1], multicastIP[2], multicastIP[3]);
			igmp_joingroup(&ifaddr, &multicast_addr);

			IP_ADDR4(&(xss->udp)->multicast_ip, multicastIP[0], multicastIP[1], multicastIP[2], multicastIP[3]);
			xss->udp->ttl = 1;
		}
	}

	if (kTCP == xss->kind) {
		if (xsmcHas(xsArg(0), xsID_host)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_host);
			xsmcToStringBuffer(xsVar(0), temp, sizeof(temp));
			ip_addr_t resolved;
			if (ERR_OK == dns_gethostbyname_safe(temp, &resolved, didFindDNS, xss)) {
#if LWIP_IPV4 && LWIP_IPV6
				ip[0] = ip4_addr1(&resolved.u_addr.ip4);
				ip[1] = ip4_addr2(&resolved.u_addr.ip4);
				ip[2] = ip4_addr3(&resolved.u_addr.ip4);
				ip[3] = ip4_addr4(&resolved.u_addr.ip4);
#else
				ip[0] = ip4_addr1(&resolved);
				ip[1] = ip4_addr2(&resolved);
				ip[2] = ip4_addr3(&resolved);
				ip[3] = ip4_addr4(&resolved);
#endif
			}
			else
				waiting = 1;
		}
		else
		if (xsmcHas(xsArg(0), xsID_address)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_address);
			xsmcToStringBuffer(xsVar(0), temp, sizeof(temp));
			if (!parseAddress(temp, ip))
				xsUnknownError("invalid IP address");
		}
		else
			xsUnknownError("invalid dictionary");
	}

	if (waiting || (kUDP == xss->kind))
		return;

	IP_ADDR4(&ipaddr, ip[0], ip[1], ip[2], ip[3]);
	err = tcp_connect_safe(xss, &ipaddr, port, didConnect);
	if (err)
		xsUnknownError("socket connect failed");
}

void xs_socket_destructor(void *data)
{
	xsSocket xss = data;
	unsigned char i;

	if (!xss) return;

	for (i = 0; i < kReadQueueLength - 1; i++) {
		if (xss->reader[i])
			pbuf_free_safe(xss->reader[i]);
	}

	if (xss->skt) {
		tcp_recv(xss->skt, NULL);
		tcp_sent(xss->skt, NULL);
		tcp_err(xss->skt, NULL);
		tcp_close_safe(xss->skt);
	}

	if (xss->udp) {
		udp_recv(xss->udp, NULL, NULL);
		udp_remove_safe(xss->udp);
	}

	forgetSocket(xss);

	c_free(xss);

	modInstrumentationAdjust(NetworkSockets, -1);
}

void xs_socket_close(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);

	if (NULL == xss)
		xsUnknownError("close on closed socket");

	if (!(xss->pending & kPendingClose))
		socketSetPending(xss, kPendingClose);
}

void xs_socket_read(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);
	xsType dstType;
	int argc = xsmcArgc;
	uint16 srcBytes;
	unsigned char *srcData;

	if (NULL == xss)
		xsUnknownError("read on closed socket");

	if (!xss->buf || (xss->bufpos >= xss->buflen) || xss->suspended) {
		if (0 == argc)
			xsResult = xsInteger(0);
		else
			xsUnknownError("nothing to read");
		return;
	}

	srcData = xss->bufpos + (unsigned char *)xss->buf;
	srcBytes = xss->buflen - xss->bufpos;

	if (0 == argc) {
		xsResult = xsInteger(srcBytes);
		return;
	}

	// address limiter argument (count or terminator)
	if (argc > 1) {
		xsType limiterType = xsmcTypeOf(xsArg(1));
		if ((xsNumberType == limiterType) || (xsIntegerType == limiterType)) {
			uint16 count = xsmcToInteger(xsArg(1));
			if (count < srcBytes)
				srcBytes = count;
		}
		else
		if (xsStringType == limiterType) {
			char *str = xsmcToString(xsArg(1));
			char terminator = c_read8(str);
			if (terminator) {
				unsigned char *t = c_strchr(srcData, terminator);
				if (t) {
					uint16 count = (t - srcData) + 1;		// terminator included in result
					if (count < srcBytes)
						srcBytes = count;
				}
			}
		}
		else if (xsUndefinedType == limiterType)
			;
	}

	// generate output
	dstType = xsmcTypeOf(xsArg(0));

	if (xsNullType == dstType)
		xsResult = xsInteger(srcBytes);
	else if (xsReferenceType == dstType) {
		xsSlot *s1, *s2;

		s1 = &xsArg(0);

		xsmcVars(1);
		xsmcGet(xsVar(0), xsGlobal, xsID_String);
		s2 = &xsVar(0);
		if (s1->data[2] == s2->data[2])		//@@
			xsResult = xsStringBuffer(srcData, srcBytes);
		else {
			xsmcGet(xsVar(0), xsGlobal, xsID_Number);
			s2 = &xsVar(0);
			if (s1->data[2] == s2->data[2]) {		//@@
				xsResult = xsInteger(*srcData);
				srcBytes = 1;
			}
			else {
				xsmcGet(xsVar(0), xsGlobal, xsID_ArrayBuffer);
				s2 = &xsVar(0);
				if (s1->data[2] == s2->data[2])		//@@
					xsResult = xsArrayBuffer(srcData, srcBytes);
				else
					xsUnknownError("unsupported output type");
			}
		}
	}

	xss->bufpos += srcBytes;

	if (xss->bufpos == xss->buflen) {
		if (xss->pb)
			pbuf_free_safe(xss->pb);
		xss->pb = NULL;

		xss->bufpos = xss->buflen = 0;
		xss->buf = NULL;
	}
}

void xs_socket_write(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);
	int argc = xsmcArgc;
	char *msg;
	size_t msgLen;
	u16_t available, needed = 0;
	err_t err;
	unsigned char pass, arg;

	if ((NULL == xss) || !(xss->skt || xss->udp) || xss->writeDisabled) {
		if (0 == argc) {
			xsResult = xsInteger(0);
			return;
		}
		xsUnknownError("write on closed socket");
	}

	if (xss->udp) {
		char temp[16];
		uint8 ip[4];
		unsigned char *data;
		uint16 port = xsmcToInteger(xsArg(1));
		ip_addr_t dst;

		xsmcToStringBuffer(xsArg(0), temp, sizeof(temp));
		if (!parseAddress(temp, ip))
			xsUnknownError("invalid IP address");
		IP_ADDR4(&dst, ip[0], ip[1], ip[2], ip[3]);

		needed = xsGetArrayBufferLength(xsArg(2));
		data = xsmcToArrayBuffer(xsArg(2));
		udp_sendto_safe(xss->udp, data, needed, &dst, port, &err);
		if (ERR_OK != err)
			xsUnknownError("UDP send failed");

		modInstrumentationAdjust(NetworkBytesWritten, needed);
		return;
	}

	available = tcp_sndbuf(xss->skt);
	if (0 == argc) {
		xsResult = xsInteger(available);
		return;
	}

	for (pass = 0; pass < 2; pass++ ) {
		for (arg = 0; arg < argc; arg++) {
			xsType t = xsmcTypeOf(xsArg(arg));

			if (xsStringType == t) {
				char *msg = xsmcToString(xsArg(arg));
				int msgLen = c_strlen(msg);
				if (0 == pass)
					needed += msgLen;
				else {
					// pull string through a temporary buffer, as it may be in ROM
					while (msgLen) {
						char buffer[128];
						int use = msgLen;
						if (use > sizeof(buffer))
							use = sizeof(buffer);

						c_memcpy(buffer, msg, use);
						do {
							err = tcp_write_safe(xss, buffer, use, TCP_WRITE_FLAG_COPY);
							if (ERR_OK == err)
								break;

							if (ERR_MEM != err) {
								socketSetPending(xss, kPendingError);
								return;
							}

							modDelayMilliseconds(25);
						} while (true);

						msg += use;
						msgLen -= use;
					}
				}
			}
			else if ((xsNumberType == t) || (xsIntegerType == t)) {
				if (0 == pass)
					needed += 1;
				else {
					unsigned char byte = (unsigned char)xsmcToInteger(xsArg(arg));
					do {
						err = tcp_write_safe(xss, &byte, 1, TCP_WRITE_FLAG_COPY);
						if (ERR_OK == err)
							break;

						if (ERR_MEM != err) {
							socketSetPending(xss, kPendingError);
							return;
						}

						modDelayMilliseconds(25);
					} while (true);
				}
			}
			else if (xsReferenceType == t) {
				if (xsmcIsInstanceOf(xsArg(arg), xsArrayBufferPrototype)) {
					int msgLen = xsGetArrayBufferLength(xsArg(arg));
					if (0 == pass)
						needed += msgLen;
					else {
						char *msg = xsmcToArrayBuffer(xsArg(arg));

						do {
							err = tcp_write_safe(xss, msg, msgLen, TCP_WRITE_FLAG_COPY);		// this assumes data is in RAM
							if (ERR_OK == err)
								break;

							if (ERR_MEM != err) {
								socketSetPending(xss, kPendingError);
								return;
							}

							modDelayMilliseconds(25);
						} while (true);
					}
				}
				else if (xsmcIsInstanceOf(xsArg(arg), xsTypedArrayPrototype)) {
					int msgLen, byteOffset;

					xsmcGet(xsResult, xsArg(arg), xsID_byteLength);
					msgLen = xsmcToInteger(xsResult);
					if (0 == pass)
						needed += msgLen;
					else {
						xsSlot tmp;
						char *msg;

						xsmcGet(tmp, xsArg(arg), xsID_byteOffset);
						byteOffset = xsmcToInteger(tmp);

						xsmcGet(tmp, xsArg(arg), xsID_buffer);
						msg = byteOffset + (char *)xsmcToArrayBuffer(tmp);

						do {
							err = tcp_write_safe(xss, msg, msgLen, TCP_WRITE_FLAG_COPY);		// this assumes data is in RAM
							if (ERR_OK == err)
								break;

							if (ERR_MEM != err) {
								socketSetPending(xss, kPendingError);
								return;
							}

							modDelayMilliseconds(25);
						} while (true);
					}
				}
				else
					xsUnknownError("unsupported type for write");
			}
			else
				xsUnknownError("unsupported type for write");
		}

		if ((0 == pass) && (needed > available))
			xsUnknownError("can't write all data");
	}

	xss->outstandingSent += needed;

	if (xss->skt)
		socketSetPending(xss, kPendingOutput);
}

void xs_socket_suspend(xsMachine *the)
{
	xsSocket xss = xsmcGetHostData(xsThis);

	if (xsmcArgc) {
		uint8_t suspended = xsmcToBoolean(xsArg(0));
		if (!suspended) {
			modLog("resume");
			socketSetPending(xss, kPendingReceive);
			if (xss->suspendedError) {
				modLog("resume - has error");
				socketSetPending(xss, kPendingError);
			}
			if (xss->suspendedDisconnect) {
				modLog("resume - has disconnect");
				socketSetPending(xss, kPendingDisconnect);
			}
			socketSetPending(xss, kPendingReceive);
			xss->suspendedError = 0;
			xss->suspendedDisconnect = 0;
		}
		else {
			modLog("suspend");
			if (xss->pending & kPendingError) {		//@@ critical section
				modLog("suspend - pending error");
				xss->pending &= ~kPendingError;
				xss->suspendedError = 1;
			}
			if (xss->pending & kPendingDisconnect) {		//@@ critical section
				modLog("suspend - pending disconnect");
				xss->pending &= ~kPendingDisconnect;
				xss->suspendedDisconnect = 1;
			}
		}
		xss->suspended = suspended;
	}

	xsmcSetBoolean(xsResult, xss->suspended);
}

void socketMsgConnect(xsSocket xss)
{
	xsMachine *the = xss->the;

	xsBeginHost(the);
	if (xss->skt)
		xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgConnect));
	xsEndHost(the);
}

void socketMsgDisconnect(xsSocket xss)
{
	xsMachine *the = xss->the;

	if (xss->suspended) {
		xss->suspendedDisconnect = 1;
		return;
	}

	xsBeginHost(the);
		xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgDisconnect));
	xsEndHost(the);
}

void socketMsgError(xsSocket xss)
{
	xsMachine *the = xss->the;

	if (xss->suspended) {
		xss->suspendedError = 1;
		return;
	}

	xsBeginHost(the);
		xsCall1(xss->obj, xsID_callback, xsInteger(kSocketMsgError));		//@@ report the error value
	xsEndHost(the);
}

void socketMsgDataReceived(xsSocket xss)
{
	xsMachine *the = xss->the;
	unsigned char i, readerCount;
	uint16_t tot_len, bufpos = 0;
	struct pbuf *pb, *walker;
	uint8_t one = 0;

	if (xss->suspended)
		return;

	modCriticalSectionBegin();
	for (readerCount = 0; xss->reader[readerCount] && (readerCount < kReadQueueLength); readerCount++)
		;
	modCriticalSectionEnd();

	if (xss->suspendedBuf) {
		pb = xss->suspendedBuf;
		walker = xss->suspendedFragment;
		bufpos = xss->suspendedBufpos;
		xss->suspendedBuf = xss->suspendedFragment = NULL;
		readerCount += 1;
		goto resumeBuffer;
	}

	while (readerCount--) {
		modCriticalSectionBegin();
		pb = xss->reader[0];
		if (!pb) {
			modCriticalSectionEnd();
			break;
		}

		for (i = 0; i < kReadQueueLength - 1; i++)
			xss->reader[i] = xss->reader[i + 1];
		xss->reader[kReadQueueLength - 1] = NULL;
		modCriticalSectionEnd();

		walker = pb;
resumeBuffer:
		xsBeginHost(the);

		tot_len = pb->tot_len;

		if (NULL == pb->next) {
			one = 1;
			xss->pb = pb;
		}

		for (; walker && !xss->suspended; walker = walker->next) {
			xss->buf = walker->payload;
			xss->bufpos = bufpos;
			xss->buflen = walker->len;
			bufpos = 0;

			xsTry {
				if (kTCP == xss->kind) {
#if !ESP32
					system_soft_wdt_stop();		//@@
#endif
					xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataReceived), xsInteger(xss->buflen - xss->bufpos));
#if !ESP32
					system_soft_wdt_restart();		//@@
#endif
				}
				else {
					ip_addr_t address = xss->remote[0].address;
					char *out;

					xsResult = xsStringBuffer(NULL, 4 * 5);
					out = xsmcToString(xsResult);
#if LWIP_IPV4 && LWIP_IPV6
					itoa(ip4_addr1(&address.u_addr.ip4), out, 10); out += strlen(out); *out++ = '.';
					itoa(ip4_addr2(&address.u_addr.ip4), out, 10); out += strlen(out); *out++ = '.';
					itoa(ip4_addr3(&address.u_addr.ip4), out, 10); out += strlen(out); *out++ = '.';
					itoa(ip4_addr4(&address.u_addr.ip4), out, 10); out += strlen(out); *out = 0;
#else
					itoa(ip4_addr1(&address), out, 10); out += strlen(out); *out++ = '.';
					itoa(ip4_addr2(&address), out, 10); out += strlen(out); *out++ = '.';
					itoa(ip4_addr3(&address), out, 10); out += strlen(out); *out++ = '.';
					itoa(ip4_addr4(&address), out, 10); out += strlen(out); *out = 0;
#endif
					xsCall4(xss->obj, xsID_callback, xsInteger(kSocketMsgDataReceived), xsInteger(xss->buflen), xsResult, xsInteger(xss->remote[0].port));

					xss->remoteCount -= 1;
					c_memmove(&xss->remote[0], &xss->remote[1], xss->remoteCount * sizeof(xsSocketUDPRemoteRecord));
				}
			}
			xsCatch {
			}

			if (one)
				break;
		}

		xsEndHost(the);

		if (xss->suspended) {
			if (xss->bufpos != xss->buflen) {
				xss->suspendedBuf = pb;
				xss->suspendedFragment = walker;
				xss->suspendedBufpos = xss->bufpos;
			}
			else if (!one && walker->next) {
				xss->suspendedBuf = pb;
				xss->suspendedFragment = walker->next;
				xss->suspendedBufpos = 0;
			}
		}

		if (xss->skt && tot_len && !xss->suspendedBuf)
			tcp_recved_safe(xss, tot_len);

		xss->buf = NULL;

		if (xss->suspendedBuf)
			break;

		if (one) {
			if (xss->pb)
				pbuf_free_safe(xss->pb);
			xss->pb = NULL;
		}
		else
			pbuf_free_safe(pb);
	}
}

void socketMsgDataSent(xsSocket xss)
{
	xsMachine *the = xss->the;

	xsBeginHost(the);
		xsCall2(xss->obj, xsID_callback, xsInteger(kSocketMsgDataSent), xsInteger(xss->skt ? tcp_sndbuf(xss->skt) : 0));
	xsEndHost(the);
}


#if ESP32
void didFindDNS(const char *name, const ip_addr_t *ipaddr, void *arg)
#else
void didFindDNS(const char *name, ip_addr_t *ipaddr, void *arg)
#endif
{
	xsSocket xss = arg;

	if (ipaddr)
		tcp_connect_safe(xss, ipaddr, xss->port, didConnect);
	else
		socketSetPending(xss, kPendingError);
}

void didError(void *arg, err_t err)
{
	xsSocket xss = arg;

	xss->skt = NULL;
	socketSetPending(xss, kPendingError);
}

err_t didConnect(void * arg, struct tcp_pcb * tpcb, err_t err)
{
	xsSocket xss = arg;

	if (ERR_OK != err)
		socketSetPending(xss, kPendingError);
	else
		socketSetPending(xss, kPendingConnect);

	return ERR_OK;
}

err_t didReceive(void * arg, struct tcp_pcb * pcb, struct pbuf * p, err_t err)
{
	xsSocket xss = arg;
	unsigned char i;
	struct pbuf *walker;
	uint16 offset;

	if (xss->pending & kPendingClose)
		return ERR_OK;

	if (!p) {
		tcp_recv(xss->skt, NULL);
		tcp_sent(xss->skt, NULL);
		tcp_err(xss->skt, NULL);

		if (xss->suspended)
			xss->suspendedDisconnect = true;
		else {
#if ESP32
			xss->skt = NULL;			// no close on socket if disconnected.
#endif
			socketSetPending(xss, kPendingDisconnect);
		}

		return ERR_OK;
	}

	modCriticalSectionBegin();
	for (i = 0; i < kReadQueueLength; i++) {
		if (NULL == xss->reader[i]) {
			xss->reader[i] = p;
			break;
		}
	}
	modCriticalSectionEnd();

	if (kReadQueueLength == i) {
		modLog("tcp read overflow!");
		pbuf_free_safe(p);
		return ERR_MEM;
	}

	modInstrumentationAdjust(NetworkBytesRead, p->tot_len);

	if (!xss->suspended)
		socketSetPending(xss, err ? kPendingError : kPendingReceive);
	else {
		if (err)
			xss->suspendedError = true;
	}

	return ERR_OK;
}

err_t didSend(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	xsSocket xss = arg;

	modInstrumentationAdjust(NetworkBytesWritten, len);

	xss->outstandingSent -= len;
	if (0 == xss->outstandingSent)
		socketSetPending(xss, kPendingSent);

	return ERR_OK;
}

void didReceiveUDP(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *remoteAddr, u16_t remotePort)
{
	xsSocket xss = arg;
	unsigned char i;

	modCriticalSectionBegin();

	for (i = 0; i < kReadQueueLength; i++) {
		if (NULL == xss->reader[i])
			break;
	}

	if (kReadQueueLength == i) {
#if 1
		// ignore oldest
		modLog("udp receive overflow - ignore earliest");
		pbuf_free(xss->reader[0]);		// not pbuf_free_safe, because we are in lwip task
		for (i = 1; i < kReadQueueLength; i++) {
			xss->reader[i - 1] = xss->reader[i];
			xss->remote[i - 1] = xss->remote[i];
		}
		xss->reader[kReadQueueLength - 1] = NULL;
		xss->remoteCount += 1;
#else
		// ignore most recent
		modCriticalSectionEnd();
		modLog("udp receive overflow - ignore latest");
		pbuf_free(p);		// not pbuf_free_safe, because we are in lwip task
		return;
#endif
	}

	xss->remote[xss->remoteCount].port = remotePort;
	xss->remote[xss->remoteCount].address = *remoteAddr;
	xss->remoteCount += 1;

	modInstrumentationAdjust(NetworkBytesRead, p->tot_len);
	xss->reader[i] = p;
	modCriticalSectionEnd();

	socketSetPending(xss, kPendingReceive);
}

static uint8 parseAddress(char *address, uint8_t *ip)
{
	char *p = address;
	int i;
	for (i = 0; i < 3; i++) {
		char *separator = c_strchr(p, (i < 3) ? '.' : 0);
		if (!separator)
			return 0;
		*separator = 0;
		ip[i] = (unsigned char)atoi(p);
		p = separator + 1;
	}
	ip[3] = (unsigned char)atoi(p);

	return 1;
}

static err_t didAccept(void * arg, struct tcp_pcb * newpcb, err_t err);

// to accept an incoming connection: let incoming = new Socket({listener});
void xs_listener(xsMachine *the)
{
	xsListener xsl;
	uint16 port = 0;
	err_t err;

	// allocate listener
	xsl = c_calloc(1, sizeof(xsListenerRecord));
	if (!xsl)
		xsUnknownError("out of memory");

	xsmcSetHostData(xsThis, xsl);

	modInstrumentationAdjust(NetworkSockets, 1);

	if (xsmcHas(xsArg(0), xsID_port)) {
		xsmcVars(1);
		xsmcGet(xsVar(0), xsArg(0), xsID_port);
		port = (uint16)xsmcToInteger(xsVar(0));
	}

	xsl->obj = xsThis;
	xsl->the = the;
	socketUpUseCount(the, xsl);
	xsRemember(xsl->obj);

	xsl->kind = kTCPListener;

	xsl->skt = tcp_new_safe();
	if (!xsl->skt)
		xsUnknownError("socket allocation failed");

	modCriticalSectionBegin();
	xsl->next = (xsListener)gSockets;
	gSockets = (xsSocket)xsl;
	modCriticalSectionEnd();

	err = tcp_bind_safe(xsl->skt, IP_ADDR_ANY, port);
	if (err)
		xsUnknownError("socket bind");

	xsl->skt = tcp_listen_safe(xsl->skt);

	tcp_arg(xsl->skt, xsl);

	tcp_accept(xsl->skt, didAccept);
}

void xs_listener_destructor(void *data)
{
	xsListener xsl = data;
	uint8 i;

	if (!xsl) return;

	if (xsl->skt) {
		tcp_accept(xsl->skt, NULL);
		tcp_close_safe(xsl->skt);
	}

	for (i = 0; i < kListenerPendingSockets; i++)
		xs_socket_destructor(xsl->accept[i]);

	forgetSocket((xsSocket)xsl);

	c_free(xsl);

	modInstrumentationAdjust(NetworkSockets, -1);
}

void xs_listener_close(xsMachine *the)
{
	xsListener xsl = xsmcGetHostData(xsThis);

	if ((NULL == xsl) || (xsl->pending & kPendingClose))
		xsUnknownError("close on closed listener");

	socketSetPending((xsSocket)xsl, kPendingClose);
}

static void listenerMsgNew(xsListener xsl)
{
	xsMachine *the = xsl->the;
	uint8 i;

	// service all incoming sockets currently in the list
	for (i = 0; i < kListenerPendingSockets; i++) {
		if (NULL == xsl->accept[i])
			continue;

		xsBeginHost(the);
		xsCall1(xsl->obj, xsID_callback, xsInteger(kListenerMsgConnect));
		xsEndHost(the);
	}
}

//@@ maybe tcp_abort instead of tcp_close here - the netconn code suggests that is correct
err_t didAccept(void * arg, struct tcp_pcb * newpcb, err_t err)
{
	xsListener xsl = arg;
	xsSocket xss;
	uint8 i;

	tcp_accepted(xsl->skt);

	xss = c_calloc(1, sizeof(xsSocketRecord) - sizeof(xsSocketUDPRemoteRecord));
	if (!xss) {
		tcp_close(newpcb);		// not tcp_close_safe since we are in the lwip thread already
		return ERR_MEM;
	}

	xss->the = xsl->the;
	xss->skt = newpcb;
	xss->useCount = 1;
	xss->kind = kTCP;

	modCriticalSectionBegin();
	for (i = 0; i < kListenerPendingSockets; i++) {
		if (NULL == xsl->accept[i]) {
			xsl->accept[i] = xss;
			break;
		}
	}
	modCriticalSectionEnd();

	if (kListenerPendingSockets == i) {
		modLog("tcp accept queue full");
		tcp_close(newpcb);		// not tcp_close_safe since we are in the lwip thread already
		c_free(xss);
		return ERR_MEM;
	}

	tcp_arg(xss->skt, xss);
	tcp_recv(xss->skt, didReceive);
	tcp_sent(xss->skt, didSend);
	tcp_err(xss->skt, didError);

	socketSetPending((xsSocket)xsl, kPendingAcceptListener);

	modInstrumentationAdjust(NetworkSockets, 1);

	return ERR_OK;
}

void socketSetPending(xsSocket xss, uint8_t pending)
{
	uint8_t doSchedule;

	modCriticalSectionBegin();

	if (((xss->pending & pending) == pending) || (xss->pending & kPendingClose)) {
		modCriticalSectionEnd();
		return;
	}

	doSchedule = 0 == xss->pending;
	xss->pending |= pending;

	if (xss->pending & (kPendingError | kPendingDisconnect))
		xss->writeDisabled = true;

	if (doSchedule && (xss->constructed || (pending & kPendingAcceptListener))) {
		socketUpUseCount(xss->the, xss);
		modCriticalSectionEnd();

		modMessagePostToMachine(xss->the, NULL, 0, socketClearPending, xss);
	}
	else
		modCriticalSectionEnd();
}

void socketClearPending(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsSocket xss = refcon;
	uint8_t pending;

	modCriticalSectionBegin();

	pending = xss->pending;
	xss->pending &= kPendingClose;		// don't clear close flag

	modCriticalSectionEnd();

	if (!pending) {
		modLog("socketClearPending called - NOTHING PENDING?");
		goto done;		// return or done...
	}

	if (pending & kPendingClose)
		socketDownUseCount(xss->the, xss);

	if (pending & kPendingReceive)
		socketMsgDataReceived(xss);

	if (pending & kPendingSent)
		socketMsgDataSent(xss);

	if (pending & kPendingOutput)
		tcp_output_safe(xss);

	if (pending & kPendingConnect)
		socketMsgConnect(xss);

	if (pending & kPendingDisconnect)
		socketMsgDisconnect(xss);

	if (pending & kPendingError)
		socketMsgError(xss);

	if (pending & kPendingAcceptListener)
		listenerMsgNew((xsListener)xss);

done:
	socketDownUseCount(xss->the, xss);
}
