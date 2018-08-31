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
 https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.human_interface_device.xml
 https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.report.xml
 https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.report_reference.xml
 
 http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
 https://docs.mbed.com/docs/ble-hid/en/latest/api/md_doc_HID.html
 */

import BLEClient from "bleclient";
import {uuid} from "btutils";
import {SM, IOCapability, Authorization} from "sm";

const HID_SERVICE_UUID = uuid`1812`;
const REPORT_CHARACTERISTIC_UUID = uuid`2A4D`;
const REPORT_REFERENCE_DESCRIPTOR_UUID = uuid`2908`;

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

const ReportType = {
	INPUT: 0x01,
	OUTPUT: 0x02,
	FEATURE: 0x03
};
Object.freeze(ReportType);

const Codes40 = [13,27,8,9,32];									// key codes 40 - 44
const Codes45 = [45,61,91,93,92,35,59,39,96,44,46,47];			// key codes 45 - 56
const Codes84 = [47,42,45,43,13,49,50,51,52,53,54,55,56,57,48,46];	// key codes 84 - 99
const ShiftCodes45 = [95,43,123,125,124,126,58,34,126,60,62,63];	// shifted key codes 45 - 56
const ShiftCodes30 = [33,64,35,36,37,94,38,42,40],				// shifted key codes 30 - 38

class BLEHIDClient extends BLEClient {
	configure(params) {
		this.reportType = params.reportType;
	}
	onReady() {
		SM.securityParameters = { mitm:true };
	}
	onSecurityParameters(params) {
		this.startScanning();
	}
	onDiscovered(device) {
		let found = false;
		let uuids = device.scanResponse.completeUUID16List;
		if (uuids)
			found = uuids.find(uuid => uuid.equals(HID_SERVICE_UUID));
		if (!found) {
			uuids = device.scanResponse.incompleteUUID16List;
			if (uuids)
				found = uuids.find(uuid => uuid.equals(HID_SERVICE_UUID));
		}
		if (found) {
			this.stopScanning();
			this.connect(device);
		}
	}
	onConnected(device) {
		this.onDeviceConnected();
		device.discoverPrimaryService(HID_SERVICE_UUID);
	}
	onDisconnected() {
		this.onDeviceDisconnected();
		this.startScanning();
	}
	onServices(services) {
		if (services.length)
			services[0].discoverCharacteristic(REPORT_CHARACTERISTIC_UUID);
	}
	onCharacteristics(characteristics) {
		this.reports = [];
		for (let i = 0; i < characteristics.length; ++i) {
			let characteristic = characteristics[i];
			this.reports.push(characteristic);
		}
		if (this.reports.length) {
			this.reportIndex = 0;
			this.reports[this.reportIndex].discoverAllDescriptors();
		}
	}
	onDescriptors(descriptors) {
		for (let i = 0; i < descriptors.length; ++i) {
			let descriptor = descriptors[i];
			if (descriptor.uuid.equals(REPORT_REFERENCE_DESCRIPTOR_UUID)) {
				this.descriptorIndex = i;
				descriptor.readValue(Authorization.NoMITM);
				return;
			}
		}
		if (++this.reportIndex < this.reports.length)
			this.reports[this.reportIndex].discoverAllDescriptors();		
	}
	onDescriptorValue(descriptor, buffer) {
		let bytes = new Uint8Array(buffer);
		let reportType = bytes[1];
		if (this.reportType == reportType) {
			this.onDeviceReady();
			descriptor.characteristic.enableNotifications();
			return;
		}
		else {
			let characteristic = descriptor.characteristic;
			if (++this.descriptorIndex < characteristic.descriptors.length) {
				let descriptor = characteristic.descriptors[this.descriptorIndex];
				descriptor.readValue(Authorization.NoMITM);
				return;
			}
		}
		if (++this.reportIndex < this.reports.length)
			this.reports[this.reportIndex].discoverAllDescriptors();		
	}
	onCharacteristicNotification(characteristic, buffer) {
		this.onReport(new Uint8Array(buffer));
	}
	onDeviceConnected() {
		debugger;
	}
	onDeviceDisconnected() {
		debugger;
	}
	onDeviceReady() {
		debugger;
	}
	onReport() {
		debugger;
	}
}

class BLEHIDKeyboard extends BLEHIDClient {
	constructor() {
		super();
		this.configure({ reportType:ReportType.INPUT })
		this.lastKey = 0;
		this.lastKeyTime = Date.now();
	}
	debounce(key) {
		let now = Date.now();
		if ((0 == key) || (key == this.lastKey && (now - this.lastKeyTime < 100)))
			key = 0;
		this.lastKey = key;
		this.lastKeyTime = now;
		return key;
	}
	onDeviceConnected() {
	}
	onDeviceDisconnected() {
		trace("\nKeyboard disconnected.\n");
	}
	onDeviceReady() {
		trace("Keyboard ready.\n\n");
		trace("> ");
	}
	onKeyCode(code) {
		//trace(`Character code: ${code}\n`);
		if (code >= 32 && code <= 126)
			trace(String.fromCharCode(code))
		else if (code == 13)
			trace("\n> ");
	}
	onReport(report) {
		// This is the 8 byte HID keyboard report
		let key = this.debounce(report[2]);
		if (0 == key) return;
		
		let modifier = report[0];
		let shift = modifier & (Modifiers.LEFT_SHIFT | Modifiers.RIGHT_SHIFT);
		let code = 0;
		
		if (key >= 4 && key <= 29) {		// a-z
			key -= 4;
			code = shift ? key + 65 : key + 97;
		}
		else if (key >= 30 && key <= 38) {	// 1-9
			key -= 30;
			code = shift ? ShiftCodes30[key] : key + 49;
		}
		else if (key == 39)
			code = shift ? 41 : 48;	// ) or 0
		else if (key >= 40 && key <= 44) {
			key -= 40;
			code = Codes40[key];
		}
		else if (key >= 45 && key <= 56) {
			key -= 45;
			code = shift ? ShiftCodes45[key] : Codes45[key];
		}
		else if (key >= 84 && key <= 99) {
			key -= 84;
			code = Codes84[key];
		}
		else {
			//trace(`unhandled report key ${key}\n`);
		}
		if (0 != code)
			this.onKeyCode(code);
	}
}

let keyboard = new BLEHIDKeyboard;
