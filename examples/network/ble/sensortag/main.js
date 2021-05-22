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

import BLEClient from "bleclient";
import Timer from "timer";
import {uuid} from "btutils";

class SensorTagSensor {
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
				case "calibration":
					this.calibration = dictionary.calibration;
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
				this.configuration_data = 1;	// start measurements
			let characteristic = this.service.findCharacteristicByUUID(this.configuration);
			characteristic.writeWithoutResponse(this.configuration_data);
		}
		if (this.period) {
			let characteristic = this.service.findCharacteristicByUUID(this.period);
			if (characteristic) {
				if (!this.period_data)
					this.period_data = 100;	// 1s (10ms * 100) read interval
				characteristic.writeWithoutResponse(this.period_data);
			}
		}
	}
	start() {
		if (this.data) {
			let characteristic = this.service.findCharacteristicByUUID(this.data);
			characteristic.enableNotifications();
		}
	}
	onNotification(value) {
		debugger;
	}
	onValue(value) {
		debugger;
	}
}

class TemperatureSensor extends SensorTagSensor {
	onNotification(value) {
		let temperature = ((value[1] / 128.0) * 1.8 + 32).toFixed(1) + ' ˚F';
		trace(`[${this.name}] ${temperature}\n`);
	}
}

class AccelerometerSensor extends SensorTagSensor {
	onNotification(value) {
		let x = (value[0] / 64).toFixed(2) + ' g';
		let y = (value[1] / 64).toFixed(2) + ' g';
		let z = (value[2] / 64).toFixed(2) + ' g';
		trace(`[${this.name}] x: ${x} y: ${y} z: ${z}\n`);
	}
}

class HumiditySensor extends SensorTagSensor {
	onNotification(value) {
		let rawH = value[1] & ~0x0003;
		let humidity = (-6.0 + 125.0/65536 * rawH).toFixed(1) + ' %';
		trace(`[${this.name}] ${humidity}\n`);
	}
}

class MagnetometerSensor extends SensorTagSensor {
	configure(dictionary) {
		this.c = 2000 / 65536;
		super.configure(dictionary);
	}
	onNotification(value) {
		let c = this.c;
		let x = (value[0] * c).toFixed(2) + ' uT';
		let y = (value[1] * c).toFixed(2) + ' uT';
		let z = (value[2] * c).toFixed(2) + ' uT';
		trace(`[${this.name}] x: ${x} y: ${y} z: ${z}\n`);
	}
}

