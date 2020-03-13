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
#include "mc.xs.h"			// for xsID_ values
#include "xsHost.h"

#define QAPI_USE_WLAN
#include "qapi.h"
#include "qapi_netservices.h"
#include "qurt_pipe.h"

enum { 
	STATION_IDLE = 0,
	STATION_CONNECTING,
	STATION_WRONG_PASSWORD,
	STATION_NO_AP_FOUND,
	STATION_CONNECT_FAIL,
	STATION_GOT_IP 
};

struct wifiScanRecord {
	xsSlot		callback;
	xsMachine	*the;
	int16_t		count;
	qapi_WLAN_BSS_Scan_Info_t results[__QAPI_MAX_SCAN_RESULT_ENTRY];
};
typedef struct wifiScanRecord wifiScanRecord;

static wifiScanRecord *gScan;
static int16_t gWiFiStatus = STATION_IDLE;

static int32_t wlan_callback_handler(uint8_t deviceId, uint32_t cbId, void *pApplicationContext, void *payload, uint32_t payload_Length);
static int32_t ap_callback_handler(uint8_t deviceId, uint32_t cbId, void *pApplicationContext, void *payload, uint32_t payload_Length);
static int32_t ipconfig_dhcpc_success_cb(uint32_t addr, uint32_t mask, uint32_t gw);
static void wifiEventPending(void *the, void *refcon, uint8_t *message, uint16_t messageLength);
static void wifiConnected(void *the, void *refcon, uint8_t *message, uint16_t messageLength);

void xs_wifi_set_mode(xsMachine *the)
{
	int mode = xsmcToInteger(xsArg(0));
	uint32_t deviceID = qca4020_wlan_get_active_device();
	qapi_WLAN_Dev_Mode_e opMode;
	
	opMode = (mode == 1 ? QAPI_WLAN_DEV_MODE_STATION_E : QAPI_WLAN_DEV_MODE_AP_E);
	qapi_WLAN_Set_Param(deviceID, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE, &opMode, sizeof(qapi_WLAN_Dev_Mode_e), FALSE);
}

void xs_wifi_get_mode(xsMachine *the)
{
    uint32_t dataLen;
	qapi_WLAN_Dev_Mode_e opMode;
	uint32_t deviceID = qca4020_wlan_get_active_device();
	
	qapi_WLAN_Get_Param(deviceID, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE, &opMode, &dataLen);
	xsmcSetInteger(xsResult, (opMode == QAPI_WLAN_DEV_MODE_STATION_E ? 1 : 0));
}

void xs_wifi_scan(xsMachine *the)
{
    uint32_t dataLen;
	qapi_WLAN_Dev_Mode_e opMode;
    qapi_WLAN_Start_Scan_Params_t scan_params;
	uint32_t deviceId = qca4020_wlan_get_active_device();
	int16_t channel = -1;
	char *ssid = "";
	
	qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE, &opMode, &dataLen);

	if (QAPI_WLAN_DEV_MODE_STATION_E != opMode)
		xsUnknownError("can only scan in STATION_MODE");

	if (gScan)
		xsUnknownError("unfinished wifi scan pending");

	gScan = (wifiScanRecord *)c_calloc(1, sizeof(wifiScanRecord));
	if (NULL == gScan)
		xsUnknownError("out of memory");
		
	gScan->callback = xsArg(1);
	gScan->the = the;
	xsRemember(gScan->callback);

	if (xsmcArgc) {
		xsmcVars(1);
		if (xsmcHas(xsArg(0), xsID_hidden)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_hidden);
			//show_hidden = xsmcTest(xsVar(0));	// @@ not supported
		}
		if (xsmcHas(xsArg(0), xsID_channel)) {
			xsmcGet(xsVar(0), xsArg(0), xsID_channel);
			channel = xsmcToInteger(xsVar(0));
		}
	}

	c_memset(&scan_params, 0, sizeof(scan_params));
	if (-1 != channel) {
		scan_params.num_Channels = 1;
		scan_params.channel_List[0] = channel;
	}
	
	qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID, (void *)ssid, 0, FALSE);
	
	qapi_WLAN_Set_Callback(deviceId, (qapi_WLAN_Callback_t)wlan_callback_handler, the);

	qapi_WLAN_Start_Scan(deviceId, &scan_params, QAPI_WLAN_BUFFER_SCAN_RESULTS_NON_BLOCKING_E);
}

