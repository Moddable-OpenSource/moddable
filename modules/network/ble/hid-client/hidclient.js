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
 */
/*
 	https://www.bluetooth.org/docman/handlers/downloaddoc.ashx?doc_id=309012&_ga=2.38842976.279320072.1537230968-1286694985.1517851833
	https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Services/org.bluetooth.service.human_interface_device.xml
	https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.report.xml
	https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Descriptors/org.bluetooth.descriptor.report_reference.xml
 
	http://www.usb.org/developers/hidpage/HID1_11.pdf
	http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
	https://docs.mbed.com/docs/ble-hid/en/latest/api/md_doc_HID.html
*/

import BLEClient from "bleclient";
import {Bytes, uuid} from "btutils";
import {Service, Characteristic} from "gatt";
import {SM, Authorization} from "sm";
import Preference from "preference";
import Timer from "timer";

const ReportType = {
	NONE: 0x00,
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

const Appearance = {
	GENERIC: 960,
	KEYBOARD: 961,
	MOUSE: 962,
	JOYSTICK: 963,
	GAMEPAD: 964
};
Object.freeze(Appearance);

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
		this.bonding = params.bonding;
		if (this.bonding)
			this.bonded = Bonded.get(this.usageID);
		else
			Bonded.remove(this.usageID);
	}
	onReady() {
		this.securityParameters = { mitm:true, bonding:this.bonding };
	}
	onSecurityParameters(params) {
		this.start();
	}
	start() {
		if (this.bonded) {
			let bonded = this.bonded;
			const address = new Bytes(bonded.device.address);
			const addressType = bonded.device.addressType;
			const device = { address, addressType };
			this.connecting = true;
			this.connect(device);
			
			// set a timeout to re-pair in case hid device no longer stores the bond
			this.timer = Timer.set(() => {
				delete this.connecting;
				delete this.bonded;
				delete this.timer;
				this.scan();
			}, 5000);
			return;
		}
		this.scan();
	}
	scan() {
		this.startScanning({ duplicates:false });
	}
	onDiscovered(device) {
		if (this.connecting)
			return;
		let found = false;
		let uuids = device.scanResponse.completeUUID16List;
		if (uuids)
			found = uuids.find(uuid => uuid.equals(this.HID_SERVICE_UUID));
		if (!found) {
			uuids = device.scanResponse.incompleteUUID16List;
			if (uuids)
				found = uuids.find(uuid => uuid.equals(this.HID_SERVICE_UUID));
		}
		if (!found) {
			let appearance = device.scanResponse.appearance;
			if (appearance) {
				switch(this.usageID) {
					case UsageID.MOUSE: found = (Appearance.MOUSE == appearance); break;
					case UsageID.JOYSTICK: found = (Appearance.JOYSTICK == appearance); break;
					case UsageID.GAMEPAD: found = (Appearance.GAMEPAD == appearance); break;
					case UsageID.KEYBOARD: found = (Appearance.KEYBOARD == appearance); break;
					default:
						break;
				}
			}
		}
		if (found) {
			this.stopScanning();
			this.connecting = true;
			this.connect(device);
		}
	}
	onConnected(device) {
		this.device = device;
		if (this.timer) {
			Timer.clear(this.timer);
			delete this.timer;
		}
		if (!this.bonded) {
			this.onDeviceConnected(device);
			this.device.discoverPrimaryService(this.HID_SERVICE_UUID);
		}
	}
	onDisconnected() {
		delete this.device;
		delete this.connecting;
		this.onDeviceDisconnected();
		this.start();
	}
	onAuthenticated(params) {
		if (this.bonded && params.bonded) {
			this.device.services = this.bonded.device.services;
			this.bonded.reports.forEach(report => report.characteristic.connection = this.device.connection);
			this.onDeviceConnected(this.device);
			this.onDeviceReports(this.bonded.reports);
			this.onDeviceReady();
		}
	}
	onServices(services) {
		if (services.length) {
			this.service = services[0];
			this.service.discoverCharacteristic(this.REPORT_MAP_CHARACTERISTIC_UUID);
		}
	}
	onCharacteristics(characteristics) {
		const count = characteristics.length;
		if (0 == count) return;
		
		this.reports = [];
		for (let i = 0; i < count; ++i) {
			let characteristic = characteristics[i];
			if (characteristic.uuid.equals(this.REPORT_MAP_CHARACTERISTIC_UUID)) {
				characteristic.readValue(Authorization.NoMITM);
				return;
			}
			else if (characteristic.uuid.equals(this.REPORT_CHARACTERISTIC_UUID)) {
				this.reports.push({ characteristic, reportType:ReportType.NONE });
			}
		}
		this.reportIndex = 0;
		this.reports[this.reportIndex].characteristic.discoverAllDescriptors();
	}
	onCharacteristicValue(characteristic, value) {
		let usageID = value[3];
		if (usageID == this.usageID) {
			this.onDeviceReportMap(value);
			characteristic.service.discoverCharacteristic(this.REPORT_CHARACTERISTIC_UUID);
		}
		else {
			// wrong hid device!
			this.device.close();
		}
	}
	onDescriptors(descriptors) {
		let descriptor = descriptors.find(descriptor => descriptor.uuid.equals(this.REPORT_REFERENCE_DESCRIPTOR_UUID));
		if (descriptor)
			descriptor.readValue(Authorization.NoMITM);
	}
	onDescriptorValue(descriptor, value) {
		let reportType = value[1];
		let matched = [];
		this.reports[this.reportIndex].reportType = reportType;
		if (1 == this.reportTypes.length && this.reportTypes[0] == reportType)
			matched.push(this.reports[this.reportIndex]);
		else if (++this.reportIndex < this.reports.length)
			this.reports[this.reportIndex].characteristic.discoverAllDescriptors();
		else {
			this.reports.forEach(report => {
				this.reportTypes.forEach(reportType => {
					if (report.reportType == reportType)
						matched.push(report);
				});
			});
		}		
		if (matched.length) {
			if (this.bonding)
				Bonded.set(this.usageID, this.device, matched);
			this.onDeviceReports(matched);
			this.onDeviceReady();
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
	onDeviceReportMap(buffer) {
		debugger;
	}
	onDeviceReports(reports) {
		debugger;
	}
}

/*
	Preference format:
	
	- length (1)
	- address (6)
	- address type (1)
	- service start handle (2)
	- service end handle (2)
	- report count (1)
	- for each report:
		- report type (1)
		- characteristic handle (2)
*/
class Bonded {
	static get(usageID) {
		const buffer = Preference.get(Bonded.PREFERENCE_DOMAIN, Bonded.key(usageID));
		if (buffer) {
			let index = 1;
			const entry = new Uint8Array(buffer);
			const address = entry.slice(index, index + 6).buffer;
			index += 6;
			const addressType = entry[index++];
			const serviceStartHandle = entry[index++] | (entry[index++] << 8);
			const serviceEndHandle = entry[index++] | (entry[index++] << 8);
			const reportCount = entry[index++];
			const reports = new Array(reportCount);
			const services = [new Service({ start:serviceStartHandle, end:serviceEndHandle })];
			for (let i = 0; i < reportCount; ++i) {
				const reportType = entry[index++];
				const handle = entry[index++] | (entry[index++] << 8);
				const characteristic = new Characteristic({ handle, type:"Uint8Array" });
				services[0].characteristics.push(characteristic);
				reports[i] = { reportType, characteristic };
			}
			const bonded = {
				device: { address, addressType, services },
				reports
			};
			return bonded;
		}
	}
	static set(usageID, device, reports) {
		const length = 13 + ((1 + 2) * reports.length);
		let entry = new Uint8Array(length);
		let index = 0;
		entry[index++] = length;
		entry.set(new Uint8Array(device.address), index);
		index += 6;
		entry[index++] = device.addressType;
		entry[index++] = device.services[0].start & 0xFF;
		entry[index++] = (device.services[0].start >> 8) & 0xFF;
		entry[index++] = device.services[0].end & 0xFF;
		entry[index++] = (device.services[0].end >> 8) & 0xFF;
		entry[index++] = reports.length;
		reports.forEach(report => {
			entry[index++] = report.reportType;
			entry[index++] = report.characteristic.handle & 0xFF;
			entry[index++] = (report.characteristic.handle >> 8) & 0xFF;
		});
		Preference.set(Bonded.PREFERENCE_DOMAIN, Bonded.key(usageID), entry.buffer);
	}
	static remove(usageID) {
		const bonded = Bonded.get(usageID);
		if (undefined !== bonded)
			SM.deleteBonding(bonded.device.address, bonded.device.addressType);
		Preference.delete(Bonded.PREFERENCE_DOMAIN, Bonded.key(usageID));
	}
	static key(usageID) {
		switch(usageID) {
			case UsageID.POINTER:
				return Bonded.PREFERENCE_KEY_POINTER;
			case UsageID.MOUSE:
				return Bonded.PREFERENCE_KEY_MOUSE;
			case UsageID.JOYSTICK:
				return Bonded.PREFERENCE_KEY_JOYSTICK;
			case UsageID.GAMEPAD:
				return Bonded.PREFERENCE_KEY_GAMEPAD;
			case UsageID.KEYBOARD:
				return Bonded.PREFERENCE_KEY_KEYBOARD;
			case UsageID.KEYPAD:
				return Bonded.PREFERENCE_KEY_KEYPAD;
			default:
				throw new Error("unsupported usage id");
		}
	}
}
Bonded.PREFERENCE_DOMAIN = "hid";
Bonded.PREFERENCE_KEY_POINTER = "poin";
Bonded.PREFERENCE_KEY_MOUSE = "mous";
Bonded.PREFERENCE_KEY_JOYSTICK = "joys";
Bonded.PREFERENCE_KEY_GAMEPAD = "gamp";
Bonded.PREFERENCE_KEY_KEYBOARD = "keyb";
Bonded.PREFERENCE_KEY_KEYPAD = "keyp";
Object.freeze(Bonded);

export {BLEHIDClient as default, Bonded, BLEHIDClient, ReportType, UsageID};

