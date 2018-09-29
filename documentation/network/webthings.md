# WebThings
Copyright 2018 Moddable Tech, Inc.

Revised: September 29, 2018

Warning: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## Introduction
WebThings is an initiative by W3C to define open communication protocols between IoT products. The IoT team at Mozilla created a home gateway that communicates using the WebThings protocol. They have also provided embedded device APIs to make it easier to create WebThings compatible products. All this work is in development, so it is not ready incorporate into products. 

In the goals of the WebThings effort we hear echoes of Moddable's own goals of creating an open environment of IoT products that put the user in control. Consequently, at Moddable have begun experimenting with WebThings. One result of that work is a simple JavaScript WebThings API for building WebThings. It has been successfully used on ESP8266 and ESP32 hardware with the Mozilla gateway.

The [draft WebThings specification](https://iot.mozilla.org/wot/) is available from Mozilla. The Moddable WebThings API implements a subset of the protocol. Some details:

- mDNS is always used to announce the WebThing to the gateway
- Actions and events are not support 
- The HTTP server always uses port 80
- WebSockets are not supported

The WebThings [gateway software](https://iot.mozilla.org/gateway/) is available from Mozilla. We run the gateway at our office on a Raspberry Pi.

The Moddable SDK repository on GitHub contains the [WebThings implementation](https://github.com/Moddable-OpenSource/moddable/tree/public/modules/network/webthings) and several [example device implementation](https://github.com/Moddable-OpenSource/moddable/tree/public/examples/network/webthings).

This documentation describes how to implement a WebThing subclass to be used with the WebThings host. The WebThing class is an base class to be subclassed by specific implementations. It contains no device specific capabilities. 

### Implementing a WebThing

A `WebThing` is a subclass of the WebThing class contained in the WebThings module.

	import {WebThings, WebThing} from "webthings";

The follow code subclasses WebThing to define a simple Light device which can be  turned on and off remotely using the WebThing protocol.

	class Light extends WebThing {
		// ...implementation goes here
	}

Each WebThing subclass provides a device description as a class static property. It is convenient to provide this as a static getter function on the class. The device description is used by the gateway to define its interactions with the device. The device description provides the name, type, and description of the device. In addition, it includes a dictionary of properties, including their data type and a human-readable description for display by the gateway. The simply Light has only a single property, it is named "on" and it is a boolean which is true when the light is on and false when it is off. The device type "onOffLight" is understood by the gateway to mean this is a simple light, and it will show it in its user interface as a lightbulb.

	static get description() {
		return {
			name: "modLight",
			type: "onOffLight",
			description: "On/off light",
			properties: {
				on: {
					type: "boolean",
					description: "light state",
				}
			}
		}
	}

The constructor of the `Light` class must invoke the superclass. After that, it may perform any initialization necessary. In this example, the `Light` initializes its state to false, indicating that the light is initially off when the device starts-up.

	constructor(host) {
		super(host);
		this.state = false;
	}

The constructor and device description are the only two predefined functions that the WebThing must implement. Beyond that, for each property declared in the device description, the WebThing implements a getter and setter. The Light has a single property named "on" so it implements a getter for "on" and a setter for "on". If it was a read-only property, only the getter would be implemented. In the following example, the on setter changes the state of the built-in LED on an ESP8266 or ESP32 to reflect the state of the light property.

	get on() {
		return this.state;
	}
	set on(state) {
		this.state = state;
		Digital.write(2, state ? 0 : 1);
	}

### Announcing a WebThing

Once a subclass of `WebThing`, such as the `Light` above, has been defined it needs to be instantiated. The `WebThings` class instantiates and hosts one or more WebThing instances.

The WebThings class uses mDNS to announce the WebThing device on the local network. The gateway will discover the WebThing from this announcement. mDNS is a service which may be used by several modules on a device. Therefore, The mDNS instance is passed to the WebThings constructor rather than being instantiated by the WebThings class.


	const mdns = new MDNS({hostName: "my light"}, (message, value) => {
		if (1 == message) && value) {
			const things = new WebThings(mdns);
			things.add(Light);
		}
	}

The mDNS name claiming process must be completed before the `WebThings` host can be instantiated. The code above waits for the claiming process to complete successfully, then instantiates the `WebThings` host, and finally registers the `Light` WebThing by passing its constructor.

## class WebThings
The `WebThings` class is a host for one or more `WebThing` devices. It provides the mDNS and HTTP services used for the device and gateway to commnicate.

	import {WebThings, WebThing} from "webthings";

### constructor(mdns)
The `WebThings` constructor takes an mDNS instance as an argument. This is a required parameter. The mDNS instance must have completed the claiming process before being passed to the constructor.

### add(constructor [, ...args])
The `add` function registers a subclass of `WebThing`. The constructor argument is a constructor of the WebThing subclass to be added. It will be instantiated with `new` by the WebThings host. Any additional arguments passed to `add` are passed to the WebThing constructor following the `host` argument. This can be useful when initializing a WebThing subclass.

## class WebThing
The `WebThing` class is subclassed to create implementations of specific WebThing devices. 

	import {WebThings, WebThing} from "webthings";

### constructor(host [, ...args])
The constructor of a `WebThing` is invoked by the WebThings host when the WebThing subclass is added to the host. It must call its super class constructor with the `host` parameter. Any additional arguments passed to the WebThings `add` function are passed to this constructor as `...args`.

### static get description()
The static getter for the description property returns the device description. The device description is defined in the draft WebThings specification from Mozilla. Its use here omits the `href` properties as they are automatically assigned by the `WebThings` host.

### get property()
When a request is received to retrieve the value of a WebThing property, the corresponding getter is invoked on the WebThing instance. For example, if a thermostat device has a "temperature" property, the "get temperature()" getter is called.

### set property()
When a request is received to set the value of a WebThing property, the corresponding setter is invoked on the WebThing instance. For example, if a thermostat device has a "temperature" property, the "set temperature()" setter is called.


<!-- ### changed() -->
