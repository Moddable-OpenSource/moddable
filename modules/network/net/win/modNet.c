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

#include "xs.h"

#undef WINVER
#include <wlanapi.h> 
#include <iphlpapi.h>

enum {
	kIP = 0,
	kMAC,
	kRSSI,
	kSSID,
	kBSSID
};

static void getWLANProperty(xsMachine *the, uint8_t property)
{
	DWORD dwResult;
	HANDLE hClient = NULL;
	DWORD dwMaxClient = 2;
	DWORD dwCurVersion = 0;
	PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
	PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
	xsVars(1);

	dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
	if (dwResult != ERROR_SUCCESS)
		goto bail;
	dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
	if (dwResult != ERROR_SUCCESS)
		goto bail;
	for (int i = 0; i < (int)pIfList->dwNumberOfItems; i++) {
		PWLAN_INTERFACE_INFO pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[i];
		if (pIfInfo->isState == wlan_interface_state_connected) {
			DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
			WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;
			dwResult = WlanQueryInterface(hClient, &pIfInfo->InterfaceGuid,
				wlan_intf_opcode_current_connection,
				NULL,
				&connectInfoSize,
				(PVOID *)&pConnectInfo,
				&opCode);
			if (dwResult != ERROR_SUCCESS)
				goto bail;
			if (kSSID == property) {
				uint32_t length = pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength;
				if (length > 0) {
					char *ssid;
					xsResult = xsStringBuffer(NULL, length);
					ssid = xsToString(xsResult);
					c_memmove(ssid, pConnectInfo->wlanAssociationAttributes.dot11Ssid.ucSSID, length);
				}
			}
			else if (kBSSID == property) {
				char mac[32];
				uint8_t *addr = pConnectInfo->wlanAssociationAttributes.dot11Bssid;
				snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
					addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
				xsResult = xsString(mac);
			}
			else if (kRSSI == property) {
				// wlanSignalQuality values are between [0, 100]
				// mapped rssi values are between [-100, -50]
				int rssi = (pConnectInfo->wlanAssociationAttributes.wlanSignalQuality) / 100 * (50) - 100;
				xsResult = xsInteger(rssi);
			}
			break;
		}
	}
bail:
	if (pConnectInfo != NULL)
		WlanFreeMemory(pConnectInfo);
	if (pIfList != NULL)
		WlanFreeMemory(pIfList);
	if (hClient != NULL)
		WlanCloseHandle(hClient, NULL);
}

static void getAdapterProperty(xsMachine *the, uint8_t property)
{
	DWORD bytes, dwResult;
	IP_ADAPTER_INFO	*adapters = NULL, *pAdapter;

	dwResult = GetAdaptersInfo(NULL, &bytes);
	if (dwResult == ERROR_NO_DATA)
		goto bail;
	if ((ERROR_SUCCESS != dwResult) && (ERROR_BUFFER_OVERFLOW != dwResult))
		goto bail;
	adapters = c_malloc(bytes);
	if (!adapters)
		goto bail;
	dwResult = GetAdaptersInfo(adapters, &bytes);
	if (ERROR_SUCCESS != dwResult)
		goto bail;

	for (pAdapter = adapters; NULL != pAdapter; pAdapter = pAdapter->Next) {
		const IP_ADDR_STRING* ip = &pAdapter->IpAddressList;
		if (c_strcmp("0.0.0.0", ip->IpAddress.String) == 0 || pAdapter->Type == IF_TYPE_PPP)
			continue;
		if (kIP == property) {
			xsResult = xsString(ip->IpAddress.String);
		}
		else if (kMAC == property) {
			if (6 == pAdapter->AddressLength) {
				char mac[32];
				uint8_t *addr = pAdapter->Address;
				snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
					addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
				xsResult = xsString(mac);
			}
		}
		break;
	}

bail:
	if (adapters)
		c_free(adapters);
}

void xs_net_get(xsMachine *the)
{
	const char *prop = xsToString(xsArg(0));

	if (0 == c_strcmp(prop, "SSID")) {
		getWLANProperty(the, kSSID);
	}
	else if (0 == c_strcmp(prop, "RSSI")) {
		getWLANProperty(the, kRSSI);
	}
	else if (0 == c_strcmp(prop, "BSSID")) {
		getWLANProperty(the, kBSSID);
	}
	else if (0 == c_strcmp(prop, "MAC")) {
		getAdapterProperty(the, kMAC);
	}
	else if (0 == c_strcmp(prop, "IP")) {
		getAdapterProperty(the, kIP);
	}
}

