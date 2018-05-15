# BLE
Copyright 2017-18 Moddable Tech, Inc.

Revised: May 14, 2018

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## About This Document
This document describes the Moddable SDK Bluetooth Low Energy (BLE) modules. Both client (master) and server (slave) roles are supported on Espressif ESP32 and Silicon Labs Blue Gecko devices.

> **Note:** A BLE server/slave is also commonly referred to as a BLE peripheral. The terms *server*, *slave* and *peripheral* in this document are used interchangeably.

### Adding BLE to a project
The BLE client and server are implemented as separate modules to accomodate the limited memory and storage available on embedded devices. BLE applications instantiate a BLE client or BLE server, but never both.

To add the BLE client to a project, include its manifest:

	"include": [
		/* other includes here */
		"$(MODDABLE)/modules/network/ble/manifest_client.json",
	],
	
Similarly, to add the BLE server to a project, includes its manifest:

	"include": [
		/* other includes here */
		"$(MODDABLE)/modules/network/ble/manifest_server.json",
	],

### Using BLE

BLE applications typically subclass `BLEClient` or `BLEserver` and add device and/or application specific code. The following BLE client example subclasses `BLEClient` to scan for and discover peripherals:

	import BLEClient from "bleclient";

	class Scanner extends BLEClient {
		onReady() {
			this.startScanning();
		}
		onDiscovered(device) {
			let scanResponse = device.scanResponse;
			let completeName = scanResponse.completeName;
			if (completeName)
				trace(completeName + "\n");
		}
	}
	
	let scanner = new Scanner;

The following BLE server example subclasses `BLEServer` to advertise the device as a BLE Health Thermometer peripheral:

	import BLEServer from "bleserver";
	
	class HealthThermometerService extends BLEServer {
		onReady() {
			this.startAdvertising({
				advertisingData: {flags: 6, completeName: 	"Moddable HTM", completeUUID16List: ["1809","180F"]}
			});
		}
	}
	
	let htm = new HealthThermometerService;


## BLE Client
A BLE client can connect to one or more BLE peripherals. The maximum number of concurrent connections is defined at build time and ultimately limited by what the underlying embedded platform supports. To override the default maximum of two connections, override the `max_connections` define in the application manifest:

	"defines": {
		"ble": {
			"max_connections": 3
		}
	},

A BLE client typically performs the following steps to receive notifications from a BLE peripheral:

1. Start scanning for peripherals
2. Find the peripheral of interest by matching the advertised complete name in the scan response
3. Establish a connection with the peripheral
4. Discover the primary service(s) of interest
5. Discover the characteristic(s) within the service(s) of interest
6. Enable notifications for characteristics of interest

The following code from the colorific example app shows a typical client flow:

	const DEVICE_NAME = 'PowerMate Bluetooth';
	const SERVICE_UUID = '25598CF7-4240-40A6-9910-080F19F91EBC';
	const CHARACTERISTIC_UUID = '9CF53570-DDD9-47F3-BA63-09ACEFC60415';

	class PowerMate extends BLEClient {
		onReady() {
			this.startScanning();
		}
		onDiscovered(device) {
			if (DEVICE_NAME == 	device.scanResponse.completeName) {
				this.stopScanning();
				this.connect(device.address);
			}
		}
		onConnected(device) {
			device.discoverPrimaryService(SERVICE_UUID);
		}
		onServices(services) {
			let service = services.find(service => SERVICE_UUID == service.uuid);
			if (service)
				service.discoverCharacteristic(CHARACTERISTIC_UUID);
		}
		onCharacteristics(characteristics) {
			let characteristic = characteristics.find(characteristic => CHARACTERISTIC_UUID == characteristic.uuid);
			if (characteristic)
				characteristic.enableNotifications();
		}
		onCharacteristicNotification(characteristic, buffer) {
			let value = new Uint8Array(buffer)[0];
			trace(`value: ${value}\n`);
		}
	}

