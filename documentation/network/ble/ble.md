# BLE
Copyright 2017-18 Moddable Tech, Inc.

Revised: June 6, 2018

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
	* [Class Bytes](#classbytes)
* [BLE Server](#bleserver)
	* [Class BLEServer](#classbleserver)
	* [GATT Services](#gattservices)
* [BLE Security](#blesecurity)
	* [Class SM](#classsm)
* [BLE Apps on ESP32 Platform](#esp32platform)
* [BLE Apps on Blue Gecko Platform](#geckoplatform)
* [BLE Example Apps](#exampleapps)

<a id="addingble"></a>
## Adding BLE to a Project
The BLE client and server are implemented as separate modules to accomodate the limited memory and storage available on embedded devices. BLE applications instantiate a BLE client or BLE server, but never both.

Pre-made manifests are available for the BLE client and BLE server. Add them to the `include` array of your application's manifest to use them in your project.

To add the BLE client to a project:

	"include": [
		/* other includes here */
		"$(MODDABLE)/modules/network/ble/manifest_client.json",
	],
	
To add the BLE server to a project:

	"include": [
		/* other includes here */
		"$(MODDABLE)/modules/network/ble/manifest_server.json",
	],

<a id="usingble"></a>
## Using BLE

BLE applications typically subclass `BLEClient` or `BLEServer` and then add device and/or application specific code. The following BLE client example subclasses `BLEClient` to scan for and discover peripherals:

```javascript
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
```

The following BLE server example subclasses `BLEServer` to advertise the device as a BLE Health Thermometer peripheral:

```javascript
import BLEServer from "bleserver";
import {uuid} from "btutils";
	
class HealthThermometerService extends BLEServer {
	onReady() {
		this.startAdvertising({
			advertisingData: {flags: 6, completeName: "Moddable HTM", completeUUID16List: [uuid`1809`, uuid`180F`]}
		});
	}
}
	
let htm = new HealthThermometerService;
```

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

The following code from the [powermate](../../../examples/network/ble/powermate) example app shows a typical client flow:

```javascript
const DEVICE_NAME = 'PowerMate Bluetooth';
const SERVICE_UUID = uuid`25598CF7-4240-40A6-9910-080F19F91EBC`;
const CHARACTERISTIC_UUID = uuid`9CF53570-DDD9-47F3-BA63-09ACEFC60415`;

class PowerMate extends BLEClient {
	onReady() {
		this.startScanning();
	}
	onDiscovered(device) {
		if (DEVICE_NAME == device.scanResponse.completeName) {
			this.stopScanning();
			this.connect(device);
		}
	}
	onConnected(device) {
		device.discoverPrimaryService(SERVICE_UUID);
	}
	onServices(services) {
		let service = services.find(service => service.uuid.equals(SERVICE_UUID));
		if (service)
			service.discoverCharacteristic(CHARACTERISTIC_UUID);
	}
	onCharacteristics(characteristics) {
		let characteristic = characteristics.find(characteristic => characteristic.uuid.equals(CHARACTERISTIC_UUID));
		if (characteristic)
			characteristic.enableNotifications();
	}
	onCharacteristicNotification(characteristic, buffer) {
		let value = new Uint8Array(buffer)[0];
		trace(`value: ${value}\n`);
	}
}
```

A BLE client is fundamentally asynchronous and results are always delivered to the `BLEClient` class callback methods, e.g. `onReady`, `onConnected`, etc. above. The following sections describe the BLE client classes, properties, and callbacks.

<a id="classbleclient"></a>
## Class BLEClient

The `BLEClient` class provides access to the BLE client features.

```javascript
import BLEClient from "bleclient";
```

### Functions

#### `onReady()`

Applications must wait for the `onReady` callback before calling other `BLEClient` functions:

```javascript
class Client extends BLEClient {
	onReady() {
		/* stack is ready to use */
	}
}
let client = new BLEClient;
```

***

#### `startScanning([params])`

| Argument | Type | Description |
| --- | --- | :--- | 
| `params` | `object` | Object containing scan properties.

The `params` object contains the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `active` | `boolean` | Set `true` for active scanning, `false` for passing scanning. Default is `true`.
| `interval` | `number` | Scan interval value in units of 0.625 ms. Default is `0x50`. 
| `window` | `number` | Scan window value in units of 0.625 ms. Default is `0x30`. 


The `startScanning` function enables scanning for nearby peripherals.

If no `params` argument is provided, the default scan properties are used:

```javascript
class Scanner extends BLEClient {
	onReady() {
		this.startScanning();
	}
	onDiscovered(device) {
		trace("device discovered\n");
	}
}
```

Passing in a `params` object overrides the defaults. This example uses passive scanning at a 70ms interval:

```javascript
class Scanner extends BLEClient {
	onReady() {
		this.startScanning({ active:false, interval:0x70 });
	}
	onDiscovered(device) {
		trace("device discovered\n");
	}
}
```
***

#### `onDiscovered(device)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `device` | `object` | A `device` object. See the section [Class Device](#classdevice) for more information. |

The `onDiscovered` callback function is called one or more times for each peripheral device discovered. 

To connect to a device named "Brian" from the `onDiscovered` callback:

```javascript
onDiscovered(device) {
	if ("Brian" == 	device.scanResponse.completeName) {
		this.connect(device);
	}
}
```

***
	
#### `stopScanning()`
The `stopScanning` function disables scanning for nearby peripherals.

***

#### `connect(device)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `device` | `object` | A `device` object. See the section [Class Device](#classdevice) for more information. |

The `connect` function initiates a connection request between the `BLEClient` and a target peripheral `device`.

```javascript
onDiscovered(device) {
	this.connect(device);
}
onConnected(device) {
	trace("Connected to device\n");
}
```

***

#### `onConnected(device)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `device` | `object` | A `device` object. See the section [Class Device](#classdevice) for more information. |

The `onConnected` callback function is called when the client connects to a target peripheral `device`.

> **Note:** The `BLEClient` object hosts all callbacks for classes used with `BLEClient`.

***

<a id="classdevice"></a>
## Class Device
An instance of the `Device` class is instantiated by `BLEClient` and provided to the host app in the `BLEClient` `onDiscovered` and `onConnected` callbacks. While applications never instantiate a `Device` class instance directly, applications do call `Device` class functions to perform GATT service/characteristic discovery and close the peripheral connection.

### Properties

| Name | Type | Description |
| --- | --- | :--- |
| `connection` | `number` | Connection identifier.
| `address` | `object` | Instance of [Bytes](#classbytes) class containing Bluetooth address bytes.
| `scanResponse` | `object` | Instance of [Advertisement](#classadvertisement) class containing advertisement and scan response packet values.

### Functions 

#### `readRSSI()`
Use the `readRSSI` function to read the connected peripheral's signal strength.

***

#### `onRSSI(device, rssi)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `device` | `object` | A `device` object. See the section [Class Device](#classdevice) for more information. |
| `rssi` | `number` | Received signal strength |

The `onRSSI` callback function is called when the peripheral signal strength is read.

To read the signal strength of the connected device:

```javascript
onConnected(device) {
	device.readRSSI();
}
onRSSI(device, rssi) {
	trace(`signal strength is ${rssi}\n`);
}
```

***

#### `discoverAllPrimaryServices()`
Use the `discoverAllPrimaryServices` function to discover all the peripheral's GATT primary services. Discovered services are returned to the `onServices` callback.

***

#### `discoverPrimaryService(uuid)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `uuid` | `object` | A [Bytes](#classbytes) object containing the UUID to discover |

Use the `discoverPrimaryService` function to discover a single GATT primary service by UUID.
***

#### `findServiceByUUID(uuid)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `uuid` | `object` | A [Bytes](#classbytes) object containing the Service UUID to find |

The `findServiceByUUID` function finds and returns the service identified by `uuid`. This function searches the `services` property array.

***

#### `onServices(services)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `services` | `array` | An array of `service` objects, or an empty array if no services are discovered. See the section [Class Service](#classservice) for more information. |

The `onServices` callback function is called when service discovery completes. If `findServiceByUUID` was called to find a single service, the `services` array contains the single service found.

To discover all primary services:

```javascript
onConnected(device) {
	device.discoverAllPrimaryServices();
}
onServices(services) {
	trace(`${services.length} services found\n`);
}
```

To discover a single primary service:	

```javascript
const SERVICE_UUID = uuid`0xFF00`;

onConnected(device) {
	device.discoverPrimaryService(SERVICE_UUID);
}
onServices(services) {
	if (services.length)
		trace("found service\n");
}
```
> **Note:** The `uuid` tagged template allows applications to represent UUID values as strings. Refer to the [Bytes](#classbytes) class section for details.

***

#### `close()`
The `close` function terminates the peripheral connection.

```javascript
onConnected(device) {
	trace("connected to device\n");
	device.close();
}
onDisconnected() {
	trace("connection closed\n");
}
```

***

#### `onDisconnected()`
The `onDisconnected` callback is called when the connection is closed.

<a id="classservice"></a>
## Class Service
The `Service` class provides access to a single peripheral GATT service. 

### Properties

| Name | Type | Description |
| --- | --- | :--- |
| `connection` | `number` | Connection identifier.
| `uuid` | `object` | Instance of [Bytes](#classbytes) class containing service UUID.
| `start` | `number` | Starting handle of included characteristics.
| `end` | `number` | Ending handle of included characteristics.
| `characteristics` | `array` | Array of service characteristics discovered.

### Functions

#### `discoverAllCharacteristics()`
Use the `discoverAllCharacteristics()` function to discover all the service characteristics. Discovered characteristics are returned to the `onCharacteristics` callback.

***

#### `discoverCharacteristic(uuid)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `uuid` | `object` | A [Bytes](#classbytes) object containing the Characteristic UUID to discover |

Use the `discoverCharacteristic` function to discover a single service characteristic by UUID.

***

#### `findCharacteristicByUUID(uuid)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `uuid` | `object` | A [Bytes](#classbytes) object containing the Characteristic UUID to find |

The `findCharacteristicByUUID` function finds and returns the characteristic identified by `uuid`. This function searches the `characteristics` property array.

***

#### `onCharacteristics(characteristics)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `characteristics` | `array` | An array of `characteristic` objects, or an empty array if no characteristics are discovered. See the section [Class Characteristic](#classcharacteristic) for more information. |

The `onCharacteristics` callback function is called when characteristic discovery completes. If `findCharacteristicByUUID` was called to find a single characteristic, the `characteristics` array contains the single characteristic found.

***

To discover all the characteristics in the [Device Information](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.device_information.xml) service:

```javascript
const DEVICE_INFORMATION_SERVICE_UUID = uuid`180A`;

onServices(services) {
	let service = services.find(service => service.uuid.equals(DEVICE_INFORMATION_SERVICE_UUID));
	if (service)
		service.discoverAllCharacteristics();
}
onCharacteristics(characteristics) {
	trace(`${characteristics.length} characteristics found\n`);
}
```

To find the [Heart Rate Measurement](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.heart_rate_measurement.xml) characteristic in the [Heart Rate](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.heart_rate.xml) service:

```javascript
const HEART_RATE_SERVICE_UUID = uuid`180A`;
const HEART_RATE_MEASUREMENT_UUID = uuid`2A37`;

onConnected(device) {
	device.discoverPrimaryService(HEART_RATE_SERVICE_UUID);
}
onServices(services) {
	if (services.length)
		services[0].discoverCharacteristic(HEART_RATE_MEASUREMENT_UUID);
}
onCharacteristics(characteristics) {
	if (characteristics.length)
		trace(`found heart rate measurement characteristic\n`);
}
```

***

<a id="classcharacteristic"></a>
## Class Characteristic
The `Characteristic` class provides access to a single service characteristic.

### Properties

| Name | Type | Description |
| --- | --- | :--- |
| `connection` | `number` | Connection identifier.
| `uuid` | `object` | Instance of [Bytes](#classbytes) class containing characteristic UUID.
| `service` | `object` | `Service` object containing characteristic.
| `handle` | `number` | Chararacteristic handle.
| `descriptors` | `array` | Array of characteristic descriptors discovered.

### Functions

#### `discoverAllDescriptors()`
Use the `discoverAllDescriptors` function to discover all the characteristic's descriptors. Discovered descriptors are returned to the `onDescriptors` callback.

***

#### `onDescriptors(descriptors)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `descriptors` | `array` | An array of `descriptor` objects, or an empty array if no descriptors are discovered. See the section [Class Descriptor](#classdescriptor) for more information. |

The `onDescriptors` callback function is called when descriptor discovery completes.

To discover the [Characteristic Presentation Format Descriptor](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.characteristic_presentation_format.xml) for a characteristic with UUID 0xFF00:

```javascript
const CHARACTERISTIC_UUID = uuid`FF00`;
const PRESENTATION_FORMAT_UUID = uuid`2904`;

onCharacteristics(characteristics) {
	let characteristic = characteristics.find(characteristic => characteristic.uuid.equals(CHARACTERISTIC_UUID));
	if (characteristic)
		characteristic.discoverAllDescriptors();
}
onDescriptors(descriptors) {
	let descriptor = descriptors.find(descriptor => descriptor.uuid.equals(PRESENTATION_FORMAT_UUID));
	if (descriptor)
		trace("found characteristic presentation format descriptor\n");
}
```

***

#### `enableNotifications()`
Use the `enableNotifications` function to enable characteristic value change notifications.

***

#### `disableNotifications()`
Use the `disableNotifications` function to disable characteristic value change notifications.

***

#### `onCharacteristicNotification(characteristic, value)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `characteristic` | `object` | A `characteristic` object. |
| `value` | `ArrayBuffer` | The `characteristic` value. |

The `onCharacteristicNotification` callback function is called when notifications are enabled and the peripheral notifies the characteristic value.

To enable and receive characteristic value change notifications for the [Battery Level](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.battery_level.xml) characteristic in the [Battery Service](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.battery_service.xml):

```javascript
const BATTERY_SERVICE_UUID = uuid`180F`;
const BATTERY_LEVEL_UUID = uuid`2A19`;

onConnected(device) {
	device.discoverPrimaryService(BATTERY_SERVICE_UUID);
}
onServices(services) {
	if (services.length)
		services[0].discoverCharacteristic(BATTERY_LEVEL_UUID);
}
onCharacteristics(characteristics) {
	if (characteristics.length)
		characteristics[0].enableNotifications();
}
onCharacteristicNotification(characteristic, value) {
	let level = new Uint8Array(value)[0];
	trace(`battery level: ${level}%\n`);
}
```

***

#### `readValue()`
Use the `readValue` function to read a characteristic value on demand.

***

#### `onCharacteristicValue(characteristic, value)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `characteristic` | `object` | A `characteristic` object. |
| `value` | `ArrayBuffer` | The `characteristic` value. |

The `onCharacteristicValue` callback function is called when a characteristic is read by the `readValue` function.

To read the [Device Name](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.gap.device_name.xml) from the [Generic Access Service](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.generic_access.xml):

```javascript
const GENERIC_ACCESS_SERVICE_UUID = uuid`1800`;
const DEVICE_NAME_UUID = uuid`2A00`;

onConnected(device) {
	device.discoverPrimaryService(GENERIC_ACCESS_SERVICE_UUID);
}
onServices(services) {
	if (services.length)
		services[0].discoverCharacteristic(DEVICE_NAME_UUID);
}
onCharacteristics(characteristics) {
	if (characteristics.length)
		characteristics[0].readValue();
}
onCharacteristicValue(characteristic, value) {
	let name = String.fromArrayBuffer(value);
	trace(`device name: ${name}\n`);
}
```

***

#### `writeWithoutResponse(value)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `value` | `ArrayBuffer` | The `characteristic` value to write. |

Use the `writeWithoutResponse` function to write a characteristic value on demand.

To write the [URI](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.uri.xml) characteristic value in the [HTTP Proxy](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.http_proxy.xml) service:

```javascript
const HTTP_PROXY_SERVICE_UUID = uuid`1823`;
const URI_UUID = uuid`2AB6`;

onConnected(device) {
	device.discoverPrimaryService(HTTP_PROXY_SERVICE_UUID);
}
onServices(services) {
	if (services.length)
		services[0].discoverCharacteristic(URI_UUID);
}
onCharacteristics(characteristics) {
	if (characteristics.length)
		characteristics[0].writeWithoutResponse(ArrayBuffer.fromString("http://moddable.tech"));
}
```

***

<a id="classdescriptor"></a>
## Class Descriptor
The `Descriptor` class provides access to a single characteristic descriptor.

### Properties

| Name | Type | Description |
| --- | --- | :--- |
| `connection` | `number` | Connection identifier.
| `uuid` | `string` | Instance of [Bytes](#classbytes) class containing descriptor UUID.
| `characteristic` | `object` | `Characteristic` object containing descriptor.
| `handle` | `number` | Descriptor handle.


### Functions

#### `writeValue(value)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `value` | `ArrayBuffer` | The descriptor value to write. |

Use the `writeValue` function to write a descriptor value.

To enable characteristic value change notifications by writing 0x0001 to the [Client Characteristic Configuration](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml) descriptor:

```javascript
const CCCD_UUID = uuid`2902`;

onDescriptors(descriptors) {
	let descriptor = descriptors.find(descriptor => descriptor.uuid.equals(CCCD_UUID));
	if (descriptor) {
		let value = Uint8Array.of(0x01, 0x00);	// little endian
		descriptor.writeValue(value.buffer);
	}
}
```
	
> **Note:** The `Characteristic` `enableNotifications` convenience function is typically used for this purpose, though writing the CCCD descriptor directly provides the same net result.

<a id="classadvertisement"></a>
## Class Advertisement

The `Advertisement` class provides accessor functions to read common advertisement and scan response data types as JavaScript properties. The class is primarily used to parse the `scanResponseData` provided to BLE clients via the `onDiscovered` callback function's `device` parameter. The accessor functions return `undefined` if the associated data type is not available in the `scanResponseData`.

### Properties

| Name | Type | Description
| --- | --- | :--- |
| `completeName` | `string` | The advertised complete local name.
| ` shortName` | `string` | The advertised shortened local name.
| `manufacturerSpecific` | `object` | An object containing the advertised manufacturer specific data.
| `flags` | `number` | The advertised flags value.


### Examples 

To identify an advertised device with the complete name "Zip":

```javascript
onDiscovered(device) {
	let completeName = device.scanResponse.completeName;
	if (completeName == "ZIP")
		trace("Found ZIP device\n");
	}
}
```

To identify a [Blue Maestro Environment Monitor](https://www.bluemaestro.com/product/tempo-environment-monitor/) and read the temperature from the manufacturer specific data:

```javascript
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
```

<a id="classbytes"></a>
## Class Bytes

The private `Bytes` class extends `ArrayBuffer`. Applications typically use the provided `uuid` and `address` tagged templates to create a `Bytes` instance from hex strings.
 
#### Constructor Description

#### `Bytes(bytes [,littleEndian])`

| Argument | Type | Description |
| --- | --- | :--- | 
| `bytes` | `object` or `string` | The ArrayBuffer `bytes` to set. The bytes can be a String or ArrayBuffer. |
| `littleEndian` | `boolean` | When set `true` the contents of the ArrayBuffer are stored in reverse order. Default is `false`.|
### Functions

#### `address`\``string``

The `address` tagged template is used to convert a Bluetooth address expressed as a hex string to a `Bytes` instance.

```javascript
import {address} from "btutils";

const DEVICE_ADDRESS = address`66:55:44:33:22:11`;
```

#### `uuid`\``string``

The `uuid` tagged template is used to convert a Bluetooth UUID expressed as a hex string to a `Bytes` instance.

```javascript
import {uuid} from "btutils";

const HTM_SERVICE = uuid`1809`;
const CHARACTERISTIC_RX_UUID = uuid`6E400002-B5A3-F393-E0A9-E50E24DCCA9E`;
```

#### `equals(buffer)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `buffer` | `object` | The `Bytes` instance to compare. |

The `equals` function returns `true` if the instance ArrayBuffer data equals the data contained in `buffer`.
 
```javascript
import {uuid} from "btutils";

const HTM_SERVICE = uuid`1809`;
onServices(services) {
	let found = services[0].uuid.equals(HTM_SERVICE);
	if (found)
		trace("found HTM service\n");
}
```

<a id="bleserver"></a>
## BLE Server
A BLE server/peripheral can connect to one BLE client and typically performs the following steps to send notifications to a BLE client:

1. Deploy services
2. Start advertising so that clients can discover the peripheral
3. Establish a connection with a client
4. Accept characteristic value change notification request(s)
5. Notify characteristic value changes.

The following abbreviated code from the [heart-rate-server](../../../examples/network/ble/heart-rate-server) example app shows a typical server flow:

```javascript
onReady() {
	this.bpm = [0, 60]; // flags, beats per minute
	this.deploy();
	this.startAdvertising({
		advertisingData: {flags: 6, completeName: "Moddable HRS", completeUUID16List: [uuid`180D`, uuid`180F`]}
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
```

A BLE server is fundamentally asynchronous and results are always delivered to the `BLEServer` class callback methods, e.g. `onReady`, `onConnected`, etc. above. The following sections describe the `BLEServer` class, properties, and callbacks.

<a id="classbleserver"></a>
## Class BLEServer

The `BLEServer` class provides access to the BLE server features.

```javascript
import BLEServer from "bleserver";
```

### Functions

#### `onReady()`
Applications must wait for the `onReady` callback before calling other `BLEServer` functions:

```javascript
class Server extends BLEServer {
	onReady() {
		/* stack is ready to use */
	}
}
let server = new BLEServer;
```

***

#### `deviceName`
The `deviceName` property accessor function is used to set/get the Bluetooth peripheral device name.

```javascript
class Advertiser extends BLEServer {
	onReady() {
		this.deviceName = "My BLE Device";
		trace(`My device is named ${this.deviceName}\n`);
	}
}
let advertiser = new BLEServer;
```

***

#### `localAddress`
The read-only `localAddress` property accessor function returns the Bluetooth peripheral's local address as a [Bytes](#classbytes) object.

```javascript
import Hex from "hex";

class Advertiser extends BLEServer {
	onReady() {
		trace(`device address: ${Hex.toString(this.localAddress, ":")}\n`);
	}
}
```

***

#### `deploy()`
Use the `deploy` function to deploy all the server GATT services. Deployed services can be browsed by BLE clients. GATT services are defined in JSON files and described in the [`GATT Services`](#gattservices) section below.

***

#### startAdvertising(params)

| Argument | Type | Description |
| --- | --- | :--- |
| `params` | `object` | Object containing advertisement properties.

The `params` object contains the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `advertisingData` | `object` | Object containing advertisement data properties.
| `connectable` | `boolean` | Optional property to specify connectable mode. Set to `true` to specify unidirected connectable mode; `false` to specify non-connectable mode. Defaults to `true`.
| `discoverable` | `object` | Optional property to specify discoverable mode. Set to `true` to use the general discovery procedure; `false` to specify non-discoverable. Defaults to `true`.
| `fast` | `boolean` | Optional property to specify the GAP advertisement interval. Set to `true` to specify TGAP(adv\_fast\_interval1); `false` to specify TGAP(adv\_slow\_interval). Defaults to `true`.
| `scanResponseData` | `object` | Optional object containing scan response data properties.

The `startAdvertising` function starts broadcasting advertisement and scan response packets. The function is also used to configure discoverable and connectable modes.

The `advertisementData` and `scanResponseData` contain one or more properties corresponding to the [Bluetooth Advertisement Data Types](https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile):

| Property | Type | Description |
| --- | --- | :--- |
| `incompleteUUID16List` | `array` | Array of UUID objects corresponding to *Incomplete List of 16 bit Service UUIDs*.
| `completeUUID16List` | `array ` | Array of UUID objects corresponding to *Complete List of 16 bit Service UUIDs*.
| `incompleteUUID128List` | `array` | Array of UUID objects corresponding to *Incomplete List of 128 bit Service UUIDs*.
| `completeUUID128List` | `array` | Array of UUID objects corresponding to *Complete List of 128 bit Service UUIDs*.
| `shortName` | `string` | String corresponding to the *Shortened Local Name*.
| `completeName` | `string` | String corresponding to the *Complete Local Name*.
| `manufacturerSpecific` | `object` | Object corresponding to the *Manufacturer Specific Data*. The `identifier` property is a number corresponding to the *Company Identifier Code*. The `data` property is an array of numbers corresponding to additional manufacturer specific data.
| `txPowerLevel` | `number` | Number corresponding to the *TX Power Level*.
| `connectionInterval` | `object` | Object corresponding to the *Slave Connection Interval Range*. The `intervalMin` property is a number corresponding to the minimum connection interval value. The `intervalMax` property is a number corresponding to the maximum connection interval value.
| `solicitationUUID16List` | `array` | Array of UUID objects corresponding to the *List of 16 bit Service Solicitation UUIDs*.
| `solicitationUUID128List ` | `array` | Array of UUID objects corresponding to the *List of 128 bit Service Solicitation UUIDs*.
| `serviceDataUUID16` | `array` | Object corresponding to the *Service Data - 16 bit UUID*. The `uuid` property is an object corresponding to the 16-bit Service UUID. The `data` property is an array of numbers corresponding to additional service data.
| `serviceDataUUID128` | `array` | Object corresponding to the *Service Data - 128 bit UUID*. The `uuid` property is an object corresponding to the 128-bit Service UUID. The `data` property is an array of numbers corresponding to additional service data.
| `appearance` | `number` | Number corresponding to the *Appearance*.
| `publicAddress` | `object` | Address object corresponding to the *Public Target Address*.
| `randomAddress` | `object` | Address object corresponding to the *Random Target Address*.
| `advertisingInterval` | `number` | Number corresponding to the *Advertising Interval*.
| `role` | `number` | Number corresponding to the *LE Role*.
| `uri` | `string` | String corresponding to the *Uniform Resource Identifier*.

To advertise a [Health Thermometer Sensor](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.health_thermometer.xml) BLE-only connectable device with the complete local name "Thermometer" and one service with UUID 0x1809:

```javascript
import {uuid} from "btutils";

this.startAdvertising({
	advertisingData: {flags: 6, completeName: "Thermometer", completeUUID16List: [uuid`1809`]}
});
```

To advertise a [Heart Rate Sensor](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.heart_rate.xml) BLE-only connectable device with the complete local name "Moddable HRS" and two services with UUIDs 0x180D and 0x180F:

```javascript
import {uuid} from "btutils";

this.startAdvertising({
	advertisingData: {flags: 6, completeName: "Moddable HRS", completeUUID16List: completeUUID16List: [uuid`180D`, uuid`180F`]}
});
```

To advertise a BLE-only non-connectable and limited discoverable device with the complete local name "Moddable HRS" and the short name "Moddable" and random target address 00:11:22:33:44:55 provided in the scan response:

```javascript
import {address} from "btutils";

this.startAdvertising({
	connectable: false,
	advertisingData: {flags: 5, completeName: "Moddable HRS"},
	scanResponseData: {shortName: "Moddable", randomAddress: address`00:11:22:33:44:55`} 
});
```

***

#### `stopAdvertising()`
Call the `stopAdvertising` function to stop broadcasting Bluetooth advertisements.

```javascript
import {uuid} from "btutils";

onReady() {
	this.startAdvertising({
		advertisingData: {flags: 6, completeName: "Thermometer Example", completeUUID16List: [uuid`1809`]}
	});
}
onConnected(device) {
	this.stopAdvertising();
}
```

***

#### `close()`
Use the `close` function to terminate a BLE client connection. 

```javascript
import {address} from "btutils";

onReady() {
	this.allowed = address`11:22:33:44:55:66`;
}
onConnected(device) {
	this.stopAdvertising();
	if (!device.address.equals(this.allowed))
		this.close();
}
```

***

#### `onDisconnected()`
The `onDisconnected` callback function is called when the connection is closed.

***

#### `notifyValue(characteristic, value)`
| Argument | Type | Description |
| --- | --- | :--- | 
| `characteristic` | `object` | The `characteristic` object to notify. |
| `value` | `ArrayBuffer` | The `characteristic` notification `value`. |

Call the `notifyValue` function to send a characteristic value change notification to the connected client.

To notify a characteristic with the string "hello" every 250 ms:

```javascript
onCharacteristicNotifyEnabled(characteristic) {
	this.startNotifications(characteristic);
}
startNotifications(characteristic) {
	this.timer = Timer.repeat(id => {
		this.notifyValue(characteristic, ArrayBuffer.fromString("hello"));
	}, 250);
}
```

***

#### `onCharacteristicNotifyEnabled(characteristic)`
| Argument | Type | Description |
| --- | --- | :--- | 
| `characteristic` | `object` | The `characteristic` object with notifications enabled.

The `onCharacteristicNotifyEnabled` callback function is called when a client enables notifications on the `characteristic`.
 
***

#### onCharacteristicWritten(params)
| Argument | Type | Description |
| --- | --- | :--- | 
| `params` | `object` | Properties associated with the characteristic value written.

The `params` object contains the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `uuid` | `object` | Instance of [Bytes](#classbytes) class containing Characteristic UUID.
| `name` | `string` | Characteristic name defined in the service JSON.
| `type` | `string` | Characteristic type defined in the service JSON.
| `handle` | `number` | Characteristic handle.
| `value` | `varies` | The value written. The value is automatically converted to the `type` defined in the service JSON.

The `onCharacteristicWritten` callback is called when a client writes a service characteristic value on demand. The `BLEServer` application is responsible for handling the write request.

The following abbreviated example from the [wifi-connection-server](../../../examples/network/ble/wifi-connection-server) example shows how characteristic write requests are handled. The `SSID` and `password` characteristics are strings and the `control` characteristic is a numeric value. When the BLE client writes the value 1 to the `control` characteristic, the app closes the client connection and initiates a WiFi connection:

```javascript
onCharacteristicWritten(params) {
	let value = params.value;
	if ("SSID" == params.name)
		this.ssid = value;
	else if ("password" == params.name)
		this.password = value;
	else if ("control" == params.name) {
		if ((1 == value) && this.ssid) {
			this.close();
			this.connectToWiFiNetwork(this.ssid, this.password);
		}
	}
}
```

***

#### onCharacteristicRead(params)
| Argument | Type | Description |
| --- | --- | :--- | 
| `params` | `object` | Properties associated with the characteristic value read.

The `params` object contains the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `uuid` | `string` | Instance of [Bytes](#classbytes) class containing Characteristic UUID.
| `name` | `string` | Characteristic name defined in the service JSON.
| `type` | `string` | Characteristic type defined in the service JSON.
| `handle` | `number` | Characteristic handle.

The `onCharacteristicRead` callback is called when a client reads a service characteristic value on demand. The `BLEServer` application is responsible for handling the read request.

To respond to a read request corresponding to a numeric "status" characteristic:

```javascript
onReady() {
	this.status = 10;
}
onCharacteristicRead(params) {
	if ("status" == params.name)
		return this.status;
}
```

<a id="gattservices"></a>
## GATT Services
GATT services are defined by JSON files located in a BLE server project's `bleservices` directory. Each JSON file defines a single service with one or more characteristics. The JSON is automatically converted to platform-specific native code by the Moddable `mcconfig` command line tool and the compiled object code is linked to the app.

The JSON file for a service consists of an object with a single `service` property. The `service` property is an object that includes the following top-level properties:

| Property | Type | Description |
| --- | --- | :--- |
| `uuid` | `string` | Service UUID.
| `characteristics` | `object` | An object with details about the service characteristic(s). Each key corresponds to a characteristic name.

Each item in the `characteristics` object contains the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `uuid` | `string` | Characteristic UUID.
| `maxBytes` | `number` | Maximum number of bytes required to store the characteristic value.
| `type` | `string` | Optional JavaScript data value type. Supported types include `Array`, `String`, `Uint8`, `Uint16` and `Uint32`. If the `type` property is not present, the data value type defaults to `ArrayBuffer`. The `BLEServer` class automatically converts characteristic values delivered in buffers by the underlying BLE implementation to the requested `type`.
| `permissions` | `string` | Characteristic permissions. Supported permissions include `read` and `write`.
| `properties` | `string` | Characteristic properties. Supported properties include `read`, `write`, `notify` and `indicate`.
| `value` | `array`, `string`, or `number` | Optional characteristic value. The `BLEServer` class automatically converts the value specified here to the type specified by the `type` property.

Characteristics that include a `value` property are considered static. The `BLEServer` class automatically responds to read requests for static characteristic values, further reducing the script code required to host a GATT service.

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

<a id="blesecurity"></a>
## BLE Security

BLE clients and servers can optionally request link-layer security to protect data transferred between devices. Passkey pairing requires both devices to exchange and confirm the code before establishing a connection. Once the pairing process is complete, the connection and data transferred is encrypted. The BLE Security Manager (SM) defines the methods and protocols for pairing and key distribution.


<a id="classsm"></a>
## Class SM

The `SM` class provides access to BLE Security Manager features. The `SM` class is available to both `BLEClient` and `BLEServer` classes.

```javascript
import SM from "sm";
```

### Functions

#### `deleteAllBondings()`

Use the `deleteAllBondings` function to delete all bonding information and encryption keys from persistent storage:

```javascript
onReady() {
	SM.deleteAllBondings();
}
```
***

#### `securityParameters(params)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `params` | `object` | Properties associated with the security configuration.

The `params` object contains the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `encryption` | `boolean` | Optional property to enable encryption. Default is `true`
| `bonding` | `boolean` | Optional property to enable bonding. Default is `false`
| `mitm` | `boolean` | Optional property to enable man-in-the-middle (MITM) protection. Default is `false`
| `ioCapability` | `object` | Optional `IOCapability` instance to configure the device I/O capabilities. Default is `IOCapability.NoInputNoOutput`

The `IOCapability` object contains the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `NoInputNoOutput` | `number` | Device has no input or output capabilities
| `DisplayOnly` | `number` | Device has only output capability
| `KeyboardOnly` | `number` | Device has only input capability
| `KeyboardDisplay` | `number` | Device has input and output capabilities
| `DisplayYesNo` | `number` | Device has output capability and button feedback for a Yes or No response

The `securityParameters` property accessor function is used to configure the device security requirements and I/O capabilities.

To request MITM protection with encryption for a display-only device. The device will display a passkey when requested:

```javascript
import {SM, IOCapability} from "sm";

onReady() {
	SM.securityParameters = { mitm:true, ioCapability:IOCapability.DisplayOnly };
}
```

To request MITM protection with encryption and bonding for a device with both a display and keyboard. The device will request a passkey to be entered and encryption keys will be saved:

```javascript
import {SM, IOCapability} from "sm";

onReady() {
	SM.securityParameters = { mitm:true, bonding:true, ioCapability:IOCapability.KeyboardDisplay };
}
```

***

#### `onAuthenticated()`

The `onAuthenticated` callback is called when an authentication procedure completes, i.e. after successful device pairing.

```javascript
import {SM, IOCapability} from "sm";

onAuthenticated() {
	trace("authentication success\n");
}
```

***

#### `onPasskeyConfirm(params)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `params` | `object` | Properties associated with the passkey confirmation.

The `params` object contains the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `address` | `object` | `ArrayBuffer` containing peer device Bluetooth address
| `passkey` | `number` | The passkey to confirm

The `onPasskeyConfirm` callback is called when the user needs to confirm a passkey value displayed on a peer device. The callback returns `true` if the passkey was accepted.

```javascript
onPasskeyConfirm(params) {
	// display passkey on screen
	trace(`confirm passkey: ${params.passkey}\n`);
	return true;	// passkey confirmed by user
}
```
***

#### `onPasskeyDisplay(params)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `params` | `object` | Properties associated with the passkey display.

The `params` object contains the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `address` | `object` | `ArrayBuffer` containing peer device Bluetooth address
| `passkey` | `number` | The passkey to display

The `onPasskeyDisplay` callback is called when the device needs to display a passkey.

```javascript
onPasskeyDisplay(params) {
	// display passkey on screen
	trace(`display passkey: ${params.passkey}\n`);
}
```

***

#### `onPasskeyRequested(params)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `params` | `object` | Properties associated with the passkey to request.

The `params` object contains the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `address` | `object` | `ArrayBuffer` containing peer device Bluetooth address

The `onPasskeyRequested` callback is called when the device needs to request a passkey entry. The callback returns a numeric passkey value between 0 and 999,999.

```javascript
onPasskeyRequested(params) {
	// display keyboard to enter passkey
	let passkey = Math.round(Math.random() * 999999);
	return passkey;
}
```
> **Note:** Passkey values are integers, but must always include six digits. The host application is responsible for padding with leading zeros for display.

***

<a id="esp32platform"></a>
## BLE Apps on ESP32 Platform
The `mcconfig` command line tool **automatically** configures the required ESP-IDF BLE options required by the host app.
The [sdkconfig.defaults](https://github.com/Moddable-OpenSource/moddable/blob/public/build/devices/esp32/xsProj/sdkconfig.defaults) configuration file in the Moddable SDK presets the following core BLE options:

	CONFIG_BTDM_CONTROLLER_PINNED_TO_CORE=0
	CONFIG_BTDM_CONTROLLER_HCI_MODE_VHCI=y
	CONFIG_BLUEDROID_ENABLED=y
	CONFIG_BLUEDROID_PINNED_TO_CORE=0
	CONFIG_BTC_TASK_STACK_SIZE=3072
	CONFIG_BLE_SMP_ENABLE=y
	CONFIG_BT_ACL_CONNECTIONS=1
	CONFIG_SMP_ENABLE=y
	CONFIG_BT_RESERVE_DRAM=0x10000
	
When building a BLE client or server app, the `mcconfig` tool enables the ESP-IDF BLE components by setting the corresponding option:

	CONFIG_BT_ENABLED=y

When building a BLE client app, the `CONFIG_GATTC_ENABLE` option is also set:

	CONFIG_GATTC_ENABLE=y

When building a BLE server app, the `CONFIG_GATTS_ENABLE` option is also set:

	CONFIG_GATTS_ENABLE=y

>**Note:** BLE options can be further customized, if necessary, by editing the `sdkconfig.defaults` file before building the host app. For example, the `CONFIG_BT_ACL_CONNECTIONS` value can be increased to support more than one BLE client connection. This value should match the `max_connections` value in the application manifest.

Once any sdkconfig.defaults file changes have been made, build the [scanner](../../../examples/network/ble/scanner) BLE app for the ESP32 platform:

	cd $MODDABLE/examples/network/ble/scanner
	mcconfig -d -m -p esp32

<a id="geckoplatform"></a>
## BLE Apps on Blue Gecko Platform
Building and deploying BLE apps on Blue Gecko follow the same workflow outlined in our [Gecko developer documentation](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/gecko/GeckoBuild.md). For BLE apps, we recommend starting from the `soc-ibeacon` Simplicity Studio example project.

The [make.blue.mk](../../../tools/mcconfig/geck0/make.blue.mk) makefile includes variables that define the Gecko target platform, kit and part. The makefile is configured by default to build apps for the Blue Gecko [EFR32BG13P632F512GM48](https://www.silabs.com/products/wireless/bluetooth/blue-gecko-bluetooth-low-energy-socs/device.efr32bg13p632f512gm48) Bluetooth low energy chip mounted on the [BRD4104A](https://www.silabs.com/documents/login/reference-manuals/brd4104a-rm.pdf) 2.4 GHz 10 dBm Radio Board. To configure the build for a different Blue Gecko target, change the makefile `GECKO_BOARD`, `GECKO_PART`, `HWKIT` and `HWINC` variables accordingly.

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

| Name | Description |
| :---: | :--- |
| [ble-friend](../../../examples/network/ble/ble-friend)| Shows how to interact with the Adafruit BLE Friend [UART service](https://learn.adafruit.com/introducing-adafruit-ble-bluetooth-low-energy-friend/uart-service) RX and TX characteristics.
| [colorific](../../../examples/network/ble/colorific) | Randomly changes the color of a BLE bulb every 100 ms.
| [discovery](../../../examples/network/ble/discovery) | Demonstrates how to discover a specific GATT service and characteristic.
| [powermate](../../../examples/network/ble/powermate) | Receives button spin and press notifications from the [Griffin BLE Multimedia Control Knob](https://griffintechnology.com/us/powermate-bluetooth).
| [scanner](../../../examples/network/ble/scanner) | Scans for and displays peripheral advertised names.
| [security-client](../../../examples/network/ble/security-client) | Demonstrates how to implement a secure health thermometer BLE client using SMP. The `security-client` can connect to the [security-server](../../../examples/network/ble/security-server) app.
| [sensortag](../../../examples/network/ble/sensortag) | Receives sensor notifications from the [TI CC2541 SensorTag](http://www.ti.com/tool/CC2541DK-SENSOR#technicaldocuments) on-board sensors.
| [tempo](../../../examples/network/ble/tempo) | Reads temperature, humidity and barometric pressure from a [Blue Maestro Environment Monitor](https://www.bluemaestro.com/product/tempo-environment-monitor/) beacon.

### Server Apps

| Name | Description |
| :---: | :--- |
| [advertiser](../../../examples/network/ble/advertiser) | Broadcasts advertisements until a BLE client connects.
| [health-thermometer-server](../../../examples/network/ble/health-thermometer-server) | Implements the Bluetooth [Health Thermometer Service](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.health_thermometer.xml).
| [health-thermometer-server-gui](../../../examples/network/ble/health-thermometer-server-gui) | [Piu](../../../documentation/piu/piu.md) app for ESP32 that implements the Bluetooth [Health Thermometer Service](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.health_thermometer.xml).| [heart-rate-server](../../../examples/network/ble/heart-rate-server) | Implements the Bluetooth [Heart Rate Service](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.heart_rate.xml).
| [security-server](../../../examples/network/ble/security-server) | Demonstrates how to implement a secure health thermometer BLE server using SMP. The `security-server` can connect to the [security-client](../../../examples/network/ble/security-client) app.
| [uri-beacon](../../../examples/network/ble/uri-beacon) | Implements a [UriBeacon](https://github.com/google/uribeacon/tree/uribeacon-final/specification) compatible with Google's [Physical Web](https://github.com/google/physical-web) discovery service.
| [wifi-connection-server](../../../examples/network/ble/wifi-connection-server) | Deploys a BLE WiFi connection service on ESP32. The connection service allows BLE clients to connect the BLE device to a WiFi access point, by writing the SSID and password characteristics. 
