/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

import { GAPClient, GATTClient } from "embedded:io/bluetoothle/central";
import config from "mc/config";

// https://github.com/WebBluetoothCG/registries/blob/master/gatt_assigned_services.txt

const GATT_SERVICES = new Map([
	["alert_notification", 0x1811],
	["automation_io", 0x1815],
	["battery_service", 0x180F],
	["blood_pressure", 0x1810],
	["body_composition", 0x181B],
	["bond_management", 0x181E],
	["continuous_glucose_monitoring", 0x181F],
	["current_time", 0x1805],
	["cycling_power", 0x1818],
	["cycling_speed_and_cadence", 0x1816],
	["device_information", 0x180A],
	["environmental_sensing", 0x181A],
	["generic_access", 0x1800],
	["generic_attribute", 0x1801],
	["glucose", 0x1808],
	["health_thermometer", 0x1809],
//	["heart_rate", 0x180D],
	["human_interface_device", 0x1812],
	["immediate_alert", 0x1802],
	["indoor_positioning", 0x1821],
	["internet_protocol_support", 0x1820],
	["link_loss", 0x1803],
	["location_and_navigation", 0x1819],
	["next_dst_change", 0x1807],
	["object_transfer", 0x1825],
	["phone_alert_status", 0x180E],
	["pulse_oximeter", 0x1822],
	["reference_time_update", 0x1806],
	["running_speed_and_cadence", 0x1814],
	["scan_parameters", 0x1813],
	["transport_discovery", 0x1824],
	["tx_power", 0x1804],
	["user_data", 0x181C],
	["weight_scale", 0x181D],
]);

const GATT_CHARACTERISTICS = new Map([
	["aerobic_heart_rate_lower_limit", 0x2A7E],
	["aerobic_heart_rate_upper_limit", 0x2A84],
	["age", 0x2A80],
	["battery_level", 0x2A19],
	["blood_pressure_feature", 0x2A49],
	["blood_pressure_measurement", 0x2A35],
	["body_sensor_location", 0x2A38],
	["csc_feature", 0x2A5C],
	["csc_measurement", 0x2A5B],
	["date_of_birth", 0x2A85],
	["date_of_threshold_assessment", 0x2A86],
	["device_name", 0x2A00],
	["firmware_revision_string", 0x2A26],
	["glucose_feature", 0x2A51],
	["glucose_measurement", 0x2A18],
	["hardware_revision_string", 0x2A27],
	["heart_rate_control_point", 0x2A39],
//	["heart_rate_measurement", 0x2A37],
	["height", 0x2A8E],
	["hid_control_point", 0x2A4C],
	["hid_information", 0x2A4A],
	["ieee_11073_20601_regulatory_certification_data_list", 0x2A2A],
	["intermediate_blood_pressure", 0x2A36],
	["intermediate_temperature", 0x2A1E],
	["manufacturer_name_string", 0x2A29],
	["model_number_string", 0x2A24],
	["preferred_connection_parameters", 0x2A04],
	["protocol_mode", 0x2A4E],
	["report", 0x2A4D],
	["report_map", 0x2A4B],
	["resting_heart_rate", 0x2A92],
	["serial_number_string", 0x2A25],
	["software_revision_string", 0x2A28],
	["step_count", 0x2AC3],
	["supported_new_alert_category", 0x2A47],
	["supported_unread_alert_category", 0x2A48],
	["system_id", 0x2A23],
	["temperature_measurement", 0x2A1C],
	["temperature_type", 0x2A1D],
	["user_control_point", 0x2A9F],
	["weight_measurement", 0x2A9D],
]);

const GATT_DESCRIPTORS = new Map([
	["gatt.characteristic_extended_properties", 0x2900],
	["gatt.characteristic_user_description", 0x2901],
	["gatt.client_characteristic_configuration", 0x2902],
	["gatt.server_characteristic_configuration", 0x2903],
	["gatt.characteristic_presentation_format", 0x2904],
	["gatt.characteristic_aggregate_format", 0x2905],
	["valid_range", 0x2906],
	["external_report_reference", 0x2907],
	["report_reference", 0x2908],
	["number_of_digitals", 0x2909],
	["value_trigger_setting", 0x290A],
	["es_configuration", 0x290B],
	["es_measurement", 0x290C],
	["es_trigger_setting", 0x290D],
	["time_trigger_setting", 0x290E],
]);