void xs_wifi_connect(xsMachine *the)
{
	char *str;
	uint8_t bssid[__QAPI_WLAN_MAC_LEN] = {0};
	qapi_WLAN_Dev_Mode_e opMode = QAPI_WLAN_DEV_MODE_STATION_E;
	qapi_WLAN_Auth_Mode_e authMode;
	qapi_WLAN_Crypt_Type_e crypt;
	uint32_t deviceId = qca4020_wlan_get_active_device();
	int argc = xsmcArgc;
  
	qapi_WLAN_Set_Callback(deviceId, (qapi_WLAN_Callback_t)wlan_callback_handler, the);
	
	qapi_WLAN_Disconnect(deviceId);
	gWiFiStatus = STATION_IDLE;
	
	if (0 == argc)
		return;

	qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE, &opMode, sizeof(qapi_WLAN_Dev_Mode_e), FALSE);

	xsmcVars(2);
	if (xsmcHas(xsArg(0), xsID_ssid)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_ssid);
		str = xsmcToString(xsVar(0));
		if (c_strlen(str) > __QAPI_WLAN_MAX_SSID_LENGTH)
			xsUnknownError("ssid too long - 32 bytes max");
		qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID, (void *)str, c_strlen(str), FALSE);
	}

	if (xsmcHas(xsArg(0), xsID_password)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_password);
		str = xsmcToString(xsVar(0));
		if (c_strlen(str) > 63)
			xsUnknownError("password too long - 64 bytes max");
		authMode = QAPI_WLAN_AUTH_WPA2_PSK_E;
		crypt = QAPI_WLAN_CRYPT_AES_CRYPT_E;
		qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY, __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE, (void*)str, c_strlen(str), FALSE);
	}
	else {
		authMode = QAPI_WLAN_AUTH_NONE_E;
		crypt = QAPI_WLAN_CRYPT_NONE_E;
	}

	if (xsmcHas(xsArg(0), xsID_bssid)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_bssid);
		if (__QAPI_WLAN_MAC_LEN != xsmcGetArrayBufferLength(xsVar(0)))
			xsUnknownError("bssid must be 6 bytes");
		xsmcGetArrayBufferData(xsVar(0), 0, bssid, sizeof(bssid));
	}
	qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_BSSID, bssid, __QAPI_WLAN_MAC_LEN, FALSE);

	qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY, __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE, (void *)&authMode, sizeof(authMode), FALSE);
	qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY, __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE, (void *)&crypt, sizeof(crypt), FALSE);

	qapi_WLAN_Commit(deviceId);
	gWiFiStatus = STATION_CONNECTING;
}

void deliverScanResults(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsBeginHost(the);

	xsmcVars(2);
	xsTry {
		for (uint16_t i = 0; i < gScan->count; ++i) {
			qapi_WLAN_BSS_Scan_Info_t *awr = &gScan->results[i];

			xsmcSetNewObject(xsVar(1));

			xsmcSetStringBuffer(xsVar(0), (char*)awr->ssid, awr->ssid_Length);
			xsmcSet(xsVar(1), xsID_ssid, xsVar(0));

			xsmcSetInteger(xsVar(0), awr->rssi);
			xsmcSet(xsVar(1), xsID_rssi, xsVar(0));

			xsmcSetInteger(xsVar(0), awr->channel);
			xsmcSet(xsVar(1), xsID_channel, xsVar(0));

			xsmcSetBoolean(xsVar(0), 0);	// @@ not supported
			xsmcSet(xsVar(1), xsID_hidden, xsVar(0));

			xsmcSetArrayBuffer(xsVar(0), awr->bssid, sizeof(awr->bssid));
			xsmcSet(xsVar(1), xsID_bssid, xsVar(0));

			if (awr->security_Enabled) {
				if (awr->rsn_Auth || awr->rsn_Cipher) {
					if (awr->wpa_Auth & __QAPI_WLAN_SECURITY_AUTH_PSK) {
						if (awr->rsn_Auth)
							xsmcSetString(xsVar(0), "wpa_wpa2_psk");
						else
							xsmcSetString(xsVar(0), "wpa_psk");
					}
				}
				if (awr->rsn_Cipher) {
					if (awr->rsn_Auth && __QAPI_WLAN_CIPHER_TYPE_WEP)
						xsmcSetString(xsVar(0), "wep");
				}		
			}
			else {
				xsmcSetString(xsVar(0), "none");
			}
			xsmcSet(xsVar(1), xsID_authentication, xsVar(0));

			xsCallFunction1(gScan->callback, xsGlobal, xsVar(1));
		}
	}
	xsCatch {
	}

	xsResult = gScan->callback;
	xsForget(gScan->callback);
	c_free(gScan);
	gScan = NULL;

	xsCallFunction1(xsResult, xsGlobal, xsNull);		// end of scan

	xsEndHost(the);
}

