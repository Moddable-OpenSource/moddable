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
	https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Services/org.bluetooth.service.human_interface_device.xml
	https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.report.xml
	https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Descriptors/org.bluetooth.descriptor.report_reference.xml
 
	http://www.usb.org/developers/hidpage/HID1_11.pdf
	http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
	https://docs.mbed.com/docs/ble-hid/en/latest/api/md_doc_HID.html
*/

import {BLEHIDClient, ReportType, UsageID} from "hidclient";

class BLEHIDMouse extends BLEHIDClient {
	#mouseReady;
	
	constructor(dictionary = { bonding:false }) {
		super();
		this.configure({ usageID: UsageID.MOUSE, reportTypes:[ReportType.INPUT], bonding:dictionary.bonding });
		this.onDeviceDisconnected();
	}
	onCharacteristicNotification(characteristic, value) {
		this.onReportData(new DataView(value.buffer));
	}
	onDeviceDisconnected() {
		this.buttons = 0;
	}
	onDeviceConnected(device) {
		trace("Pairing mouse...\n");
	}
	onDeviceReady() {
		trace("Pairing complete.\n");
		this.onMouseReady();
		this.#mouseReady = true;
	}
	onDeviceReportMap(buffer) {
		// @@ TBD - parse report map for mouse data payload format
	}
	onDeviceReports(reports) {
		if (reports.length)
			reports[0].characteristic.enableNotifications();
	}
	onReportData(view) {
		if (!this.#mouseReady)
			return;
			
		// This is the HID mouse report
		//trace(report.join(' ') + '\n');
		//return;
		
		// This example assumes the following payload format:
		// byte 0: buttons - 1 bit per button
		// bytes 1 - 2: 16-bit little endian X relative value
		// bytes 3 - 4: 16-bit little endian Y relative value
		let buttons = view.getUint8(0);
		let x = view.getInt16(1, true);
		let y = view.getInt16(3, true);
		let up = 0;
		let down = 0;
		if (buttons != this.buttons) {
			for (let i = 0, mask = 0x1; i < 8; ++i, mask <<= 1) {
				if ((buttons & mask) != (this.buttons & mask)) {
					if (buttons & mask)
						down |= mask;
					else
						up |= mask;
				}
			}
		}
		if (down != 0)
			this.onButtonDown(x, y, down);
		else if (up != 0)
			this.onButtonUp(x, y, up);
		else if (x != 0 || y != 0) {
			this.onMoved(x, y, buttons);
		}
		this.buttons = buttons;
	}
	onMouseReady() {
	}
	onButtonDown(x, y, buttons) {
		debugger;
	}
	onButtonUp(x, y, buttons) {
		debugger;
	}
	onMoved(x, y, buttons) {
		debugger;
	}
}

export default BLEHIDMouse;

