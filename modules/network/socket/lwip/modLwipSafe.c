/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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

#include "modLwipSafe.h"

#if ESP32

#include "lwip/priv/tcpip_priv.h"

typedef struct {
	struct tcpip_api_call_data	call;

	err_t						err;
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

static err_t tcp_new_INLWIP(struct tcpip_api_call_data *tcpMsg)
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

err_t tcp_connect_safe(struct tcp_pcb *skt, const ip_addr_t *ipaddr, u16_t port, tcp_connected_fn connected)
{
	LwipMsg msg = c_malloc(sizeof(LwipMsgRecord));
	msg->tcpPCB = skt,
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

static err_t tcp_bind_INLWIP(struct tcpip_api_call_data *tcpMsg)
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

static err_t tcp_close_INLWIP(struct tcpip_api_call_data *tcpMsg)
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

static err_t tcp_output_INLWIP(struct tcpip_api_call_data *tcpMsg)
{
	LwipMsg msg = (LwipMsg)tcpMsg;
	tcp_output(msg->tcpPCB);
	return ERR_OK;
}

void tcp_output_safe(struct tcp_pcb *tcpPCB)
{
	LwipMsgRecord msg = {
		.tcpPCB = tcpPCB,
	};
	tcpip_api_call(tcp_output_INLWIP, &msg.call);
}

static err_t tcp_write_INLWIP(struct tcpip_api_call_data *tcpMsg)
{
	LwipMsg msg = (LwipMsg)tcpMsg;
	if (msg->tcpPCB)
		msg->err = tcp_write(msg->tcpPCB, msg->data, msg->len, msg->flags);
	return ERR_OK;
}

err_t tcp_write_safe(struct tcp_pcb *tcpPCB, const void *data, u16_t len, u8_t flags)
{
	LwipMsgRecord msg = {
		.tcpPCB = tcpPCB,
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
	tcp_recved(msg->tcpPCB, msg->len);
	c_free(msg);
}

void tcp_recved_safe(struct tcp_pcb *tcpPCB, u16_t len)
{
	LwipMsg msg = c_malloc(sizeof(LwipMsgRecord));
	msg->tcpPCB = tcpPCB;
	msg->len = len;
	tcpip_callback_with_block(tcp_recved_INLWIP, msg, 1);
}

static err_t tcp_listen_INLWIP(struct tcpip_api_call_data *tcpMsg)
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

static err_t udp_new_INLWIP(struct tcpip_api_call_data *tcpMsg)
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

static err_t udp_bind_INLWIP(struct tcpip_api_call_data *tcpMsg)
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

static err_t udp_remove_INLWIP(struct tcpip_api_call_data *tcpMsg)
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

static err_t udp_sendto_INLWIP(struct tcpip_api_call_data *tcpMsg)
{
	LwipMsg msg = (LwipMsg)tcpMsg;
	struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, msg->len, PBUF_RAM);
	c_memcpy(p->payload, msg->data, msg->len);
	msg->err = udp_sendto(msg->udpPCB, p, &msg->addr, msg->port);
	pbuf_free_safe(p);
}

void udp_sendto_safe(struct udp_pcb *udpPCB, const void *data, uint16_t len, ip_addr_t *dst, uint16_t port, err_t *err)
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

#endif

