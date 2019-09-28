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

#ifndef __MODLWIPSAFE__
#define __MODLWIPSAFE__

#include "xsHost.h"

#if !ESP32
	#define tcp_new_safe tcp_new
	#define tcp_bind_safe tcp_bind
	#define tcp_listen_safe tcp_listen
	#define tcp_connect_safe(skt, ipaddr, port, connected) tcp_connect(skt, ipaddr, port, connected)
	// ESP8266 has a sort-of memory leak when using tcp_close without tcp_abort (see https://github.com/esp8266/Arduino/issues/230)
	#define tcp_close_safe(pb) {if (CLOSED == (pb)->state) tcp_close(pb); else {tcp_close(pb); tcp_abort(pb);}}
	#define tcp_output_safe tcp_output
	#define tcp_write_safe tcp_write
	#define tcp_recved_safe(skt, len) tcp_recved(skt, len)
	#define udp_new_safe udp_new
	#define udp_bind_safe udp_bind
	#define udp_remove_safe udp_remove
	#define udp_sendto_safe(skt, data, size, dst, port, err) \
		{ \
		struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM); \
		c_memcpy(p->payload, data, size); \
		*err = udp_sendto(skt, p, dst, port); \
		pbuf_free_safe(p); \
		}
	#define pbuf_free_safe pbuf_free
	#define dns_gethostbyname_safe dns_gethostbyname
#else
	#include "lwip/tcp.h"
	#include "lwip/dns.h"
	#include "lwip/udp.h"
	#include "lwip/raw.h"

	struct tcp_pcb *tcp_new_safe(void);
	err_t tcp_connect_safe(struct tcp_pcb *tcpPCB, const ip_addr_t *ipaddr, u16_t port, tcp_connected_fn connected);
	u8_t pbuf_free_safe(struct pbuf *p);
	err_t tcp_bind_safe(struct tcp_pcb *tcpPCB, const ip_addr_t *ipaddr, u16_t port);
	void tcp_close_safe(struct tcp_pcb *tcpPCB);
	void tcp_output_safe(struct tcp_pcb *tcpPCB);
	err_t tcp_write_safe(struct tcp_pcb *tcpPCB, const void *data, u16_t len, u8_t flags);
	void tcp_recved_safe(struct tcp_pcb *tcpPCB, u16_t len);
	struct tcp_pcb * tcp_listen_safe(struct tcp_pcb *pcb);
	struct udp_pcb *udp_new_safe(void);
	err_t udp_bind_safe(struct udp_pcb *udpPCB, const ip_addr_t *ipaddr, u16_t port);
	void udp_remove_safe(struct udp_pcb *udpPCB);
	void udp_sendto_safe(struct udp_pcb *udpPCB, const void *data, uint16_t len, ip_addr_t *dst, uint16_t port, err_t *err);
	err_t dns_gethostbyname_safe(const char *hostname, ip_addr_t *addr, dns_found_callback found, void *callback_arg);
#endif

#endif
