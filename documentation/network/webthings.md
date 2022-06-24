# Web Things
Copyright 2018-2019 Moddable Tech, Inc.<BR>
Revised: August 28, 2019

**IMPORTANT**: Web Thing API support in the Moddable SDK is **deprecated** as of August 28, 2019.

## Table of Contents

* [Introduction](#introduction)
	* [Implementing a Light using the WebThing class](#implementing-a-light)
	* [Announcing a Web Thing](#announcing-a-webthing)
* [class WebThings](#webthings)
* [class WebThing](#webthing)

<a id="introduction"></a>
## Introduction
The Web Thing API is part of the [Project Things](https://iot.mozilla.org) initiative by Mozilla to define open communication protocols between IoT products. The Web Thing API is a protocol built using JSON, HTTP, and mDNS. It is related to the larger [Web of Things](https://www.w3.org/WoT/) effort of W3C. The IoT team at Mozilla created a home gateway that works with IoT products that  implement the Web Thing API protocol. Mozilla also provides embedded device APIs to make it easier to create products that support the Web Thing API. All this work is in development, so it is not ready incorporate into products. 

In the goals of Mozilla's Web Thing API effort we hear echoes of Moddable's own goals of creating an open environment of IoT products that put the user in control. Consequently, at Moddable we have begun experimenting with the Web Thing API. One result of that work is a set of simple JavaScript classes for building devices compatible with the Web Thing API. The classes have been successfully used on ESP8266 and ESP32 hardware with the Mozilla gateway.

The [draft Web Thing API specification](https://iot.mozilla.org/wot/) is available from Mozilla. The Moddable classes implements a subset of the protocol. Some details:

- mDNS is always used to announce devices to the gateway
- Actions and events are not supported
- The HTTP server always uses port 80
- WebSockets are not supported

The Moddable classes implement optional publishing of selected properties of a  WebThing in the mDNS TXT record to speed notification of changes to interested devices. The classes also implement optional binding of a WebThings properties to a property published by another WebThing. The binding is performed using the `controller` property.

The Web of Things [gateway software](https://iot.mozilla.org/gateway/) is available from Mozilla. We run the gateway on a Raspberry Pi at the Moddable office.

The Moddable SDK repository on GitHub contains the [WebThings implementation](https://github.com/Moddable-OpenSource/moddable/tree/public/modules/network/webthings) and several [example device implementations](https://github.com/Moddable-OpenSource/moddable/tree/public/examples/network/webthings).

This documentation describes how to implement a `WebThing` subclass to be used with the `WebThings` host. The `WebThing` class is a base class to be subclassed by specific implementations. It contains no device specific capabilities. 

<a id="implementing-a-light"></a>
### Implementing a Light using the WebThing class

A Web Thing is a subclass of the `WebThing` class contained in the `WebThings` module.

```js
import {WebThings, WebThing} from "webthings";
```

The following code subclasses `WebThing` to define a simple `OnOffLight ` device which can be turned on and off remotely by the gateway using the Web Thing API.

```js
class OnOffLight extends WebThing {
	constructor(host) {
		super(host);
		this.state = true;
	}
	get on() {
		return this.state;
	}
	set on(state) {
		if (state == this.state) return;
		this.state = state;
		this.changed();
		// light specific implementation details here
	}
	static get description() {
		return {
			"@context": "https://iot.mozilla.org/schemas",
			"@type": ["Light", "OnOffSwitch"],
			name: "light",
			description: "On/off light",
			properties: {
				on: {
					type: "boolean",
					description: "light state",
					txt: "on"
				}
			}
		}
	}
}
```

Each `WebThing` subclass provides a device description as a class static property. It is convenient to provide this as a static getter function on the class. The device description is used by the gateway to know how to interact with the device. The device description provides the name, type, and description of the device. In addition, it contains a dictionary of properties that lists their property name, data type, and a human-readable description for display by the gateway. The simple Light has only a single property named `on`, a boolean which is true when the light is on and false when it is off. The `@context` and `@type` parameters tell the gateway this Web Thing is a simple light causing it to be represented with a lightbulb icon in the gateway user interface.

```js
static get description() {
	return {
		name: "modLight",
		"@context": "https://iot.mozilla.org/schemas",
		"@type": ["Light"],
		description: "On/off light",
		properties: {
			on: {
				type: "boolean",
				description: "light state",
				txt: "o",
			}
		}
	}
}
```

The optional `txt` property on the `on` property tells the `WebThing` class that the value of the `on` property should be included in the mDNS TXT resource record for this `WebThing` subclass. The name of the property in the TXT resource record is `o` in this example. Often TXT resource records use abbreviated names to save space. Including properties in the TXT resource record allows devices to be notified of changes in the property using mDNS multicast broadcasts.

The constructor of the `Light` class must invoke the superclass of its super-class. After that, it performs any initialization necessary. In this example, the `Light` initializes its state to false, indicating that the light is initially off when the device starts up.

```js
constructor(host) {
	super(host);
	this.state = false;
}
```

The constructor and device description are the only two predefined functions that the Web Thing must implement. Beyond that, for each property declared in the device description, the Web Thing implements a getter and setter. The `Light` has a single property named "on" so it implements a getter for "on" and a setter for "on". If it was a read-only property, only the getter would be implemented. In the following example, the on setter changes the state of the built-in LED on an ESP8266 or ESP32 to reflect the state of the light property.

```js
get on() {
	return this.state;
}
set on(state) {
	this.state = state;
	Digital.write(2, state ? 0 : 1);
	this.changed();
}
```

The call to `this.changed()` tells the WebThing base class that the property value has been modified. The call triggers an mDNS update to the TXT resource record, such as the `on` property in the example device description above.

<a id="announcing-a-webthing"></a>
### Announcing a Web Thing

Once a subclass of `WebThing`, such as the `Light` above, has been defined it needs to be instantiated. The `WebThings` class instantiates and hosts one or more `WebThing` instances.

The `WebThings` class uses mDNS to announce the Web Thing on the local network. The gateway will discover the Web Thing from this announcement. mDNS is a service which may be used by several modules on a device. Therefore, The mDNS instance is passed to the `WebThings` constructor rather than being instantiated by the `WebThings` class.

```js
const mdns = new MDNS({hostName: "my light"}, (message, value) => {
	if (1 == message) && value) {
		const things = new WebThings(mdns);
		things.add(Light);
	}
}
```

The mDNS name claiming process must be completed before the `WebThings` host can be instantiated. The code above waits for the claiming process to complete successfully, then instantiates the `WebThings` host, and finally registers the `Light` by passing its constructor.

***

<a id="webthings"></a>
## class WebThings
The `WebThings` class is a host for one or more `WebThing` devices. It provides the mDNS and HTTP services used for the device and gateway to communicate.

```js
import {WebThings} from "webthings";
```

### `constructor(mdns)`
The `WebThings` constructor takes an mDNS instance as an argument. This is a required parameter. The mDNS instance must have completed the claiming process before being passed to the constructor.

```js
let mdns = new MDNS( /* mDNS parameters here */ );
let things = new WebThings(mdns);
```

***


### `add(constructor [, ...args])`
The `add` function registers a subclass of `WebThing`. The constructor argument is a constructor of the `WebThing` subclass to be added. It will be instantiated with `new` by the `WebThings` host. Any additional arguments passed to `add` are passed to the `WebThing` constructor following the `host` argument. This can be useful when initializing a `WebThing` subclass.

In this example, `OnOffLight` is the class defined in the "Implementing a Light using the WebThing class" section above.

```js
things.add(OnOffLight);	
```

***

<a id="webthing"></a>
## class WebThing
The `WebThing` class is subclassed to create implementations of specific Web Things. 

```js
import {WebThing} from "webthings";
```

The example code in the sections below refer to the `OnOffLight` class defined in the "Implementing a Light using the WebThing class" section above.

### `constructor(host [, ...args])`
The constructor of a `WebThing` is invoked by the `WebThings` host when the `WebThing` subclass is added to the host. It must call the constructor of its super class with the `host` parameter. Any additional arguments passed to the `add` function of the `WebThings` class are passed to this constructor as `...args`.

```js
let light = new OnOffLight(this, ...args);
```

***


### `static get description()`
The static getter for the description property returns the device description. The device description is defined in the draft Web Thing API specification from Mozilla. Its use here omits the `href` properties as they are automatically assigned by the `WebThings` host.

```js
let description = OnOffLight.description;
```

***


### `get property()`
When a request is received to retrieve the value of a `WebThing` property, the corresponding getter is invoked on the `WebThing` instance. For example, if a thermostat device has a "temperature" property, the "get temperature()" getter is called.

```js
let on = light.on;
```

***


### `set property()`
When a request is received to set the value of a `WebThing` property, the corresponding setter is invoked on the `WebThing` instance. For example, if a thermostat device has a "temperature" property, the "set temperature()" setter is called.

```js
light.on = true;
```

***

### `changed()`
A WebThing subclass calls `changed` to indicate that one or more property values been modified. It is usually called from a setter. Properties published by including the optional `txt` property in the device description are updated only after changed is called.

```js
class OnOffLight extends WebThing {
	...
	set on(state) {
    	this.state = state;
    	this.changed();
	}
	...
}
```

***