BLE is fundamentally asynchronous and results are always delivered to class callback methods, e.g. `onReady`, `onConnected`, etc... above. The following sections describe the BLE client classes, properties and callbacks.

## Class BLEClient

The `BLEClient` class provides access to the BLE client features.

	import BLEClient from "bleclient";
	
### onReady() callback
Applications must wait for the `onReady` callback before calling any `BLEClient` functions:

	class Client extends BLEClient {
		onReady() {
			/* stack is ready to use */
		}
	}
	let client = new BLEClient;
	
### startScanning([params])
The `startScanning` function enables scanning for nearby peripherals.

	class Scanner extends BLEClient {
		onReady() {
			this.startScanning();
		}
		onDiscovered(device) {
			trace("device discovered\n");
		}
	}

The `startScanning` function performs active scanning by default. The optional `params` can be used to override the scan parameters:

| Property | Type | Description |
| --- | --- | :--- |
| `active` | `boolean` | Set `true` for active scanning, `false` for passing scanning. Default is `true`.
| `interval` | `number` | Scan window value in units of 0.625 ms. Default is `0x50`. 
| `window` | `number` | Scanner window value in units of 0.625 ms. Default is `0x30`. 

To perform passing scanning at a 70 ms interval:

	this.startScanning({ active:false, interval:0x70 });
	
### onDiscovered(device)
The `onDiscovered` callback function is called one or more times for each peripheral device discovered. 

To connect to a device named "Brian" from the `onDiscovered` callback:

	onDiscovered(device) {
		if ("Brian" == 	device.scanResponse.completeName) {
			this.connect(device.address);
		}
	}
		
### stopScanning()
The `startScanning` function disables scanning for nearby peripherals.

### connect(address)
The `connect`function initiates a connection request between the `BLEClient` and target peripheral with the Bluetooth address `address`.

	onDiscovered(device) {
		this.connect(device.address);
	}
	onConnected(device) {
		trace(`Connected to device address ${device.address}\n`);
	}

### onConnected(device)
The `onConnected` callback function is called when the client connects to a peripheral.

> **Note:** The `BLEClient` object hosts all callbacks for classes used with `BLEClient`.

## Class Device
An instance of the `Device ` class is instantiated by `BLEClient` and provided to the host app in the `BLEClient` `onDiscovered` and `onConnected` callbacks. While applications never instantiate a `Device` class instance directly, applications do call `Device` class methods to perform GATT service/characteristic discovery and other functions.

The `Device` class includes the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `connection` | `number` | Connection identifier.
| `address` | `string` | Bluetooth device address.
| `scanResponse` | `object` | Instance of `Advertisement` class (described below) containing advertisement and scan response packet values.
| `discoverAllPrimaryServices` | `function` | A function to initiate full GATT primary service discovery.
| `discoverPrimaryService` | `function` | A function to initiate discovery of a single GATT primary service.

### discoverAllPrimaryServices()
Use the `discoverAllPrimaryServices()` function to discover all the peripheral's GATT primary services. Discovered services are returned to the `onServices` callback.

### discoverPrimaryService(uuid)
Use the `discoverPrimaryService()` function to discover a single GATT primary service by UUID.

### onServices(services)
The `onServices` callback function is called when service discovery completes. The `services` parameter contains an `Array` of services discovered.