typedef struct xsWiFiRecord xsWiFiRecord;
typedef xsWiFiRecord *xsWiFi;

struct xsWiFiRecord {
	xsWiFi				next;
	xsMachine			*the;
	xsSlot				obj;
	uint8_t				haveCallback;
};

static xsWiFi gWiFi;

void xs_wifi_destructor(void *data)
{
	xsWiFi wifi = data;

	if (wifi) {
		if (wifi == gWiFi)
			gWiFi = wifi->next;
		else {
			xsWiFi walker;
			for (walker = gWiFi; walker->next != wifi; walker = walker->next)
				;
			walker->next = wifi->next;
		}

		c_free(wifi);
	}
}

void xs_wifi_constructor(xsMachine *the)
{
	int argc = xsmcArgc;

	if (1 == argc)
		xsCall1(xsThis, xsID_build, xsArg(0));
	else if (2 == argc)
		xsCall2(xsThis, xsID_build, xsArg(0), xsArg(1));
}

void xs_wifi_close(xsMachine *the)
{
	xsWiFi wifi = xsmcGetHostData(xsThis);
	xs_wifi_destructor(wifi);
	xsmcSetHostData(xsThis, NULL);
}

void xs_wifi_set_onNotify(xsMachine *the)
{
	xsWiFi wifi = xsmcGetHostData(xsThis);
	uint32_t deviceId = qca4020_wlan_get_active_device();
	
	if (NULL == wifi) {
		wifi = c_calloc(1, sizeof(xsWiFiRecord));
		if (!wifi)
			xsUnknownError("out of memory");
		xsmcSetHostData(xsThis, wifi);
		wifi->the = the;
	}
	else if (wifi->haveCallback) {
		xsmcDelete(xsThis, xsID_callback);
		wifi->haveCallback = false;
	}

	if (!xsmcTest(xsArg(0)))
		return;

	wifi->haveCallback = true;

	wifi->next = gWiFi;
	gWiFi = wifi;

	wifi->obj = xsThis;
	xsmcSet(xsThis, xsID_callback, xsArg(0));

	qapi_WLAN_Set_Callback(deviceId, (qapi_WLAN_Callback_t)wlan_callback_handler, the);
}

