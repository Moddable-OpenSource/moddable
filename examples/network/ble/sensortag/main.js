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
 	TI CC2541 SensorTag
	http://www.ti.com/tool/CC2541DK-SENSOR#technicaldocuments
 */

import BLE from "ble";
import {UUID} from "btutils";

const DEVICE_NAME = "SensorTag";

class SensorTagSensor {
	constructor() {}
	configure(dictionary) {
		for (let property in dictionary) {
			switch (property) {
				case "name":
					this.name = dictionary.name;
					break;
				case "service":
					this.service = dictionary.service;
					break;
				case "data":
					this.data = dictionary.data;
					break;
				case "configuration":
					this.configuration = dictionary.configuration;
					break;
				case "configuration_data":
					this.configuration_data = dictionary.configuration_data;
					break;
				case "period":
					this.period = dictionary.period;
					break;
				case "period_data":
					this.period_data = dictionary.period_data;
					break;
			}
		}
		this.initialize();
	}
	initialize() {
		if (this.configuration) {
			if (!this.configuration_data)
				this.configuration_data = [1];	// start measurements
			let characteristic = this.service.findCharacteristicByUUID(this.configuration);
			characteristic.writeWithoutResponse((new Uint8Array(this.configuration_data)).buffer);
		}
		if (this.period) {
			if (!this.period_data)
				this.period_data = [100];	// 1s (10ms * 100) read interval
			let characteristic = this.service.findCharacteristicByUUID(this.period);
			characteristic.writeWithoutResponse((new Uint8Array(this.period_data)).buffer);
		}
	}
	start() {
		if (this.data) {
			let characteristic = this.service.findCharacteristicByUUID(this.data);
			characteristic.onNotification = this.onNotification.bind(this);
			let descriptor = characteristic.findDescriptorByUUID(UUID.CCCD);
			descriptor.writeValue(1);
		}
	}
	stop() {
		if (this.data) {
			let characteristic = this.service.findCharacteristicByUUID(this.data);
			let descriptor = characteristic.findCharacteristicByUUID(UUID.CCCD);
			descriptor.writeValue(0);
		}
	}
	onNotification(buffer) {
		debugger;
	}
}

class TemperatureSensor extends SensorTagSensor {
	onNotification(buffer) {
		let view = new DataView(buffer);
		let value = view.getUint16(2, true);
		let temperature = ((value / 128.0) * 1.8 + 32).toFixed(1) + ' ˚F';
		trace(`[${this.name}] ${temperature}\n`);
	}
}

class AccelerometerSensor extends SensorTagSensor {
	onNotification(buffer) {
		let view = new DataView(buffer);
		let x = (view.getInt8(0) / 64).toFixed(2) + ' g';
		let y = (view.getInt8(1) / 64).toFixed(2) + ' g';
		let z = (view.getInt8(2) / 64).toFixed(2) + ' g';
		trace(`[${this.name}] x: ${x} y: ${y} z: ${z}\n`);
	}
}

class HumiditySensor extends SensorTagSensor {
	onNotification(buffer) {
		let view = new DataView(buffer);
		let rawH = view.getUint16(2, true) & ~0x0003;
		let humidity = (-6.0 + 125.0/65536 * rawH).toFixed(1) + ' %';
		trace(`[${this.name}] ${humidity}\n`);
	}
}

class MagnetometerSensor extends SensorTagSensor {
	configure(dictionary) {
		this.c = 2000 / 65536;
		super.configure(dictionary);
	}
	onNotification(buffer) {
		let view = new DataView(buffer);
		let c = this.c;
		let x = (view.getInt16(0, true) * c).toFixed(2) + ' uT';
		let y = (view.getInt16(2, true) * c).toFixed(2) + ' uT';
		let z = (view.getInt16(4, true) * c).toFixed(2) + ' uT';
		trace(`[${this.name}] x: ${x} y: ${y} z: ${z}\n`);
	}
}

class BarometerSensor extends SensorTagSensor {
	configure(dictionary) {
		super.configure(dictionary);
		let characteristic;
		characteristic = this.service.findCharacteristicByUUID(this.configuration);
		characteristic.writeWithoutResponse((new Uint8Array(this.configuration_data)).buffer);
		characteristic = this.service.findCharacteristicByUUID(dictionary.calibration);
		characteristic.onValue = buffer => {
			let view = new DataView(buffer);
			let calibration_data = this.calibration_data = new Array(8);
			for (let i = 0; i < 4; ++i)
				calibration_data[i] = view.getUint16(i * 2, true);
			for (let i = 4; i < 8; ++i)
				calibration_data[i] = view.getUint16(i * 2, true);
				
			// enable measurements
			let ch = this.service.findCharacteristicByUUID(this.configuration);
			let config = new Uint8Array(1);
			config[0] = 0x01;
			ch.writeWithoutResponse(config.buffer);
			
			// enable notifications
			ch = this.service.findCharacteristicByUUID(this.data);
			ch.onNotification = this.onNotification.bind(this);
			let descriptor = ch.findDescriptorByUUID(UUID.CCCD);
			descriptor.writeValue(1);
		}
		characteristic.readValue();
	}
	initialize() {
	}
	start() {
	}
	onNotification(buffer) {
		let view = new DataView(buffer);
		let t_r = view.getInt16(0, true);
		let p_r = view.getUint16(2, true);
		let c = this.calibration_data;
	
		let t_a = (100 * (c[0] * t_r / Math.pow(2,8) + c[1] * Math.pow(2,6))) / Math.pow(2,16);
		let S = c[2] + c[3] * t_r / Math.pow(2,17) + ((c[4] * t_r / Math.pow(2,15)) * t_r) / Math.pow(2,19);
		let O = c[5] * Math.pow(2,14) + c[6] * t_r / Math.pow(2,3) + ((c[7] * t_r / Math.pow(2,15)) * t_r) / Math.pow(2,4);
		let p_a = (S * p_r + O) / Math.pow(2,14);
		let pressure = (p_a / 100).toFixed(1) + ' hPa';
		trace(`[${this.name}] pressure: ${pressure}\n`);
	}
}

