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
#include "mc.xs.h"
#include "modBLE.h"

#include "FreeRTOSConfig.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_bt_defs.h"

void xs_ble_smp_delete_bonding(xsMachine *the)
{
	uint8_t *address = (uint8_t*)xsmcToArrayBuffer(xsArg(0));
    int dev_num = esp_ble_get_bond_device_num();
    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)c_malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    if (!dev_list)
		xsUnknownError("no memory");
    esp_ble_get_bond_device_list(&dev_num, dev_list);
    for (int i = 0; i < dev_num; i++) {
		if (0 == c_memcmp(address, dev_list[i].bd_addr, 6)) {
			esp_ble_remove_bond_device(address);
			break;
		}
	}
    c_free(dev_list);
}

void xs_ble_smp_delete_all_bondings(xsMachine *the)
{
    int dev_num = esp_ble_get_bond_device_num();
    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)c_malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    if (!dev_list)
		xsUnknownError("no memory");
    esp_ble_get_bond_device_list(&dev_num, dev_list);
    for (int i = 0; i < dev_num; i++)
        esp_ble_remove_bond_device(dev_list[i].bd_addr);
    c_free(dev_list);
}

void xs_ble_smp_set_security_parameters(xsMachine *the)
{
	uint8_t encryption = xsmcToBoolean(xsArg(0));
	uint8_t bonding = xsmcToBoolean(xsArg(1));
	uint8_t mitm = xsmcToBoolean(xsArg(2));
	uint16_t ioCapability = xsmcToInteger(xsArg(3));
	uint8_t key_size = 16;
	uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	esp_ble_auth_req_t auth_req;
	uint8_t iocap;
 	switch(ioCapability) {
 		case NoInputNoOutput:
 			iocap = ESP_IO_CAP_NONE;
 			break;
 		case DisplayOnly:
 			iocap = ESP_IO_CAP_OUT;
 			break;
 		case KeyboardOnly:
 			iocap = ESP_IO_CAP_IN;
 			break;
 		case KeyboardDisplay:
 			iocap = ESP_IO_CAP_KBDISP;
 			break;
 		case DisplayYesNo:
 			iocap = ESP_IO_CAP_IO;
 			break;
 	} 	
	if (encryption) {
		if (bonding)
			auth_req = (mitm ? ESP_LE_AUTH_REQ_SC_MITM_BOND : ESP_LE_AUTH_REQ_SC_BOND);
		else
			auth_req = (mitm ? ESP_LE_AUTH_REQ_SC_MITM : ESP_LE_AUTH_REQ_SC_ONLY);
	}
	else {
		if (bonding)
			auth_req = (mitm ? ESP_LE_AUTH_REQ_MITM : ESP_LE_AUTH_BOND);
		else
			auth_req = (mitm ? ESP_LE_AUTH_REQ_MITM : ESP_LE_AUTH_NO_BOND);
	}
	
	setSecurityParameters(encryption, bonding, mitm);
	
 	esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
 	esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
 	esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
 	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
 	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
}

