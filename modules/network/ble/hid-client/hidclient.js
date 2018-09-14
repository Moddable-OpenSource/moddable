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
 
 http://www.usb.org/developers/hidpage/HID1_11.pdf
 http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
 https://docs.mbed.com/docs/ble-hid/en/latest/api/md_doc_HID.html
 */

import BLEClient from "bleclient";
import {uuid} from "btutils";
import {SM, IOCapability, Authorization} from "sm";

const ReportType = {
	INPUT: 0x01,
	OUTPUT: 0x02,
	FEATURE: 0x03
};
Object.freeze(ReportType);

const UsageID = {
	POINTER: 1,
	MOUSE: 2,
	JOYSTICK: 4,
	GAMEPAD: 5,
	KEYBOARD: 6,
	KEYPAD: 7
};
Object.freeze(UsageID);

class BLEHIDClient extends BLEClient {
	constructor() {
		super();
		this.HID_SERVICE_UUID = uuid`1812`;
		this.REPORT_CHARACTERISTIC_UUID = uuid`2A4D`;
		this.REPORT_MAP_CHARACTERISTIC_UUID = uuid`2A4B`;
		this.REPORT_REFERENCE_DESCRIPTOR_UUID = uuid`2908`;
	}
	configure(params) {
		this.reportTypes = params.reportTypes;
		this.usageID = params.usageID;
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
			found = uuids.find(uuid => uuid.equals(this.HID_SERVICE_UUID));
		if (!found) {
			uuids = device.scanResponse.incompleteUUID16List;
			if (uuids)
				found = uuids.find(uuid => uuid.equals(this.HID_SERVICE_UUID));
		}
		if (found) {
			this.stopScanning();
			this.connect(device);
		}
	}
	onConnected(device) {
		this.onDeviceConnected();
		device.discoverPrimaryService(this.HID_SERVICE_UUID);
	}
	onDisconnected() {
		this.onDeviceDisconnected();
		this.startScanning();
	}
	onServices(services) {
		if (services.length)
			services[0].discoverCharacteristic(this.REPORT_MAP_CHARACTERISTIC_UUID);
	}
	onCharacteristics(characteristics) {
		let count = characteristics.length;
		if (0 == count) return;
		
		this.reports = [];
		for (let i = 0; i < count; ++i) {
			let characteristic = characteristics[i];
			if (characteristic.uuid.equals(this.REPORT_MAP_CHARACTERISTIC_UUID)) {
				characteristic.readValue(Authorization.NoMITM);
				return;
			}
			else if (characteristic.uuid.equals(this.REPORT_CHARACTERISTIC_UUID)) {
				this.reports.push({ characteristic, reportType:0 });
			}
		}
		this.reportIndex = 0;
		this.reports[this.reportIndex].characteristic.discoverAllDescriptors();
	}
	onCharacteristicValue(characteristic, buffer) {
		let bytes = new Uint8Array(buffer);
		let usageID = bytes[3];
		if (usageID == this.usageID) {
			characteristic.service.discoverCharacteristic(this.REPORT_CHARACTERISTIC_UUID);
		}
		else {
			// wrong hid device!
			// @@ disconnect from this device
		}
	}
	onDescriptors(descriptors) {
		let descriptor = descriptors.find(descriptor => descriptor.uuid.equals(this.REPORT_REFERENCE_DESCRIPTOR_UUID));
		if (descriptor)
			descriptor.readValue(Authorization.NoMITM);
	}
	onDescriptorValue(descriptor, buffer) {
		let bytes = new Uint8Array(buffer);
		let reportType = bytes[1];
		this.reports[this.reportIndex].reportType = reportType;
		if (++this.reportIndex < this.reports.length)
			this.reports[this.reportIndex].characteristic.discoverAllDescriptors();
		else {
			let matched = [];
			this.reports.forEach(report => {
				this.reportTypes.forEach(reportType => {
					if (report.reportType == reportType)
						matched.push(report);
				});
			});
			if (matched.length) {
				this.onDeviceReports(matched);
				this.onDeviceReady();
			}
		}		
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
	onDeviceReports(reports) {
		debugger;
	}
}

export {BLEHIDClient as default, BLEHIDClient, ReportType, UsageID};