if (config.webbluetooth) {
	config.webbluetooth.services?.forEach(item => GATT_SERVICES.set(item[0], item[1]));
	config.webbluetooth.characteristics?.forEach(item => GATT_CHARACTERISTICS.set(item[0], item[1]));
	config.webbluetooth.descriptors?.forEach(item => GATT_DESCRIPTORS.set(item[0], item[1]));
}

// TypeError: Invalid Service name: '123'. It must be a valid UUID alias (e.g. 0x1234), UUID (lowercase hex characters e.g. '00001234-0000-1000-8000-00805f9b34fb'), or recognized standard name from https://www.bluetooth.com/specifications/gatt/services e.g. 'alert_notification'.
function map(id, map) {
	if (typeof id !== "number") {
		let i = map.get(id);
		if (undefined === i) {
			// does it look like a UUID....		//@@ cheap hack
			if (36 !== id.length)
				throw new TypeError;
			return id;
		}
		id = i;
	}
	return BluetoothUUID.canonicalUUID(id);
}

class BluetoothUUID {
	static canonicalUUID(id) {
		return (id & 0xffffffff).toString(16).padStart(8, "0") + "-0000-1000-8000-00805f9b34fb"; 
	}
	static getCharacteristic(id) {
		return map(id, GATT_CHARACTERISTICS);
	}
	static getDescriptor(id) {
		return map(id, GATT_DESCRIPTORS);
	}
	static getService(id) {
		return map(id, GATT_SERVICES);
	}
}

const bluetooth = Object.freeze({
	async requestDevice(options = {}) {
		let names = [];
		let services = [];
		options.filters?.forEach(filter => {
			if (filter.name)
				names.push(filter.name);
			if (filter.services)
				services = services.concat(filter.services.map(id => BluetoothUUID.getService(id)));
		});
		const target = Promise.withResolvers();
		const scanner = new GAPClient({
			services,
			onReadable() {
				const advertisement = this.read();
				if (names.length && (names.indexOf(advertisement.name) < 0))
					return;
				this.close();
				const device = new BluetoothDevice(advertisement.address, advertisement.name);
				target.resolve(device);
			},
			onError() {
				target.reject("not found");
			}
		});
		return target.promise;
	}
});

class BluetoothDevice {
	#address;
	#bleClient;
	#name;
	#notifications;
	#server;
	constructor(address, name) {
		this.#address = address;
		this.#name = name ?? address; 
		this.#notifications = new Map; 
	}
	get gatt() {
		this.#server ??= new BluetoothDevice.#BluetoothRemoteGATTServer(this);
		return this.#server;
	}
	get id() {
		return this.#address;
	}
	get name() {
		return this.#name;
	}

	static #BluetoothRemoteGATTServer = class {
		#device;
		#connected = false;

