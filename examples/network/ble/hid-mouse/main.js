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

import BLEHIDMouse from "hidmouse";

class Mouse extends BLEHIDMouse {
	onDeviceConnected() {
		trace("Pairing mouse...\n")
	}
	onDeviceReady() {
		trace("Pairing complete.\n");
	}
	onButtonDown(buttons) {
		if (buttons & 0x01)
			trace(`button 0 down\n`);
		if (buttons & 0x02)
			trace(`button 1 down\n`);
		if (buttons & 0x04)
			trace(`button 2 down\n`);
	}
	onButtonUp(buttons) {
		if (!(buttons & 0x01))
			trace(`button 0 up\n`);
		if (!(buttons & 0x02))
			trace(`button 1 up\n`);
		if (!(buttons & 0x04))
			trace(`button 2 up\n`);
	}
	onMoved(x, y) {
		trace(`x: ${x}, y: ${y}\n`);
	}
}

let mouse = new Mouse;
