/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
	Internal helper shared by the simulated network interfaces.

	The Wi-Fi connection - station or access point - is simulated, but the IP and MAC
	addresses it reports are those of the host computer's network interface, so that
	code which uses the address - to bind, to advertise, to display - gets an address
	that works.
*/

#include "xs.h"

#if mxWindows
	#undef WINVER
	#include <iphlpapi.h>
#else
	#include <ifaddrs.h>
	#include <net/if.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#if mxMacOSX
		#include <net/if_dl.h>
	#else
		#include <netpacket/packet.h>
	#endif
#endif

static void formatMAC(xsMachine *the, const unsigned char *mac)
{
	char buffer[18];
	snprintf(buffer, sizeof(buffer), "%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	xsResult = xsString(buffer);
}

#if mxWindows

// returns the address (wantMAC false) or MAC address (wantMAC true) of the first
// adapter with an IPv4 address, or undefined if there is none
static void hostInterface(xsMachine *the, uint8_t wantMAC)
{
	DWORD bytes, result;
	IP_ADAPTER_INFO *adapters = NULL, *adapter;

	result = GetAdaptersInfo(NULL, &bytes);
	if ((ERROR_SUCCESS != result) && (ERROR_BUFFER_OVERFLOW != result))
		return;

	adapters = c_malloc(bytes);
	if (!adapters)
		return;

	if (ERROR_SUCCESS != GetAdaptersInfo(adapters, &bytes))
		goto bail;

	for (adapter = adapters; NULL != adapter; adapter = adapter->Next) {
		const char *address = adapter->IpAddressList.IpAddress.String;
		if ((IF_TYPE_PPP == adapter->Type) || !c_strcmp("0.0.0.0", address))
			continue;

		if (!wantMAC)
			xsResult = xsString((char *)address);
		else if (6 == adapter->AddressLength)
			formatMAC(the, adapter->Address);
		break;
	}

bail:
	c_free(adapters);
}

#else

// returns the address (wantMAC false) or MAC address (wantMAC true) of the host's
// primary network interface, or undefined if there is none. A non-loopback
// interface is preferred; loopback is used only when nothing else is available.
static void hostInterface(xsMachine *the, uint8_t wantMAC)
{
	struct ifaddrs *ifaddr, *ifa;
	char name[IF_NAMESIZE], address[INET_ADDRSTRLEN];
	uint8_t pass, found = 0;

	if (-1 == getifaddrs(&ifaddr))
		return;

	for (pass = 0; (pass < 2) && !found; pass++) {
		for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
			struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
			uint8_t loopback = (0 != (ifa->ifa_flags & IFF_LOOPBACK));
			if (!sin || (AF_INET != sin->sin_family) || (loopback != pass))
				continue;
			if ((IFF_UP | IFF_RUNNING) != (ifa->ifa_flags & (IFF_UP | IFF_RUNNING)))
				continue;
			if (!inet_ntop(AF_INET, &sin->sin_addr, address, sizeof(address)))
				continue;

			c_strncpy(name, ifa->ifa_name, sizeof(name));
			name[sizeof(name) - 1] = 0;
			found = 1;
			break;
		}
	}
	if (!found)
		goto bail;

	if (!wantMAC) {
		xsResult = xsString(address);
		goto bail;
	}

#if mxMacOSX
	#define kLinkFamily AF_LINK
#else
	#define kLinkFamily AF_PACKET
#endif

	for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr || (kLinkFamily != ifa->ifa_addr->sa_family))
			continue;
		if (c_strcmp(ifa->ifa_name, name))
			continue;
#if mxMacOSX
		struct sockaddr_dl *sdl = (struct sockaddr_dl *)ifa->ifa_addr;
		if (6 != sdl->sdl_alen)
			continue;
		formatMAC(the, (const unsigned char *)LLADDR(sdl));
#else
		struct sockaddr_ll *sll = (struct sockaddr_ll *)ifa->ifa_addr;
		if (6 != sll->sll_halen)
			continue;
		formatMAC(the, sll->sll_addr);
#endif
		break;
	}

bail:
	freeifaddrs(ifaddr);
}

#endif

void xs_wifisim_address(xsMachine *the)
{
	hostInterface(the, 0);
}

void xs_wifisim_MAC(xsMachine *the)
{
	hostInterface(the, 1);
}
