# BLE
Copyright 2017-18 Moddable Tech, Inc.

Revised: May 16, 2018

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## About This Document
This document describes the Moddable SDK Bluetooth Low Energy (BLE) modules. Both client (master) and server (slave) roles are supported on Espressif ESP32 and Silicon Labs Blue Gecko devices.

> **Note:** A BLE server/slave is also commonly referred to as a BLE peripheral. The terms *server*, *slave* and *peripheral* in this document are used interchangeably.

## Table of Contents
* [Adding BLE to a Project](#addingble)
* [Using BLE](#usingble)
* [BLE Client](#bleclient)
	* [Class BLEClient](#classbleclient)
		* [Class Device](#classdevice)
		* [Class Service](#classservice)
		* [Class Characteristic](#classcharacteristic)
		* [Class Descriptor](#classdescriptor)
		* [Class Advertisement](#classadvertisement)
* [BLE Server](#bleserver)
	* [Class BLEServer](#classbleserver)
	* [GATT Services](#gattservices)
* [BLE Apps on ESP32 Platform](#esp32platform)
* [BLE Apps on Blue Gecko Platform](#geckoplatform)
* [BLE Example Apps](#exampleapps)

<a id="addingble"></a>
### Adding BLE to a Project
The BLE client and server are implemented as separate modules to accomodate the limited memory and storage available on embedded devices. BLE applications instantiate a BLE client or BLE server, but never both.

To add the BLE client to a project, include its manifest:

	"include": [
		/* other includes here */
		"$(MODDABLE)/modules/network/ble/manifest_client.json",
	],
	
Similarly, to add the BLE server to a project, include its manifest:

	"include": [
		/* other includes here */
		"$(MODDABLE)/modules/network/ble/manifest_server.json",
	],

<a id="usingble"></a>
### Using BLE

BLE applications typically subclass `BLEClient` or `BLEServer` and then add device and/or application specific code. The following BLE client example subclasses `BLEClient` to scan for and discover peripherals:

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


<a id="bleclient"></a>
## BLE Client
A BLE client can connect to one or more BLE peripherals. The maximum number of concurrent connections is defined at build time and ultimately limited by what the underlying embedded platform supports. To override the default maximum of two connections, override the `max_connections` value in the manifest `defines` section:

	"defines": {
		"ble": {
			"max_connections": 3
		}
	},

A BLE client typically performs the following steps to receive notifications from a BLE peripheral:

1. Start scanning for peripherals
2. Find the peripheral of interest by matching the advertised complete name or UUID(s) in the scan response
3. Establish a connection with the peripheral
4. Discover the primary service(s) of interest
5. Discover the characteristic(s) within the service(s) of interest
6. Enable notifications for characteristic(s) of interest

The following code from the [colorific](../../../examples/network/ble/colorific) example app shows a typical client flow:

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

A BLE client is fundamentally asynchronous and results are always delivered to the `BLEClient` class callback methods, e.g. `onReady`, `onConnected`, etc... above. The following sections describe the BLE client classes, properties and callbacks.

<a id="classbleclient"></a>
## Class BLEClient

The `BLEClient` class provides access to the BLE client features.

	import BLEClient from "bleclient";
	
### onReady()
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

The `startScanning` function performs active scanning by default. The optional `params` includes properties that can be set to override the default scan behavior:

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
The `stopScanning` function disables scanning for nearby peripherals.

### connect(address)
The `connect` function initiates a connection request between the `BLEClient` and target peripheral `device` with the Bluetooth address `address`.

	onDiscovered(device) {
		this.connect(device.address);
	}
	onConnected(device) {
		trace(`Connected to device address ${device.address}\n`);
	}

### onConnected(device)
The `onConnected` callback function is called when the client connects to a peripheral.

> **Note:** The `BLEClient` object hosts all callbacks for classes used with `BLEClient`.

<a id="classdevice"></a>
## Class Device
An instance of the `Device` class is instantiated by `BLEClient` and provided to the host app in the `BLEClient` `onDiscovered` and `onConnected` callbacks. While applications never instantiate a `Device` class instance directly, applications do call `Device` class functions to perform GATT service/characteristic discovery.

The `Device` class includes the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `connection` | `number` | Connection identifier.
| `address` | `string` | Bluetooth device address.
| `scanResponse` | `object` | Instance of [Advertisement](#classadvertisement) class containing advertisement and scan response packet values.
| `discoverAllPrimaryServices` | `function` | A function to initiate full GATT primary service discovery.
| `discoverPrimaryService` | `function` | A function to initiate discovery of a single GATT primary service.

### discoverAllPrimaryServices()
Use the `discoverAllPrimaryServices` function to discover all the peripheral's GATT primary services. Discovered services are returned to the `onServices` callback.

### discoverPrimaryService(uuid)
Use the `discoverPrimaryService` function to discover a single GATT primary service by UUID.

### onServices(services)
The `onServices` callback function is called when service discovery completes. The `services` parameter contains an `Array` of services discovered. If no services are discovered, an empty `services` array is returned.

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
The `close` function terminates the peripheral connection. The `onDisconnected` callback is called when the connection is closed.

	onConnected(device) {
		trace(`connected to device ${device.address}\n`);
		device.close();
	}
	onDisconnected() {
		trace("connection closed\n");
	}
	
<a id="classservice"></a>
## Class Service
The `Service` class provides access to a single peripheral GATT service. The following properties are included:

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
Use the `discoverAllCharacteristics()` function to discover all the service characteristics. Discovered characteristics are returned to the `onCharacteristics` callback.

### discoverCharacteristic(uuid)
Use the `discoverCharacteristic` function to discover a single service characteristic by UUID.

### onCharacteristics(characteristics)
The `onCharacteristics` callback function is called when characteristic discovery completes. The `characteristics` parameter contains an `Array` of characteristics discovered. If no characteristics are discovered, an empty `characteristics` array is returned.

### findCharacteristicByUUID(uuid)
The `findCharacteristicByUUID` function finds and returns the characteristic identified by `uuid`. This function searches the `characteristics` property array.

To discover all the characteristics in the [Device Information](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.device_information.xml) service:

	onServices(services) {
		let service = services.find(service => "180A" == service.uuid);
		if (service)
			service.discoverAllCharacteristics();
	}
	onCharacteristics(characteristics) {
		characteristics.forEach(characteristic => trace(`found characteristic ${characteristic}\n`));
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

<a id="classcharacteristic"></a>
## Class Characteristic
The `Characteristic` class provides access to a single service characteristic. The following properties are included:

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
Use the `discoverAllDescriptors` function to discover all the characteristic's descriptors. Discovered descriptors are returned to the `onDescriptors` callback.

### onDescriptors(descriptors)
The `onDescriptors` callback function is called when descriptor discovery completes. The `descriptors` parameter contains an `Array` of characteristic descriptors discovered. If no descriptors are discovered, an empty `descriptors` array is returned.


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
Use the `enableNotifications` function to enable characteristic value change notifications.

### disableNotifications()
Use the `disableNotifications` function to disable characteristic value change notifications.

### onCharacteristicNotification(characteristic, value)
The `onCharacteristicNotification` callback function is called when notifications are enabled and the peripheral notifies the characteristic value. The `value` parameter is an `ArrayBuffer` containing the characteristic value.

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
The `onCharacteristicValue` callback function is called when a characteristic is read by the `readValue` function. The `value` parameter is an `ArrayBuffer` containing the characteristic value.

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
Use the `writeWithoutResponse` function to write a characteristic value on demand. The `value` parameter is an `ArrayBuffer` containing the characteristic value to write.

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

<a id="classdescriptor"></a>
## Class Descriptor
The `Descriptor` class provides access to a single characteristic descriptor. The following properties are included:

| Property | Type | Description |
| --- | --- | :--- |
| `connection` | `number` | Connection identifier.
| `uuid` | `string` | Descriptor UUID.
| `characteristic` | `object` | `Characteristic` object containing descriptor.
| `handle` | `number` | Descriptor handle.

### writeValue(value)
Use the `writeValue` function to write a descriptor value. The `value` parameter is an `ArrayBuffer` containing the descriptor value to write.

To enable characteristic value change notifications by writing 0x0001 to the [Client Characteristic Configuration](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml) descriptor:

	onDescriptors(descriptors) {
		let descriptor = descriptors.find(descriptor => '2902' == descriptor);
		if (descriptor) {
			let value = Uint8Array.from(0x01, 0x00);	// little endian
			descriptor.writeValue(value.buffer);
		}
	}
	
> **Note:** The `Characteristic` `enableNotifications` convenience function is typically used for this purpose, though writing the CCCD descriptor directly provides the same net result.

<a id="classadvertisement"></a>
## Class Advertisement
The `Advertisement` class provides accessor functions to read common advertisement and scan response data types as JavaScript properties. The class is primarily used to parse the `scanResponseData` provided to BLE clients via the `onDiscovered` callback function's `device` parameter.

### completeName
The advertised complete local name.

### shortName
The advertised shortened local name.

### manufacturerSpecific
An object containing the advertised manufacturer specific data.

### flags
The advertised flags value.

To identify an advertised device with the complete name "Zip":

	onDiscovered(device) {
		let completeName = device.scanResponse.completeName;
		if (completeName == "ZIP")
			trace(`Found ZIP device with address ${device.address}\n`);
		}
	}

To identify a [Blue Maestro Environment Monitor](https://www.bluemaestro.com/product/tempo-environment-monitor/) and read the temperature from the manufacturer specific data:

	onDiscovered(device) {
		let manufacturerSpecific = device.scanResponse.manufacturerSpecific;
		const TempoManufacturerID = 307; 
		
		// If this is a Tempo device...
		if (manufacturerSpecific && (TempoManufacturerID == manufacturerSpecific.identifier)) {
			let data = manufacturerSpecific.data;
			if (data[0] == 0 || data[0] == 1) {	// ...and product model T30 or THP
				let temperature = (data[3] | (data[4] << 8)) / 10;
				trace(`Temperature: ${temperature} ˚C\n`);
			}
		}
	}

<a id="bleserver"></a>
## BLE Server
A BLE server/peripheral can connect to one BLE client and typically performs the following steps to send notifications to a BLE client:

1. Deploy services
2. Start advertising so that clients can discover the peripheral
3. Establish a connection with a client
4. Accept characteristic value change notification request(s)
5. Notify characteristic value changes.

The following abbreviated code from the [heart-rate-server](../../../examples/network/ble/heart-rate-server) example app shows a typical server flow:

	onReady() {
		this.bpm = [0, 60]; // flags, beats per minute
		this.deploy();
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: "Moddable HRS", completeUUID16List: ["180D","180F"]}
		});
	}
	onConnected() {
		this.stopAdvertising();
	}
	onCharacteristicNotifyEnabled(characteristic) {
		this.startMeasurements(characteristic);
	}
	startMeasurements(characteristic) {
		this.bump = +1;
		this.timer = Timer.repeat(id => {
			this.notifyValue(characteristic, this.bpm);
			this.bpm[1] += this.bump;
			if (this.bpm[1] == 65) {
				this.bump = -1;
				this.bpm[1] == 64;
			}
			else if (this.bpm[1] == 55) {
				this.bump = +1;
				this.bpm[1] == 56;
			}
		}, 1000);
	}

A BLE server is fundamentally asynchronous and results are always delivered to the `BLEServer` class callback methods, e.g. `onReady`, `onConnected`, etc... above. The following sections describe the BLE server class, properties and callbacks.

<a id="classbleserver"></a>
## Class BLEServer

The `BLEServer` class provides access to the BLE server features.

	import BLEServer from "bleserver";
	
### onReady()
Applications must wait for the `onReady` callback before calling any `BLEServer` functions:

	class Server extends BLEServer {
		onReady() {
			/* stack is ready to use */
		}
	}
	let server = new BLEServer;

### deviceName
The `deviceName` property accessor function is used to set/get the Bluetooth peripheral device name.

	class Advertiser extends BLEServer {
		onReady() {
			this.deviceName = "My BLE Device";
			trace(`My device is named ${this.deviceName}\n`);
		}
	}
	let advertiser = new BLEServer;
	
### localAddress
The read-only `localAddress` property accessor function returns the Bluetooth peripheral's local address.

	class Advertiser extends BLEServer {
		onReady() {
			trace(`device address: ${this.localAddress}\n`);
		}
	}
	
### deploy()
Use the `deploy` function to deploy all the server GATT services. Deployed services can be browsed by BLE clients. GATT services are defined in JSON files and described in the [`GATT Services`](#gattservices) section below.

### startAdvertising(params)
The `startAdvertising` function starts broadcasting advertisement and scan response packets. The function is also used to configure discoverable and connectable modes. The `params` includes the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `advertisingData` | `object` | Object containing advertisement data types.
| `connectable` | `boolean` | Optional property to specify connectable mode. Set to `true` to specify unidirected connectable mode; `false` to specify non-connectable mode. Defaults to `true`.
| `discoverable` | `object` | Optional property to specify discoverable mode. Set to `true` to use the general discovery procedure; `false` to specify non-discoverable. Defaults to `true`.
| `fast` | `number` | Optional property to specify the GAP advertisement interval. Set to `true` to specify TGAP(adv_fast_interval1); `false` to specify TGAP(adv_slow_interval). Defaults to `true`.
| `scanResponseData` | `object` | Optional object containing scan response data types.

The `advertisementData` and `scanResponseData` contain one or more properties corresponding to the [Bluetooth Advertisement Data Types](https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile):

| Property | Type | Description |
| --- | --- | :--- |
| `incompleteUUID16List` | `array` | Array of strings corresponding to *Incomplete List of 16 bit Service UUIDs*.
| `completeUUID16List` | `array ` | Array of strings corresponding to *Complete List of 16 bit Service UUIDs*.
| `incompleteUUID128List` | `array` | Array of strings corresponding to *Incomplete List of 128 bit Service UUIDs*.
| `completeUUID128List` | `array` | Array of strings corresponding to *Complete List of 128 bit Service UUIDs*.
| `shortName` | `string` | String corresponding to the *Shortened Local Name*.
| `completeName` | `string` | String corresponding to the *Complete Local Name*.
| `manufacturerSpecific` | `object` | Object corresponding to the *Manufacturer Specific Data*. The `identifier` property is a number corresponding to the *Company Identifier Code*. The `data` property is an array of numbers corresponding to additional manufacturer specific data.
| `txPowerLevel` | `number` | Number corresponding to the *TX Power Level*.
| `connectionInterval` | `object` | Object corresponding to the *Slave Connection Interval Range*. The `intervalMin` property is a number corresponding to the minimum connection interval value. The `intervalMax` property is a number corresponding to the maximum connection interval value.
| `solicitationUUID16List` | `array` | Array of strings corresponding to the *List of 16 bit Service Solicitation UUIDs*.
| `solicitationUUID128List ` | `array` | Array of strings corresponding to the *List of 128 bit Service Solicitation UUIDs*.
| `serviceDataUUID16` | `array` | Object corresponding to the *Service Data - 16 bit UUID*. The `uuid` property is a string corresponding to the 16-bit Service UUID. The `data` property is an array of numbers corresponding to additional service data.
| `serviceDataUUID128` | `array` | Object corresponding to the *Service Data - 128 bit UUID*. The `uuid` property is a string corresponding to the 128-bit Service UUID. The `data` property is an array of numbers corresponding to additional service data.
| `appearance` | `number` | Number corresponding to the *Appearance*.
| `publicAddress` | `string` | String corresponding to the *Public Target Address*.
| `randomAddress` | `string` | String corresponding to the *Random Target Address*.
| `advertisingInterval` | `number` | Number corresponding to the *Advertising Interval*.
| `role` | `number` | Number corresponding to the *LE Role*.
| `uri` | `string` | String corresponding to the *Uniform Resource Identifier*.

To advertise a [Health Thermometer Sensor](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.health_thermometer.xml) BLE-only connectable device with the complete local name "Thermometer" and one service with UUID 0x1809:

		this.startAdvertising({
			advertisingData: {flags: 6, completeName: "Thermometer", completeUUID16List: ["1809"]}
		});

To advertise a [Heart Rate Sensor](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.heart_rate.xml) BLE-only connectable device with the complete local name "Moddable HRS" and two services with UUIDs 0x180D and 0x180F:

		this.startAdvertising({
			advertisingData: {flags: 6, completeName: "Moddable HRS", completeUUID16List: completeUUID16List: ["180D","180F"]}
		});

To advertise a BLE-only non-connectable and limited discoverable device with the complete local name "Moddable Device" and the short name "Moddable" provided in the scan response:

		this.startAdvertising({
			connectable: false,
			advertisingData: {flags: 5, completeName: "Moddable HRS"},
			scanResponseData: {shortName: "Moddable"} 
		});
		
### stopAdvertising()
Call the `stopAdvertising` function to stop broadcasting Bluetooth advertisements.

	onReady() {
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: "Thermometer Example", completeUUID16List: ["1809"]}
		});
	}
	onConnected(device) {
		this.stopAdvertising();
	}

### close()
Use the `close` function to terminate a BLE client connection. The `onDisconnected` callback function is called when the connection is closed.

	onReady() {
		this.allowed = ["11:22:33:44:55:66"];
	}
	onConnected(device) {
		this.stopAdvertising();
		if (!this.allowed.find(address => address == device.address))
			this.close();
	}
	onDisconnected() {
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: this.deviceName, completeUUID16List: ["180D","180F"]}
		});
	}

### notifyValue(characteristic, value)
Call the `notifyValue` function to send a characteristic value change notification to the connected client. The `value` parameter is an `ArrayBuffer` containing the value. The `onCharacteristicNotifyEnabled` callback function is called when a client enables notifications on the `characteristic`.

To notify a characteristic with the string "hello" every 250 ms:

	onCharacteristicNotifyEnabled(characteristic) {
		this.startNotifications(characteristic);
	}
	startNotifications(characteristic) {
		this.timer = Timer.repeat(id => {
			this.notifyValue(characteristic, ArrayBuffer.fromString("hello"));
		}, 250);
	}

### onCharacteristicWritten(params)
The `onCharacteristicWritten` callback is called when a client writes a service characteristic value on demand. The `params` includes the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `uuid` | `string` | Characteristic UUID.
| `name` | `string` | Characteristic name defined in the service JSON.
| `type` | `string` | Characteristic type defined in the service JSON.
| `handle` | `number` | Characteristic handle.
| `value` | `varies` | The value written. The value is automatically converted to the `type` defined in the service JSON.

The `BLEServer` application is responsible for handling the write request.

The following example from the [wifi-connection-server](../../../examples/network/ble/wifi-connection-server) example shows how characteristic write requests are handled. The `SSID` and `password` characteristics are strings and the `control` characteristic is a numeric value. When the `control` characteristic value is 1, the app initiates a WiFi connection:

	onCharacteristicWritten(params) {
		let value = params.value;
		if ("SSID" == params.name)
			this.ssid = value;
		else if ("password" == params.name)
			this.password = value;
		else if ("control" == params.name) {
			if ((1 == value) && this.ssid) {
				this.close();
				this.connectToWiFiNetwork();
			}
		}
	}

### onCharacteristicRead(params)
The `onCharacteristicRead` callback is called when a client reads a service characteristic value on demand. The `params` includes the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `uuid` | `string` | Characteristic UUID.
| `name` | `string` | Characteristic name defined in the service JSON.
| `type` | `string` | Characteristic type defined in the service JSON.
| `handle` | `number` | Characteristic handle.

The `BLEServer` application is responsible for handling the read request.

To respond to a read request corresponding to a numeric "status" characteristic:

	onReady() {
		this.status = 10;
	}
	onCharacteristicRead(params) {
		if ("status" == params.name)
			return this.status;
	}

<a id="gattservices"></a>
## GATT Services
GATT services are defined by simple JSON files located in a BLE server project's `bleservices` directory. Each JSON file defines a single service with one or more characteristics. The JSON is automatically converted to platform-specific native code by the Moddable `mcconfig` command line tool and the compiled object code is linked to the app. The object code lives in Flash memory, significantly reducing the footprint required to deploy GATT services.

The following is an example of JSON corresponding to a Bluetooth [Battery Service](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.battery_service.xml):

	{
		"service": {
			"uuid": "180F",
			"characteristics": {
				"battery": {
					"uuid": "2A19",
					"maxBytes": 1,
					"type": "Uint8",
					"permissions": "read",
					"properties": "read",
					"value": 85
				},
			}
		}
	}
	
The service is defined as follows:

* The service UUID is 0x180F
* The service contains a single characteristic named "battery"
	* The characteristic UUID is 0x2A19
	* The characteristic represents an unsigned 8-bit value
	* The characteristic value is 85 and clients only have permission to read the characteristic

Characteristics that include a `value` property are considered static. The `BLEServer` class automatically responds to read requests for static characteristic values, further reducing the script code required to host a GATT service.

The service JSON includes the following top-level properties:

| Property | Value |
| --- | --- | :--- |
| `service` | Service object. Each JSON file must contain only a single `service` property.
| `uuid` | Service UUID.
| `characteristics` | Service characteristic(s). Each key corresponds to a characteristic name.

Each JSON `characteristic` contains the following properties:

| Property | Value |
| --- | --- | :--- |
| `uuid` | Characteristic UUID.
| `maxBytes` | Maximum number of bytes required to store the characteristic value.
| `type` | Optional JavaScript data value type. Supported types include `Array`, `String`, `Uint8`, `Uint16` and `Uint32`. If the `type` property is not present, the data value type defaults to `ArrayBuffer`. The `BLEServer` class automatically converts characteristic values delivered in buffers by the underlying BLE implementation to the requested `type`.
| `permissions` | Characteristic permissions. Supported permissions include `read` and `write`.
| `properties` | Characteristic properties. Supported properties include `read`, `write`, `notify` and `indicate`.

<a id="esp32platform"></a>
## BLE Apps on ESP32 Platform
In order to enable the BLE client or server on the ESP32 platform, the [sdkconfig](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj/sdkconfig) file must set the following core BLE features:

	CONFIG_BT_ENABLED=y
	CONFIG_BTDM_CONTROLLER_PINNED_TO_CORE=0
	CONFIG_BTDM_CONTROLLER_HCI_MODE_VHCI=y
	CONFIG_BLUEDROID_ENABLED=y
	CONFIG_BLUEDROID_PINNED_TO_CORE=0
	CONFIG_BTC_TASK_STACK_SIZE=3072
	CONFIG_BLE_SMP_ENABLE=y
	CONFIG_BT_ACL_CONNECTIONS=1
	CONFIG_SMP_ENABLE=y
	CONFIG_BT_RESERVE_DRAM=0x10000
	
>**Note:** The `CONFIG_BT_ACL_CONNECTIONS` value can be increased to support more than one BLE client connection. This value should match the `max_connections` value in the application manifest.

The core features defined above build the core ESP32 BLE firmware, but not the client or server interfaces. To enable only BLE client functionality, additionally set the `CONFIG_GATTC_ENABLE` feature:

	CONFIG_GATTC_ENABLE=y
	
To enable only BLE server functionality, additionally set the `CONFIG_GATTS_ENABLE` feature:

	CONFIG_GATTS_ENABLE=y
	
> **Note:** Because both a BLE client and server cannot be enabled on the same device, we recommend setting only the `CONFIG_GATTC_ENABLE` or `CONFIG_GATTS_ENABLE` feature, but not both. This will conserve memory and Flash.

Once the sdkconfig file changes have been made, build the [scanner](../../../examples/network/ble/scanner) BLE app for the ESP32 platform:

	cd $MODDABLE/examples/network/ble/scanner
	mcconfig -d -m -p esp32

<a id="geckoplatform"></a>
## BLE Apps on Blue Gecko Platform
Building and deploying BLE apps on Blue Gecko follow the same workflow outlined in our [Gecko developer documentation](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/gecko/GeckoBuild.md). For BLE apps, we recommend starting from the `soc-ibeacon` Simplicity Studio example project.

The [make.blue.mk](../../../tools/mcconfig/geck0/make.blue.mk) makefile includes variables that define the Gecko target platform, kit and part. The makefile is configured by default to build apps for the Blue Gecko [EFR32BG13P632F512GM48](https://www.silabs.com/products/wireless/bluetooth/blue-gecko-bluetooth-low-energy-socs/device.efr32bg13p632f512gm48) Bluetooth low energy chip mounted on the [BRD4104A](https://www.silabs.com/documents/login/reference-manuals/brd4104a-rm.pdf) 2.4 GHz 10 dBm Radio Board. To configure the build for a different Blue Gecko target, change the makefile `GECKO_BOARD`, `GECKO_PART`, `HWKIT` and `HWINC` accordingly.

To build the [scanner](../../../examples/network/ble/scanner) BLE app `xs_gecko.a` archive for Blue Gecko:

	cd $MODDABLE/examples/network/ble/scanner
	mcconfig -d -m -p gecko/blue

After building the BLE archive, the archive is linked to the native app hosted in Simplicity Studio. The `main()` native code function for a basic BLE client/server app can be reduced to the following:

	void main(void)
	{
		initMcu();
		initBoard();
		initApp();
		
		xs_setup();
		
		while (1) {
			xs_loop();
		}
	}

BLE apps require additional stack and heap. We've been able to run the Moddable BLE example apps using the following values:

	__STACK_SIZE=0x2000
	__HEAP_SIZE=0xA000
	
Simplicity Studio includes a **BLE GATT Configurator** to define BLE services. Moddable apps define BLE services in JSON files and hence don't use the `gatt_db.c` and `gatt_db.h` files generated by the configurator tool. These two files must be removed from the Simplicity Studio project.

<a id="exampleapps"></a>
## BLE Example Apps
The Moddable SDK includes many BLE client and server example apps to build from. We recommend starting from an example app, since the apps demonstrate how to implement common use cases:

### Client Apps

[ble-friend](../../../examples/network/ble/ble-friend)

Shows how to interact with the Adafruit BLE Friend [UART service](https://learn.adafruit.com/introducing-adafruit-ble-bluetooth-low-energy-friend/uart-service) RX and TX characteristics.

[colorific](../../../examples/network/ble/colorific)

Randomly changes the color of a BLE bulb every 100 ms.

[discovery](../../../examples/network/ble/discovery)

Demonstrates how to perform full service and characteristic discovery.

[powermate](../../../examples/network/ble/powermate)

Recieves button spin and press notifications from the [Griffin BLE Multimedia Control Knob](https://griffintechnology.com/us/powermate-bluetooth).

[scanner](../../../examples/network/ble/scanner)

Scans for and displays peripheral advertised names.

[sensortag](../../../examples/network/ble/sensortag)

Receives sensor notifications from the [TI CC2541 SensorTag](http://www.ti.com/tool/CC2541DK-SENSOR#technicaldocuments) on-board sensors.

[tempo](../../../examples/network/ble/tempo)

Reads temperature, humidity and barometric pressure from a [Blue Maestro Environment Monitor](https://www.bluemaestro.com/product/tempo-environment-monitor/) beacon.

### Server Apps

[advertiser](../../../examples/network/ble/advertiser)

Broadcasts advertisements until a BLE client connects.

[health-thermometer-server](../../../examples/network/ble/health-thermometer-server)

Implements the Bluetooth [Health Thermometer Service](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.health_thermometer.xml).

[heart-rate-server](../../../examples/network/ble/heart-rate-server)

Implements the Bluetooth [Heart Rate Service](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.heart_rate.xml).

[uri-beacon](../../../examples/network/ble/uri-beacon)

Implements a [UriBeacon](https://github.com/google/uribeacon/tree/uribeacon-final/specification) compatible with Google's [Physical Web](https://github.com/google/physical-web) discovery service.

[wifi-connection-server](../../../examples/network/ble/wifi-connection-server)

Deploys a BLE WiFi connection service on ESP32. The connection service allows BLE clients to connect the BLE device to a WiFi access point, by writing the SSID and password characteristics. 
