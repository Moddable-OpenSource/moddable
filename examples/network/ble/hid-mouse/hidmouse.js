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
		if (buttons != this.buttons) {
			let up = 0;
			let down = 0;
			this.buttons = buttons;
		}
		if (x != this.x || y != this.y) {
			this.x = x;
			this.y = y;
			this.onMoved(x, y);
		}
	}
	onButtonDown(buttons) {
		debugger;
	}
	onButtonUp(buttons) {
		debugger;
	}
	onMoved(x, y) {
		debugger;
	}
}

export default BLEHIDMouse;