class BarometerSensor extends SensorTagSensor {
	configure(dictionary) {
		super.configure(dictionary);
		let characteristic = this.service.findCharacteristicByUUID(this.configuration);
		characteristic.writeWithoutResponse(this.configuration_data);
		characteristic = this.service.findCharacteristicByUUID(this.calibration);
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
	onValue(value) {
		this.calibration_data = value;

		// enable measurements
		let ch = this.service.findCharacteristicByUUID(this.configuration);
		ch.writeWithoutResponse(1);
		
		// enable notifications
		ch = this.service.findCharacteristicByUUID(this.data);
		ch.enableNotifications();
		Timer.delay(100);
	}
}

class GyroscopeSensor extends SensorTagSensor {
	configure(dictionary) {
		this.c = 500 / 65536;
		super.configure(dictionary);
	}
	onNotification(value) {
		let c = this.c;
		let x = (value[0] * c).toFixed(2) + ' ˚/s';
		let y = (value[1] * c).toFixed(2) + ' ˚/s';
		let z = (value[2] * c).toFixed(2) + ' ˚/s';
		trace(`[${this.name}] x: ${x} y: ${y} z: ${z}\n`);
	}
}

class KeysSensor extends SensorTagSensor {
	onNotification(value) {
		let right = (value & 1 ? 'pressed' : 'off');
		let left = (value & 2 ? 'pressed' : 'off');
		trace(`[${this.name}] left: ${left} right: ${right}\n`);
	}
}

const SERVICES = {
	"temperature": {
		uuid: uuid`F000AA00-0451-4000-B000-000000000000`,
	    constructor: TemperatureSensor,
	    data: uuid`F000AA01-0451-4000-B000-000000000000`,
	    configuration: uuid`F000AA02-0451-4000-B000-000000000000`,
	    period: uuid`F000AA03-0451-4000-B000-000000000000`,
	},
	"accelerometer": {
		uuid: uuid`F000AA10-0451-4000-B000-000000000000`,
	    constructor: AccelerometerSensor,
	    data: uuid`F000AA11-0451-4000-B000-000000000000`,
	    configuration: uuid`F000AA12-0451-4000-B000-000000000000`,
	    period: uuid`F000AA13-0451-4000-B000-000000000000`,
	},
	"humidity": {
		uuid: uuid`F000AA20-0451-4000-B000-000000000000`,
	    constructor: HumiditySensor,
	    data: uuid`F000AA21-0451-4000-B000-000000000000`,
	    configuration: uuid`F000AA22-0451-4000-B000-000000000000`,
	    period: uuid`F000AA23-0451-4000-B000-000000000000`,
	},
	"magnetometer": {
		uuid: uuid`F000AA30-0451-4000-B000-000000000000`,
	    constructor: MagnetometerSensor,
	    data: uuid`F000AA31-0451-4000-B000-000000000000`,
	    configuration: uuid`F000AA32-0451-4000-B000-000000000000`,
	    period: uuid`F000AA33-0451-4000-B000-000000000000`,
	},
	"barometer": {
		uuid: uuid`F000AA40-0451-4000-B000-000000000000`,
	    constructor: BarometerSensor,
	    data: uuid`F000AA41-0451-4000-B000-000000000000`,
	    configuration: uuid`F000AA42-0451-4000-B000-000000000000`,
		configuration_data: 0x02,
	    period: uuid`F000AA44-0451-4000-B000-000000000000`,
	    calibration: uuid`F000AA43-0451-4000-B000-000000000000`,
	},
	"gyroscope": {
		uuid: uuid`F000AA50-0451-4000-B000-000000000000`,
	    constructor: GyroscopeSensor,
	    data: uuid`F000AA51-0451-4000-B000-000000000000`,
	    configuration: uuid`F000AA52-0451-4000-B000-000000000000`,
		configuration_data: 0x07,	// Enable X, Y and Z
	    period: uuid`F000AA53-0451-4000-B000-000000000000`,
	},
	"keys": {
		uuid: uuid`FFE0`,
	    constructor: KeysSensor,
	    data: uuid`FFE1`,
	},
};

class SensorTag extends BLEClient {
	onReady() {
		this.startScanning();
	}
	onDiscovered(device) {
		if ("SensorTag" == device.scanResponse.completeName) {
			this.sensors = [];
			this.stopScanning();
			this.connect(device);
		}
	}
	onConnected(device) {
		trace(`discovering services\n`);
		device.discoverAllPrimaryServices();
	}
	onServices(services) {
		services.forEach(service => {
			for (let name in SERVICES) {
				if (service.uuid.equals(SERVICES[name].uuid)) {
					let sensor = Object.assign(SERVICES[name], {name}, {service});
					this.sensors.push(sensor);
				}
			}
		});
		trace(`found ${this.sensors.length} sensor services\n`);
		this.index = 0;
		this.sensors[0].service.discoverAllCharacteristics();
	}
	onCharacteristics(characteristics) {
		trace(`found ${characteristics.length} characteristics for sensor service ${this.sensors[this.index].name}\n`);
		if (++this.index == this.sensors.length) {
			this.index = 0;
			this.startMeasurements(this.index);
		}
		else
			this.sensors[this.index].service.discoverAllCharacteristics();
	}
	onCharacteristicValue(characteristic, value) {
		let sensors = this.sensors;
		for (let i = 0; i < sensors.length; ++i) {
			let sensor = sensors[i];
			if (sensor.service.findCharacteristicByUUID(characteristic.uuid)) {
				sensor.driver.onValue.call(sensor.driver, value);
				break;
			}
		}
	}
	onCharacteristicNotification(characteristic, value) {
		let sensors = this.sensors;
		let i = 0;
		for (; i < sensors.length; ++i) {
			let sensor = sensors[i];
			if (sensor.service.findCharacteristicByUUID(characteristic.uuid)) {
				sensor.driver.onNotification.call(sensor.driver, value);
				break;
			}
		}
		if (this.index < this.sensors.length - 1 && this.index == i) {
			Timer.delay(100);
			this.startMeasurements(++this.index);
		}
	}
	onDisconnected() {
		this.sensors.length = 0;
		this.startScanning();
	}
	startMeasurements(index) {
		let sensor = this.sensors[index];
		trace(`starting sensor [${sensor.name}]\n`);
		sensor.driver = new sensor.constructor();
		sensor.driver.configure(sensor);
		sensor.driver.start();
	}
}

let sensortag = new SensorTag;
