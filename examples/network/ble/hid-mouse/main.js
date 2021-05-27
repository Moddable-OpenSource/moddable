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

import BLEHIDMouse from "hidmouse";

class Mouse extends BLEHIDMouse {
	onMouseReady() {
		this.onMoved(0, 0, 0);
	}
	onButtonDown(x, y, buttons) {
		if (buttons & 0x01)
			trace(`x: ${x}, y: ${y}, button 0 down\n`);
		else if (buttons & 0x02)
			trace(`x: ${x}, y: ${y}, button 1 down\n`);
		else if (buttons & 0x04)
			trace(`x: ${x}, y: ${y}, button 2 down\n`);
	}
	onButtonUp(x, y, buttons) {
		if (buttons & 0x01)
			trace(`x: ${x}, y: ${y}, button 0 up\n`);
		else if (buttons & 0x02)
			trace(`x: ${x}, y: ${y}, button 1 up\n`);
		else if (buttons & 0x04)
			trace(`x: ${x}, y: ${y}, button 2 up\n`);
	}
	onMoved(x, y, buttons) {
		trace(`x: ${x}, y: ${y}\n`);
	}
}

let mouse = new Mouse({ bonding:false });
