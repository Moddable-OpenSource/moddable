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

class BLEHIDMouse extends BLEHIDClient {
	constructor() {
		super();
		this.configure({ usageID: UsageID.MOUSE, reportTypes:[ReportType.INPUT] });
		this.onDeviceDisconnected();
	}
	onCharacteristicNotification(characteristic, buffer) {
		this.onReportData(new Int8Array(buffer));
	}
	onDeviceDisconnected() {
		this.buttons = 0;
		this.x = this.y = -1;
	}
	onDeviceReports(reports) {
		if (reports.length)
			reports[0].characteristic.enableNotifications();
	}
	onReportData(report) {
		// This is the 3 byte HID mouse report
		//trace(report.join(' ') + '\n');
		//return;
		let buttons = report[0];
		let x = report[1];
		let y = report[2];
		let up = 0;
		let down = 0;
		if (buttons != this.buttons) {
			let mask = 0x1;
			for (let i = 0; i < 3; ++i) {
				if ((buttons & mask) != (this.buttons & mask)) {
					if (buttons & mask)
						down |= mask;
					else
						up |= mask;
				}
				mask <<= 1;
			}
		}
		if (down != 0)
			this.onButtonDown(x, y, down);
		else if (up != 0)
			this.onButtonUp(x, y, up);
		else if (x != 0 || y != 0) {
			this.onMoved(x, y, buttons);
		}
		this.x = x;
		this.y = y;
		this.buttons = buttons;
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