		constructor(device) {
			this.#device = device;
		}
		connect() {
			if (this.#connected)
				return Promise.resolve(this);

			const target = Promise.withResolvers();
			const device = this.device;
			device.#bleClient ??= new GATTClient({ 
				address: device.#address,
				onError: (error) => {
					this.#connected = false;
					device.#bleClient.waiting?.forEach(target => target.reject(error));
					delete device.#bleClient.waiting;
				},
				onReadable: (count) => {
					while (count--) { 
						const value = device.#bleClient.read();
						const callback = device.#notifications.get(value.handle);
						if (callback)
							callback.call(null, value);
					}
				},
				onReady: () => {
					this.#connected = true;
					device.#bleClient.waiting.forEach(target => target.resolve(this));
					delete device.#bleClient.waiting;
				},
			});
			device.#bleClient.waiting ??= [];
			device.#bleClient.waiting.push(target);
			return target.promise;
		}
		disconnect() {
			const device = this.device;
			if (device.#bleClient) {
				device.#bleClient.close();
				device.#bleClient = undefined;
			}
		}
		getPrimaryService(id) {
			id = BluetoothUUID.getService(id);
			const target = Promise.withResolvers();
			const device = this.device;
			device.#bleClient.getPrimaryServices([ id ], (error, results) => {
				if (!error && !results.length)
					error = new Error("not found");

				if (error)
					target.reject(error);
				else
					target.resolve(new BluetoothDevice.#BluetoothRemoteGATTService(device, results[0]));
			});
			return target.promise;
		}
		getPrimaryServices(ids) {
			ids = ids?.map(id => BluetoothUUID.getService(id));
			const target = Promise.withResolvers();
			const device = this.device;
			device.#bleClient.getPrimaryServices(ids, (error, results) => {
				if (!error && !results.length)
					error = new Error("not found");

				if (error)
					target.reject(error);
				else
					target.resolve(results.map(result => new BluetoothDevice.#BluetoothRemoteGATTService(device, result)));
			});
			return target.promise;
		}
		get connected()  {
			return this.#connected;
		}
		get device() {
			return this.#device;
		}
	}

	static #BluetoothRemoteGATTService = class {
		#device;
		#bleService;
		constructor(device, bleService) {
			this.#device = device;
			this.#bleService = bleService;
		}
		getCharacteristic(id) {
			id = BluetoothUUID.getCharacteristic(id);
			const target = Promise.withResolvers();
			this.device.#bleClient.getCharacteristics(this.#bleService, [ id ], (error, results) => {
				if (!error && !results.length)
					error = new Error("not found");

				if (error)
					target.reject(error);
				else
					target.resolve(new BluetoothDevice.#BluetoothRemoteGATTCharacteristic(this, results[0]));
			});
			return target.promise;
		}
		getCharacteristics(ids) {
			ids = ids?.map(id => BluetoothUUID.getCharacteristic(id));
			const target = Promise.withResolvers();
			this.device.#bleClient.getCharacteristics(this.#bleService, ids, (error, results) => {
				if (!error && !results.length)
					error = new Error("not found");

				if (error)
					target.reject(error);
				else
					target.resolve(results.map(result => new BluetoothDevice.#BluetoothRemoteGATTCharacteristic(this, result)));
			});
			return target.promise;
		}
		get uuid() {
			return this.#bleService.uuid;
		}
		get isPrimary() {
			return true;
		}
		get device() {
			return this.#device;
		}
	}

	static #BluetoothRemoteGATTCharacteristic = class {
		#service;
		#bleCharacteristic;
		#listeners;
		#value;
		constructor(service, bleCharacteristic) {
			this.#service = service;
			this.#bleCharacteristic = bleCharacteristic;
			this.#listeners = [];
		}
		addEventListener(event, callback) {
			if (event != "characteristicvaluechanged")
				throw new Error("no such event");
			this.#listeners.push(callback);
		}
		getDescriptor(id) {
			id = BluetoothUUID.getDescriptor(id);
			const target = Promise.withResolvers();
			this.service.device.#bleClient.getDescriptors(this.#bleCharacteristic, [ id ], (error, results) => {
				if (!error && !results.length)
					error = new Error("not found");

				if (error)
					target.reject(error);
				else
					target.resolve(new BluetoothDevice.#BluetoothRemoteGATTDescriptor(this, results[0]));
			});
			return target.promise;
		}
		getDescriptors(ids) {
			ids = ids?.map(id => BluetoothUUID.getDescriptor(id));
			const target = Promise.withResolvers();
			this.service.device.#bleClient.getDescriptors(this.#bleCharacteristic, ids, (error, results) => {
				if (!error && !results.length)
					error = new Error("not found");

				if (error)
					target.reject(error);
				else
					target.resolve(results.map(result => new BluetoothDevice.#BluetoothRemoteGATTDescriptor(this, result)));
			});
			return target.promise;
		}
		readValue() {
			const target = Promise.withResolvers();
			this.service.device.#bleClient.read(this.#bleCharacteristic, (error, result) => {
				if (error)
					target.reject(error);
				else {
					this.#value = new DataView(result);
					target.resolve(this.#value);
				}
			});
			return target.promise;
		}
		removeEventListener(event, listener) {
			if (event != "characteristicvaluechanged")
				throw new Error("no such event");
			const listeners = this.#listeners;
			const index = listeners.indexOf(listener);
			if (index >= 0)
				listeners.splice(index, 1);
		}
		startNotifications() {
			const target = Promise.withResolvers();
			this.service.device.#bleClient.subscribe(this.#bleCharacteristic, (error, result) => {
				if (error)
					target.reject(error);
				else {
					this.service.device.#notifications.set(this.#bleCharacteristic.handle, value => {
						this.#value = new DataView(value);
						const event = {
							target: this
						}
						this.#listeners.forEach(listener => listener.call(null, event));
					});
					target.resolve(result);
				}
			});
			return target.promise;
		}
		stopNotifications() {
			const target = Promise.withResolvers();
			this.service.device.#bleClient.unsubscribe(this.#bleCharacteristic, (error, result) => {
				if (error)
					target.reject(error);
				else {
					this.service.device.#notifications.delete(this.#bleCharacteristic.handle);
					target.resolve(result);
				}
			});
			return target.promise;
		}

		writeValue(value) {
			return this.writeValueWithoutResponse(value);
		}
		writeValueWithResponse(value) {
			const target = Promise.withResolvers();
			this.service.device.#bleClient.write(this.#bleCharacteristic, value, { response:true }, (error, result) => {
				if (error)
					target.reject(error);
				else
					target.resolve(result);
			});
			return target.promise;
		}
		writeValueWithoutResponse(value) {
			const target = Promise.withResolvers();
			this.service.device.#bleClient.write(this.#bleCharacteristic, value, { response:false }, (error, result) => {
				if (error)
					target.reject(error);
				else
					target.resolve(result);
			});
			return target.promise;
		}

		get properties() {
			return new BluetoothDevice.#BluetoothCharacteristicProperties(this.#bleCharacteristic.properties);
		}
		get service() {
			return this.#service;
		}
		get uuid() {
			return this.#bleCharacteristic.uuid;
		}
		get value() {
			return this.#value;
		}
		set value(it) {
			// nop
		}
	}

	static #BluetoothRemoteGATTDescriptor = class {
		#characteristic;
		#bleDescriptor;
		#value;
		constructor(characteristic, bleDescriptor) {
			this.#characteristic = characteristic;
			this.#bleDescriptor = bleDescriptor;
		}
		readValue() {
			const target = Promise.withResolvers();
			this.characteristic.service.device.#bleClient.read(this.#bleDescriptor, (error, result) => {
				if (error)
					target.reject(error);
				else {
					this.#value = new DataView(result);
					target.resolve(this.#value);
				}
			});
			return target.promise;
		}
		writeValue(value) {
			const target = Promise.withResolvers();
			this.characteristic.service.device.#bleClient.write(this.#bleDescriptor, value, (error, result) => {
				if (error)
					target.reject(error);
				else
					target.resolve(result);
			});
			return target.promise;
		}
		get characteristic() {
			return this.#characteristic;
		}
		get uuid() {
			return this.#bleDescriptor.uuid;
		}
		get value() {
			return this.#value;
		}
		set value(it) {
			// nop
		}
	}

	static #BluetoothCharacteristicProperties = class {
		#flags;
		constructor(flags) {
			this.#flags = flags;
		}
		get authenticatedSignedWrites() {
			return (this.#flags & (1 << 6)) != 0;
		}
		get broadcast() {
			return (this.#flags & (1 << 0)) != 0;
		}
		get indicate() {
			return (this.#flags & (1 << 5)) != 0;
		}
		get notify() {
			return (this.#flags & (1 << 4)) != 0;
		}
		get read() {
			return (this.#flags & (1 << 1)) != 0;
		}
		get reliableWrite() {
			return (this.#flags & (1 << 7)) != 0;
		}
		get write() {
			return (this.#flags & (1 << 3)) != 0;
		}
		get writeWithoutResponse() {
			return (this.#flags & (1 << 2)) != 0;
		}
	}
}

export { BluetoothUUID,  bluetooth, BluetoothDevice }