To discover all primary services:

	onConnected(device) {
		device.discoverAllPrimaryServices();
	}
	onServices(services) {
		services.forEach( service => trace(`found service ${service.uuid}\n`);
	}

To discover a single primary service:	

	onConnected(device) {
		device.discoverPrimaryService('FF00');
	}
	onServices(services) {
		if (services.length)
			trace(`found service ${services[0].uuid}\n`);
	}
	
### close()
The `close()` function closes the connection to the peripheral. The `onDisconnected()` callback is called when the connection is closed.

	onConnected(device) {
		trace(`connected to device ${device.address}\n`);
		device.close();
	}
	onDisconnected() {
		trace("connection closed\n");
	}
	
## Class Service
The `Service` class provides access to a peripheral GATT service. The following properties are included:

| Property | Type | Description |
| --- | --- | :--- |
| `connection` | `number` | Connection identifier.
| `uuid` | `string` | Service UUID.
| `start` | `number` | Starting handle of included characteristics.
| `end` | `number` | Ending handle of included characteristics.
| `characteristics` | `array` | Array of service characteristics discovered.
| `discoverAllCharacteristics` | `function` | A function to discover all service characteristics.
| `discoverCharacteristic` | `function` | A function to discover a single service characteristic.
| `findCharacteristicByUUID` | `function` | A function to find a specific service characteristic by UUID.

### discoverAllCharacteristics()
Use the `discoverAllCharacteristics()` function to discover all the characteristics in a GATT service. Discovered characteristics are returned to the `onCharacteristics` callback.

### discoverCharacteristic(uuid)
Use the `discoverCharacteristic()` function to discover a single service characteristic by UUID.

### onCharacteristics(characteristics)
The `onCharacteristics` callback function is called when characteristic discovery completes. The `characteristics` parameter contains an `Array` of characteristics discovered.

### findCharacteristicByUUID(uuid)
The `findCharacteristicByUUID()` function finds and returns the characteristic identified by `uuid`. This function searches the `characteristics` property array.

To discover all the characteristics in the [Device Information](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.device_information.xml) service:

	onServices(services) {
		let service = services.find(service => "180A" == service.uuid);
		if (service)
			service.discoverAllCharacteristics();
	}
	onCharacteristics(characteristics) {
		characteristics.forEach( characteristic => trace(`found characteristic ${characteristic}\n`);
	}

To find the [Heart Rate Measurement](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.heart_rate_measurement.xml) characteristic in the [Heart Rate](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.heart_rate.xml) service:

	onConnected(device) {
		device.discoverPrimaryService('180D');
	}
	onServices(services) {
		this.hrs = services.find(service => SERVICE_UUID == service.uuid);
		if (this.hrs)
			this.hrs.discoverCharacteristic('2A37');
	}
	onCharacteristics(characteristics) {
		let characteristic = this.hrs.findCharacteristicByUUID('2A37');
		if (characteristic)
			trace("Found heart rate measurement characteristic\n");
	}

## Class Characteristic
The `Characteristic` class provides access to a GATT service characteristic. The following properties are included:

| Property | Type | Description |
| --- | --- | :--- |
| `connection` | `number` | Connection identifier.
| `uuid` | `string` | Characteristic UUID.
| `service` | `object` | `Service` object containing characteristic.
| `handle` | `number` | Chararacteristic handle.
| `descriptors` | `array` | Array of characteristic descriptors discovered.
| `discoverAllDescriptors` | `function` | A function to discover all characteristic descriptors.
| `disableNotifications` | `function` | A function to disable characteristic value change notifications.
| `enableNotifications` | `function` | A function to enable characteristic value change notifications.
| `readValue` | `function` | A function to read a characteristic value.
| `writeWithoutResponse` | `function` | A function to write a characteristic value without requiring a server response.

### discoverAllDescriptors()
Use the `discoverAllDescriptors()` function to discover all the descriptors in the characteristic. Discovered descriptps are returned to the `onDescriptors` callback.

### onDescriptors(descriptors)
The `onDescriptors()` callback function is called when descriptor discovery completes. The `descriptors` parameter contains an `Array` of characteristic descriptors discovered.

To discover the [Characteristic Presentation Format Descriptor](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.characteristic_presentation_format.xml) for a characteristic with UUID 0xFF00:

	onCharacteristics(characteristics) {
		let characteristic = characteristics.find(characteristic => 'FF00' == characteristic.uuid);
		if (characteristic)
			characteristic.discoverAllDescriptors();
	}
	onDescriptors(descriptors) {
		let descriptor = descriptors.find(descriptor => '2904' == descriptor);
		if (descriptor)
			trace("found characteristic presentation format descriptor\n");
	}

### enableNotifications()
Use the `enableNotifications()` function to enable characteristic value change notifications.

### disableNotifications()
Use the `disableNotifications()` function to disable characteristic value change notifications.

### onCharacteristicNotification(characteristic, value)
The `onCharacteristicNotification()` callback is called when notifications are enabled and the peripheral notifies the characteristic value. The `value` parameter is an `ArrayBuffer` containing the value.

To enable and receive characteristic value change notifications for the [Battery Level](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.battery_level.xml) characteristic in the [Battery Service](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.battery_service.xml):

	onConnected(device) {
		device.discoverPrimaryService('180F');
	}
	onServices(services) {
		let service = services.find(service => '180F' == service.uuid);
		if (service)
			service.discoverCharacteristic('2A19');
	}
	onCharacteristics(characteristics) {
		let characteristic = characteristics.find(characteristic => '2A19' == characteristic.uuid);
		if (characteristic)
			characteristic.enableNotifications();
	}
	onCharacteristicNotification(characteristic, value) {
		let level = new Uint8Array(value)[0];
		trace(`battery level: ${level}%\n`);
	}

### readValue()
Use the `readValue` function to read a characteristic value on demand.

### onCharacteristicValue(characteristic, value)
The `onCharacteristicValue` callback function is called when a characteristic is read by the `readValue()` function. The `value` parameter is an `ArrayBuffer` containing the value.

To read the [Device Name](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.gap.device_name.xml) from the [Generic Access Service](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.generic_access.xml):

	onConnected(device) {
		device.discoverPrimaryService('1800');
	}
	onServices(services) {
		let service = services.find(service => '1800' == service.uuid);
		if (service)
			service.discoverCharacteristic('2A00');
	}
	onCharacteristics(characteristics) {
		let characteristic = characteristics.find(characteristic => '2A00' == characteristic.uuid);
		if (characteristic)
			characteristic.readValue();
	}
	onCharacteristicValue(characteristic, value) {
		let name = String.fromArrayBuffer(value);
		trace(`device name: ${name}\n`);
	}

### writeWithoutResponse(value)
Use the `writeWithoutResponse()` function to write a characteristic value on demand. The `value` parameter is an `ArrayBuffer` containing the value to write.

To write the [URI](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.uri.xml) characteristic value in the [HTTP Proxy](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.http_proxy.xml) service:

	onConnected(device) {
		device.discoverPrimaryService('1823');
	}
	onServices(services) {
		let service = services.find(service => '1823' == service.uuid);
		if (service)
			service.discoverCharacteristic('2AB6');
	}
	onCharacteristics(characteristics) {
		let characteristic = characteristics.find(characteristic => '2AB6' == characteristic.uuid);
		if (characteristic)
			characteristic.writeWithoutResponse(ArrayBuffer.fromString("http://moddable.tech"));
	}

## Class Descriptor
The `Descriptor` class provides access to a GATT service characteristic descriptor. The following properties are included:

| Property | Type | Description |
| --- | --- | :--- |
| `connection` | `number` | Connection identifier.
| `uuid` | `string` | Descriptor UUID.
| `characteristic` | `object` | `Characteristic` object containing characteristic.
| `handle` | `number` | Descriptor handle.

### writeValue(value)
Use the `writeValue()` function to write a descriptor value. The `value` parameter is an `ArrayBuffer` containing the value to write.

To enable characteristic value change notifications by writing 0x0001 to the [Client Characteristic Configuration](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml) descriptor:

	onDescriptors(descriptors) {
		let descriptor = descriptors.find(descriptor => '2902' == descriptor);
		if (descriptor) {
			let value = Uint8Array.from(0x01, 0x00);	// little endian
			descriptor.writeValue(value.buffer);
		}
	}
	
> **Note:** The `Characteristic` `enableNotifications()` function is typically used for this purpose, though writing the CCCD descriptor has the same effect.