void xs_wifi_accessPoint(xsMachine *the)
{
	char *str;
	int32_t channel;
	uint8_t hidden;
	uint32_t beacon_int_in_tu;
	qapi_WLAN_Dev_Mode_e opMode = QAPI_WLAN_DEV_MODE_AP_E;
	qapi_WLAN_Auth_Mode_e authMode;
	qapi_WLAN_Crypt_Type_e crypt;
	uint16_t connected = 0;
	qurt_pipe_attr_t attr;
	qurt_pipe_t pipe;
	uint32_t deviceId = qca4020_wlan_get_active_device();

	qapi_WLAN_Disconnect(deviceId);
	
	qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE, &opMode, sizeof(qapi_WLAN_Dev_Mode_e), FALSE);

	xsmcVars(2);

	xsmcGet(xsVar(0), xsArg(0), xsID_ssid);
	str = xsmcToString(xsVar(0));
	if (c_strlen(str) > __QAPI_WLAN_MAX_SSID_LENGTH)
		xsUnknownError("ssid too long - 32 bytes max");
	qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID, (void *)str, c_strlen(str), FALSE);
		
	authMode = QAPI_WLAN_AUTH_NONE_E;
	crypt = QAPI_WLAN_CRYPT_NONE_E;
	if (xsmcHas(xsArg(0), xsID_password)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_password);
		str = xsmcToString(xsVar(0));
		if (c_strlen(str) > 63)
			xsUnknownError("password too long - 64 bytes max");
		if (c_strlen(str) < 8)
			xsUnknownError("password too short - 8 bytes min");
		if (!c_isEmpty(str)) {
			authMode = QAPI_WLAN_AUTH_WPA2_PSK_E;
			crypt = QAPI_WLAN_CRYPT_AES_CRYPT_E;
			qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY, __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE, (void*)str, c_strlen(str), FALSE);
		}
	}
	qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY, __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE, (void *)&authMode, sizeof(authMode), FALSE);
	qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY, __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE, (void *)&crypt, sizeof(crypt), FALSE);
		
	channel = 1;
	if (xsmcHas(xsArg(0), xsID_channel)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_channel);
		channel = xsmcToInteger(xsVar(0));
		if ((channel < 1) || (channel > 13))
			xsUnknownError("invalid channel");
	}
	qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL, (void*)&channel, sizeof(channel), FALSE);

    hidden = 0;
	if (xsmcHas(xsArg(0), xsID_hidden)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_hidden);
		hidden = xsmcTest(xsVar(0));
	}
	qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_ENABLE_HIDDEN_MODE, &hidden, sizeof(hidden), FALSE);

    beacon_int_in_tu = 100;
	if (xsmcHas(xsArg(0), xsID_interval)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_interval);
		beacon_int_in_tu = xsmcToInteger(xsVar(0));
	}
	qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_BEACON_INTERVAL_IN_TU, &beacon_int_in_tu, sizeof(beacon_int_in_tu), FALSE);

	qurt_pipe_attr_init(&attr);
	qurt_pipe_attr_set_elements(&attr, 1);
	qurt_pipe_attr_set_element_size(&attr, sizeof(connected));
	qurt_pipe_create(&pipe, &attr);
  
  	qapi_WLAN_Set_Callback(deviceId, (qapi_WLAN_Callback_t)ap_callback_handler, pipe);

	qapi_WLAN_Commit(deviceId);

	qurt_pipe_receive_timed(pipe, &connected, 2000);
	qurt_pipe_delete(pipe);
    
	qapi_WLAN_Set_Callback(deviceId, (qapi_WLAN_Callback_t)NULL, NULL);
 	
 	// assign 192.168.10.1 soft-ap IP address
 	// clients will be assigned 192.168.10.10 - 192.168.10.100
 	if (1 == connected) {
    	uint32_t addr, mask, gw, startaddr, endaddr;
    	int leasetime = 0xFFFFFFFF;
       	const char *interface_name;
		qapi_Net_Get_Wlan_Dev_Name(deviceId, &interface_name);	

		inet_pton(AF_INET, "192.168.10.1", &addr);
		inet_pton(AF_INET, "255.255.255.0", &mask);
		inet_pton(AF_INET, "192.168.10.100", &gw);

		if (0 == qapi_Net_IPv4_Config(interface_name, QAPI_NET_IPV4CFG_STATIC_IP_E, &addr, &mask, &gw)) {
			inet_pton(AF_INET, "192.168.10.10", &startaddr);
			inet_pton(AF_INET, "192.168.10.100", &endaddr);
			if (0 != qapi_Net_DHCPv4s_Set_Pool(interface_name, startaddr, endaddr, leasetime))
				connected = 0;
		}
 	}
 	
	if (!connected)
		xsUnknownError("unable to start Wi-Fi AP!");
}

static void wifiEventPending(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	xsWiFi wifi = refcon;
	const char *msg;

	switch (gWiFiStatus) {
		case STATION_CONNECTING:		msg = "start"; break;
		case STATION_GOT_IP:			msg = "gotIP"; break;
		case STATION_WRONG_PASSWORD:
		case STATION_NO_AP_FOUND:
		case STATION_CONNECT_FAIL:		msg = "disconnect"; break;
		default: return;
	}

	xsBeginHost(the);
		xsCall1(wifi->obj, xsID_callback, xsString(msg));
	xsEndHost(the);
}