class GyroscopeSensor extends SensorTagSensor {
	configure(dictionary) {
		this.c = 500 / 65536;
		super.configure(dictionary);
	}
	onNotification(buffer) {
		let view = new DataView(buffer);
		let c = this.c;
		let x = (view.getInt16(0, true) * c).toFixed(2) + ' ˚/s';
		let y = (view.getInt16(2, true) * c).toFixed(2) + ' ˚/s';
		let z = (view.getInt16(4, true) * c).toFixed(2) + ' ˚/s';
		trace(`[${this.name}] x: ${x} y: ${y} z: ${z}\n`);
	}
}

class KeysSensor extends SensorTagSensor {
	onNotification(buffer) {
		let value = new Uint8Array(buffer)[0];
		let right = (value & 1 ? 'pressed' : 'off');
		let left = (value & 2 ? 'pressed' : 'off');
		trace(`[${this.name}] left: ${left} right: ${right}\n`);
	}
}

const SERVICES = {
	["F000AA00-0451-4000-B000-000000000000"]: {
		name: "temperature",
	    constructor: TemperatureSensor,
	    data: "F000AA01-0451-4000-B000-000000000000",
	    configuration: "F000AA02-0451-4000-B000-000000000000",
	    period: "F000AA03-0451-4000-B000-000000000000",
	},
	["F000AA10-0451-4000-B000-000000000000"]: {
		name: "accelerometer",
	    constructor: AccelerometerSensor,
	    data: "F000AA11-0451-4000-B000-000000000000",
	    configuration: "F000AA12-0451-4000-B000-000000000000",
	    period: "F000AA13-0451-4000-B000-000000000000",
	},
	["F000AA20-0451-4000-B000-000000000000"]: {
		name: "humidity",
	    constructor: HumiditySensor,
	    data: "F000AA21-0451-4000-B000-000000000000",
	    configuration: "F000AA22-0451-4000-B000-000000000000",
	    period: "F000AA23-0451-4000-B000-000000000000",
	},
	["F000AA30-0451-4000-B000-000000000000"]: {
		name: "magnetometer",
	    constructor: MagnetometerSensor,
	    data: "F000AA31-0451-4000-B000-000000000000",
	    configuration: "F000AA32-0451-4000-B000-000000000000",
	    period: "F000AA33-0451-4000-B000-000000000000",
	},
	["F000AA40-0451-4000-B000-000000000000"]: {
		name: "barometer",
	    constructor: BarometerSensor,
	    data: "F000AA41-0451-4000-B000-000000000000",
	    configuration: "F000AA42-0451-4000-B000-000000000000",
		configuration_data: [0x02],
	    period: "F000AA44-0451-4000-B000-000000000000",
	    calibration: "F000AA43-0451-4000-B000-000000000000",
	},
	["F000AA50-0451-4000-B000-000000000000"]: {
		name: "gyroscope",
	    constructor: GyroscopeSensor,
	    data: "F000AA51-0451-4000-B000-000000000000",
	    configuration: "F000AA52-0451-4000-B000-000000000000",
		configuration_data: [0x07],	// Enable X, Y and Z
	    period: "F000AA53-0451-4000-B000-000000000000",
	},
	["FFE0"]: {
		name: "keys",
	    constructor: KeysSensor,
	    data: "FFE1",
	},
};
let serviceCount = Object.keys(SERVICES).length;

let ble = new BLE();
ble.onReady = () => {
	ble.onDiscovered = device => {
		if (DEVICE_NAME == device.scanResponse.completeName) {
			ble.sensors = [];
			ble.remaining = serviceCount;
			ble.stopScanning();
			ble.connect(device.address);
		}
	}
	ble.onConnected = connection => {
		connection.onDisconnected = () => {
			ble.sensors.length = 0;
			ble.remaining = serviceCount;
			ble.startScanning();
		}
		let client = connection.client;
		client.onServices = services => {
			services.forEach(service => {
				if (service.uuid in SERVICES) {
					let sensor = Object.assign({service}, SERVICES[service.uuid]);
					ble.sensors.push(sensor);
				}
			});
			ble.sensors.forEach(sensor => {
				--ble.remaining;
				sensor.service.onCharacteristics = characteristics => {
					characteristics.forEach(characteristic => {
						characteristic.onDescriptors = descriptors => {
							if (0 == ble.remaining) {
								ble.sensors.forEach(sensor => {
									sensor.driver = new sensor.constructor();
									sensor.driver.configure(sensor);
									sensor.driver.start();
								});
							}
						}
						characteristic.discoverAllDescriptors();
					})
				}
				sensor.service.discoverAllCharacteristics();
			});
		}
		client.discoverAllPrimaryServices();
	}
	ble.startScanning();
}
	
ble.initialize();

