/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */
/*
 * Portions based on Kinoma LowPAN Framework: Kinoma Bluetooth 4.2 Stack
 */

const GAP = {};

GAP.SCAN_FAST_INTERVAL = 0x0030;		// TGAP(scan_fast_interval)		30ms to 60ms
GAP.SCAN_FAST_WINDOW = 0x0030;			// TGAP(scan_fast_window)		30ms
GAP.SCAN_SLOW_INTERVAL1 = 0x0800;		// TGAP(scan_slow_interval1)	1.28s
GAP.SCAN_SLOW_WINDOW1 = 0x0012;			// TGAP(scan_slow_window1)		11.25ms
GAP.SCAN_SLOW_INTERVAL2 = 0x1000;		// TGAP(scan_slow_interval2)	2.56s
GAP.SCAN_SLOW_WINDOW2 = 0x0024;			// TGAP(scan_slow_window2)		22.5ms
GAP.ADV_FAST_INTERVAL1 = {				// TGAP(adv_fast_interval1)		30ms to 60ms
	min: 0x0030,
	max: 0x0060
};
GAP.ADV_FAST_INTERVAL2 = {				// TGAP(adv_fast_interval2)		100ms to 150ms
	min: 0x00A0,
	max: 0x00F0
};
GAP.ADV_SLOW_INTERVAL = {				// TGAP(adv_slow_interval)		1s to 1.2s
	min: 0x0640,
	max: 0x0780
};
GAP.ADType = {
	/* Service UUID */
	INCOMPLETE_UUID16_LIST: 0x02,
	COMPLETE_UUID16_LIST: 0x03,
	INCOMPLETE_UUID128_LIST: 0x06,
	COMPLETE_UUID128_LIST: 0x07,
	/* Local Name */
	SHORTENED_LOCAL_NAME: 0x08,
	COMPLETE_LOCAL_NAME: 0x09,
	/* Flags */
	FLAGS: 0x01,
	/* Manufacturer Specific Data */
	MANUFACTURER_SPECIFIC_DATA: 0xFF,
	/* TX Power Level */
	TX_POWER_LEVEL: 0x0A,
	SLAVE_CONNECTION_INTERVAL_RANGE: 0x12,
	/* Service Solicitation */
	SOLICITATION_UUID16_LIST: 0x14,
	SOLICITATION_UUID128_LIST: 0x15,
	/* Service Data */
	SERVICE_DATA_UUID16: 0x16,
	SERVICE_DATA_UUID128: 0x21,
	/* Appearance */
	APPEARANCE: 0x19,
	/* Public Target Address */
	PUBLIC_TARGET_ADDRESS: 0x17,
	/* Random Target Address */
	RANDOM_TARGET_ADDRESS: 0x18,
	/* Advertising Interval */
	ADVERTISING_INTERVAL: 0x1A,
	/* LE Bluetooth Device Address */
	LE_BLUETOOTH_DEVICE_ADDRESS: 0x1B,
	/* LE Role */
	LE_ROLE: 0x1C,
	/* URI */
	URI: 0x24
};
GAP.MAX_AD_LENGTH = 31;
GAP.ADFlag = {
	LE_LIMITED_DISCOVERABLE_MODE: 0x01,
	LE_GENERAL_DISCOVERABLE_MODE: 0x02,
	NO_BR_EDR: 0x04,
	LE_BR_EDR_CONTROLLER: 0x08,
	LE_BR_EDR_HOST: 0x10,
};
GAP.AddressType = {
	PUBLIC: 0,
	RANDOM: 1,
	RPA_PUBLIC: 2,
	RPA_RANDOM: 3
};
GAP.ScanFilterPolicy = {
	NONE: 0,
	WHITELIST: 1,
	NOT_RESOLVED_DIRECTED: 2,
	WHITELIST_NOT_RESOLVED_DIRECTED: 3
};
GAP.AdvFilterPolicy = {
	NONE: 0,
	WHITELIST_SCANS: 1,
	WHITELIST_CONNECTIONS: 2,
	WHITELIST_SCANS_CONNECTIONS: 3
};

Object.freeze(GAP, true);

export default GAP;