void wifiConnected(void *the, void *refcon, uint8_t *message, uint16_t messageLength)
{
	uint32_t deviceId = (uint32_t)refcon;
	const char *devName;
	
	qapi_Net_Get_Wlan_Dev_Name(deviceId, &devName);
	
	if (0 == qapi_Net_DHCPv4c_Register_Success_Callback(devName, ipconfig_dhcpc_success_cb))
		qapi_Net_IPv4_Config(devName, QAPI_NET_IPV4CFG_DHCP_IP_E, NULL, NULL, NULL);
}

int32_t wlan_callback_handler(uint8_t deviceId, uint32_t cbId, void *pApplicationContext, void *payload, uint32_t payload_Length)
{
    int32_t result = 0;
    xsMachine *the = (xsMachine*)pApplicationContext;
 	xsWiFi walker;

	switch(cbId) {
    	case QAPI_WLAN_READY_CB_E:
 			gWiFiStatus = STATION_IDLE;
			break;
		case QAPI_WLAN_SCAN_COMPLETE_CB_E:
			gScan->count = __QAPI_MAX_SCAN_RESULT_ENTRY;
			qapi_WLAN_Get_Scan_Results(deviceId, &gScan->results[0], &gScan->count);
			modMessagePostToMachine(the, NULL, 0, deliverScanResults, NULL);
			return 0;
		case QAPI_WLAN_CONNECT_CB_E: {
			qapi_WLAN_Connect_Cb_Info_t *cxnInfo = (qapi_WLAN_Connect_Cb_Info_t *)payload;
			if ((1 == cxnInfo->value) && cxnInfo->bss_Connection_Status) {
				modMessagePostToMachine(the, NULL, 0, wifiConnected, (void*)(uint32_t)deviceId);
				return 0;
			}
			else if ((0 == cxnInfo->value) && cxnInfo->bss_Connection_Status) {
				gWiFiStatus = STATION_CONNECT_FAIL;
			}
			else if (0x10 == cxnInfo->value) {
				// 4-way handshake success
			}
			break;
		}
		case QAPI_WLAN_ERROR_HANDLER_CB_E:
			gWiFiStatus = STATION_CONNECT_FAIL;
			break;
		default:
			return 0;
	}
	
	for (walker = gWiFi; NULL != walker; walker = walker->next)
		modMessagePostToMachine(walker->the, NULL, 0, wifiEventPending, walker);

    return result;
}

int32_t ap_callback_handler(uint8_t deviceId, uint32_t cbId, void *pApplicationContext, void *payload, uint32_t payload_Length)
{
    int32_t result = 0;
	qurt_pipe_t pipe = (qurt_pipe_t)pApplicationContext;
	
	switch(cbId) {
		case QAPI_WLAN_CONNECT_CB_E: {
			qapi_WLAN_Connect_Cb_Info_t *cxnInfo = (qapi_WLAN_Connect_Cb_Info_t *)payload;
			if ((1 == cxnInfo->value) && cxnInfo->bss_Connection_Status) {
				uint16_t connected = 1;
				qurt_pipe_send(pipe, &connected);
			}
			break;
		}
		default:
			break;
	}

    return result;
}

int32_t ipconfig_dhcpc_success_cb(uint32_t addr, uint32_t mask, uint32_t gw)
{
 	uint32_t deviceID = qca4020_wlan_get_active_device();
	qapi_WLAN_Dev_Mode_e opMode;
	uint32_t dataLen;
	
	qapi_WLAN_Get_Param(deviceID, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE, &opMode, &dataLen);
	
	if (opMode == QAPI_WLAN_DEV_MODE_STATION_E) {
		gWiFiStatus = STATION_GOT_IP;
	
		for (xsWiFi walker = gWiFi; NULL != walker; walker = walker->next)
			modMessagePostToMachine(walker->the, NULL, 0, wifiEventPending, walker);
	}

    return 0;
}



