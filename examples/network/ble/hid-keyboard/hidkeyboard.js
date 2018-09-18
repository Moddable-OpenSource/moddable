/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */
/*
/*
 https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.human_interface_device.xml
 https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.report.xml
 https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.report_reference.xml
 
 http://www.usb.org/developers/hidpage/HID1_11.pdf
 http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
 https://docs.mbed.com/docs/ble-hid/en/latest/api/md_doc_HID.html
*/

import {BLEHIDClient, ReportType, UsageID} from "hidclient";

const Modifiers = {
	LEFT_CONTROL: 0x01,
	LEFT_SHIFT: 0x02,
	LEFT_ALT: 0x04,
	LEFT_GUI: 0x08,
	RIGHT_CONTROL: 0x10,
	RIGHT_SHIFT: 0x20,
	RIGHT_ALT: 0x40,
	RIGHT_GUI: 0x80
};
Object.freeze(Modifiers);

const Indicators = {
	NUM_LOCK: 0x01,
	CAPS_LOCK: 0x02,
	SCROLL_LOCK: 0x04,
	COMPOSE: 0x08,
	KANA: 0x10,
	CONSTANT0: 0x20,
	CONSTANT1: 0x40,
	CONSTANT2: 0x80
};
Object.freeze(Indicators);

const Codes40 = [13,27,8,9,32];										// key codes 40 - 44
const Codes45 = [45,61,91,93,92,35,59,39,96,44,46,47];				// key codes 45 - 56
const Codes79 = [67, 68, 66, 65];									// key codes 79 - 82 (arrows)
const Codes84 = [47,42,45,43,13];									// key codes 84 - 88
const Codes89 = [35,40,34,37,53,39,36,38,33,45,46];					// key codes 89 - 99
const ShiftCodes45 = [95,43,123,125,124,126,58,34,126,60,62,63];	// shifted key codes 45 - 56
const ShiftCodes30 = [33,64,35,36,37,94,38,42,40],					// shifted key codes 30 - 38
const NumLockCodes89 = [49,50,51,52,53,54,55,56,57,48,46];			// num lock key codes 89 - 99

class BLEHIDKeyboard extends BLEHIDClient {
	constructor() {
		super();
		this.configure({ usageID: UsageID.KEYBOARD, reportTypes:[ReportType.INPUT, ReportType.OUTPUT] });
		this.onDeviceDisconnected();
	}
	onCharacteristicNotification(characteristic, buffer) {
		this.onReportData(new Uint8Array(buffer));
	}
	onDeviceDisconnected() {
		this.lastKeyCount = 0;
		this.lastKeys = new Uint8Array(6);
		this.keys = new Uint8Array(6);
		this.indicators = 0;
		this.outputReportCharacteristic = null;
	}
	onDeviceReportMap(buffer) {
	}
	onDeviceReports(reports) {
		let enabledNotifications = false;
		reports.forEach(report => {
			if (ReportType.OUTPUT == report.reportType)
				this.outputReportCharacteristic = report.characteristic;
			else if (!enabledNotifications && (ReportType.INPUT == report.reportType)) {
				enabledNotifications = true;
				report.characteristic.enableNotifications();
			}
		});
	}
	onReportData(report) {
		// This is the 8 byte HID keyboard report
		//trace(report.join(' ') + '\n');
		//return;
		let i, j, found, key;
		let keyCount = 0;
		for (i = 2; i < 8; ++i) {
			let key = report[i];
			if (0 == key)
				break;
			for (j = 0, found = false; j < this.lastKeyCount; ++j) {
				if (this.lastKeys[j] == key) {
					found = true;
					break;
				}
			}
			if (!found)
				this.keys[keyCount++] = key;
		}
		
		this.lastKeyCount = i - 2;
		for (i = 0; i < this.lastKeyCount; ++i)
			this.lastKeys[i] = report[2 + i];
		
		/**	
		if (keyCount)
			trace("take: " + this.keys.slice(0, keyCount).join(' ') + "\n");
		else
			trace("take: none\n");
		**/
		let modifier = report[0];
		
		for (i = 0; i < keyCount; ++i) {
			let shift = this.capsLock || (modifier & (Modifiers.LEFT_SHIFT | Modifiers.RIGHT_SHIFT));
			let code = 0;
			let escape;
		
			key = this.keys[i];
			
			if (key >= 4 && key <= 29) {		// a-z
				key -= 4;
				code = shift ? key + 65 : key + 97;
			}
			else if (key >= 30 && key <= 38) {	// 1-9
				key -= 30;
				code = shift ? ShiftCodes30[key] : key + 49;
			}
			else if (key == 39)
				code = shift ? 41 : 48;			// ) or 0
			else if (key >= 40 && key <= 44) {
				key -= 40;
				code = Codes40[key];
			}
			else if (key >= 45 && key <= 56) {	// - to /
				key -= 45;
				code = shift ? ShiftCodes45[key] : Codes45[key];
			}
			else if (key == 57) {				// caps lock
				this.indicators ^= Indicators.CAPS_LOCK;
				this.updateIndicators();
			}
			else if (key == 71) {				// scroll lock
				this.indicators ^= Indicators.SCROLL_LOCK;
				this.updateIndicators();
			}
			if ((79 <= key) && (key <= 82)) {	// arrow keys
				key -= 79;
				code = Codes79[key];
				escape = true;
			}
			else if (key == 83) {				// num lock
				this.indicators ^= Indicators.NUM_LOCK;
				this.updateIndicators();
			}
			else if (key >= 84 && key <= 88) {	// keypad / to keypad enter
				key -= 84;
				code = Codes84[key];
			}
			else if (key >= 89 && key <= 99) {	// keypad 1 to keypad .
				key -= 89;
				code = this.numLock ? NumLockCodes89[key] : Codes89[key];
			}
			else {
				//trace(`unhandled report key ${key}\n`);
			}
			if (0 != code) {
				if (escape)
					this.onCharCode(27);
				this.onCharCode(code);
			}
		}
	}
	updateIndicators() {
		if (this.outputReportCharacteristic)
			this.outputReportCharacteristic.writeWithoutResponse(Uint8Array.of(this.indicators).buffer);
	}
}

export default BLEHIDKeyboard;

