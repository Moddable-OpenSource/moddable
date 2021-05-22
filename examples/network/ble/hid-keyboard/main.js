/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

import BLEHIDKeyboard from "hidkeyboard";

class TerminalKeyboard extends BLEHIDKeyboard {
	onKeyDown(key) {
		trace(key, " ");
		if (13 === key)
			trace("\n> ");
	}
	onDeviceConnected(device) {
		trace("Pairing keyboard...\n")
	}
	onDeviceReady() {
		trace("Pairing complete.\n");
		trace("\n> ");
	}
}

const keyboard = new TerminalKeyboard;
