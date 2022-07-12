# Exploring TC53 IO using Firmata
Copyright 2019 Moddable Tech, Inc.<BR>
Author: Peter Hoddie<BR>
Revised: August 4, 2019

The Moddable SDK contains an implementation of the Firmata protocol for communicating hardware control information between two devices, often a computer and  microcontroller. The implementation uses a new set of IO classes based on an active proposal to [Ecma TC53](https://www.ecma-international.org/memento/tc53.htm). The IO classes are based on a common IO pattern designed to provide access to hardware resources through a simple API that provides consistent behavior across a range of IO types while allowing efficient implementation on the most constrained hardware configurations capable of executing modern JavaScript.

The primary goal of the implementation is to explore using the new IO class. The Firmata protocol is an interesting test case as it exercises many different kinds of IO. The Firmata protocol has been invaluable in this regard. The resulting Firmata implementation may also be useful more broadly. As the TC53 IO class becomes available on other hardware platforms, this Firmata implementation should run more-or-less as-is there, perhaps providing Firmata support on additional hardware hosts.

> **Note**: The IO Class implementation is available only for the ESP8266 microcontroller which was selected as the initial target because it is representative of resource constrained devices, widely available, well-understood at Moddable, and inexpensive. 

The Firmata protocol is effectively a client/server protocol with the microcontroller taking the role of the server, and the device controller, often a computer, taking the role of the client. In the Firmata ecosystem, [Firmata.js](https://github.com/firmata/firmata.js#firmatajs) is a popular client, providing a [JavaScript API](https://github.com/firmata/firmata.js/tree/master/packages/firmata.js#firmata-prototype-api) in the Node.js environment to communicate with Firmata servers. There are [many other](https://github.com/firmata/arduino#firmata-client-libraries) Firmata clients available for various languages and environments. There are many fewer implementations of the Firmata server, with the [Standard Firmata Adruino implementation](https://github.com/firmata/arduino#firmata) being the most commonly used.

The implementation of Firmata in the Moddable SDK includes both a client and server. Each is implemented in JavaScript using the proposed TC53 IO class APIs. In the case of the client, it presents Firmata as a TC53 IO class provider. 

Implementing both the client and server allowed more uses of the IO class to be explored. The server implementation was developed against the Firmata.js client; the client implementation, against the server. The Firmata.js and Arduino Firmata server code were consulted to supplement the information in the protocol documentation. 

> **Note:** To build and run the examples in this document, you must have already installed the Moddable SDK and followed the instructions to set up the build tools for the ESP8266 target from the [Getting Started document](../Moddable%20SDK%20-%20Getting%20Started.md).

## Table of Contents

* [Firmata Background](#firmata-background)
* [Firmata Server](#firmata-server)
	* [Example Code](#server-example-code)
	* [Communicating with the Firmata Server Over TCP](#server-tcp)
	* [Implementation Notes](#server-implementation-notes)
* [Firmata Client](#firmata-client)
	* [Example Code](#client-example-code)
	* [Implementation Notes](#client-implementation-notes)
* [Firmata Graphics using Poco](#firmata-graphics-using-poco)
	* [Example Code](#poco-example-code)
	* [Implementation Notes](#poco-implementation-notes)
* [Conclusion](#conclusion)

<a id="firmata-background"></a>
## Firmata Background
Firmata is a [communication protocol](https://github.com/firmata/protocol#firmata-protocol-documentation) to interact with low-level hardware resources. These resources include digital inputs, digital outputs, I2C peripherals, serial ports, and analog inputs among many others. Serial is the most common transport for Firmata, for example between an Arduino and a computer, but the protocol can be carried over any bi-directional connection such as a TCP network connection.

The default baud rate for Firmata over serial is 57600, which is relatively slow. For comparison, the Moddable SDK runs the `xsbug` debugging protocol over serial at 921600 baud (16 times faster) on an ESP8266. Because the Firmata protocol is derived from the MIDI protocol, it is compact with many messages requiring only a few bytes. Consequently, the baud rate is not usually a limiting factor.

The Firmata protocol has some implicit assumptions which shape any implementation:

- **Reliable transport**: It assumes a reliable transport as there are no provisions for checksums, acknowledgement, or retransmit in the protocol. The basic form of the Firmata protocol was derived from the MIDI protocol, though the meaning of the messages is different.

- **Near real-time**:  It assumes that the connection is near-real time as there are no time stamps in the protocol. When operating over a direct physical connection such as serial, the latency is small and constant. When communicating over Wi-Fi, however, the latency is larger and unpredictable.

- **Single client**: It assumes that a server is connected to no more than one client as the protocol has no provision to report that a pin resource is unavailable because it is already in use.

Because of the limited testing, it is possible that there are implementation errors based on oversights or misunderstandings. Prior to implementing Firmata, the Moddable team had no experience working with Firmata.

<a id="firmata-server"></a>
## Firmata Server
To build and run the Firmata server, execute the following commands:

	cd $MODDABLE/examples/io/firmata/server
	mcconfig -m -p esp

This builds and deploys the Firmata server to an ESP8266. Note that this a release build, not a debug build. This is because Firmata uses the serial port for communication, which precludes it from being by the xsbug debugger to communicate with the ESP8266. 

From here, the Firmata.js server may be used as usual with Firmata clients. Note that the Firmata server does not announce itself over serial so it is necessary to wait about five seconds for Firmata.js to issue a probe request before the connection is fully established.

The Firmata server implementation supports the following standard pin types: 

- digital input
- digital input pull-up
- I2C
- analog input
- serial

The pins that are available depend on the configuration. For example, only a single serial port is available in the implementation. If that port is being used for Firmata transport, it is unavailable for use by a Firmata client. 

The Firmata Server was tested primarily on a [Moddable One](https://www.moddable.com/moddable-one.php), which combines an ESP8266 with a high quality IPS display and capacitive touch screen. 

<a id="server-example-code"></a>
### Example code

The following Firmata.js code fragments are useful when working with the Moddable One hardware. The `repl` included in the Firmata.js repository is a useful place to start exploring.

#### Turn Built-In LED On and Off

````js
board.pinMode(2, 1)
board.digitalWrite(2, 1)	// off
board.digitalWrite(2, 0)	// on
````
#### Monitor Flash Button

````js
board.pinMode(0, 0)
board.digitalRead(0, v => console.log(v))
````

#### Disable Flash Button Monitor

````js
board.reportDigitalPin(0, 0);
````

#### Monitor Analog Input

````js
board.analogRead(0, v => console.log(v))
````

#### Disable Analog Input Monitor

````js
board.reportAnalogPin(0, 0)
````

#### Write Bytes to Serial Port

````js
board.serialConfig({portId: 0, baud: 921600})
board.serialWrite(0, [64, 65, 66, 67]);
````

#### Monitor Incoming Bytes on Serial Port

````js
board.serialConfig({portId: 0, baud: 921600})
board.serialRead(0, bytes =>
	console.log(bytes.map(c => String.fromCharCode(c)).join("")))
````

#### Disable Serial Port Monitor

````js
board.serialStop(0)
````

#### Monitor Touch Screen for Number of Touch Points (I2C)

````js
void board.i2cConfig()
void board.i2cRead(0x38, 2, 1, v => console.log(v))
````

#### Disable I2C Monitoring

````js
void board.i2cStop(0x38)
````

<a id="server-tcp"></a>
### Communicating with the Firmata Server Over TCP
The Firmata server implements communication over a TCP network connection, in addition to serial. The Firmata server can operate in TCP in two different ways. One way is by initiating a TCP connection to a Firmata client, in which case (confusingly) the Firmata server is acting as a TCP client while the Firmata client is acting as a TCP server. This is the more common way. Alternatively, it can listen for an incoming TCP request from a Firmata client, in which case the Firmata server is a TCP server and the Firmata client is a TCP client. 

#### Using `FirmataTCPClient`
To configure the Firmata server to initiate a TCP connection to a Firmata client, do the following:

1. Modify the source code in the `main.js` Firmata server file to connect to the Firmata client.

	````js
	// new FirmataSerial;
	new FirmataTCPClient({address: "192.168.1.19"});
	````

	Firmata over TCP uses port 3030 by default. To connect to a different port, include a `port` property when calling `FirmataTCPClient`:

	````js
	new FirmataTCPClient({address: "192.168.1.19", port: 3029});
	````

2. Build the Firmata server. Be sure to provide the Wi-Fi access point credentials:

	```
	mcconfig -d -m -p esp ssid="Moddable" password="secret"
	```

The Firmata.js client supports TCP communication using the [Etherport](https://github.com/rwaldron/etherport#etherport) module. While Firmata.js repl does not support Etherport, it is [easy to add](https://gist.github.com/phoddie/17601031d83602f688c20c98292c622e). If you are using that version of the `repl`, just enter `etherport` in place of the serial port ID to launch Firmata.js with a TCP listener waiting for incoming connections.

#### Using `FirmataTCPServer`
To configure the Firmata server to wait for an incoming TCP connection from a Firmata client, do the following:

1. Modify the source code the main.js Firmata server file to connect to the Firmata client.

	````js
	// new FirmataSerial;
	new FirmataTCPServer;
	````

	Firmata over TCP uses port 3030 by default. To listen on a different port, include a `port` property when calling `FirmataTCPServer `:

	````js
	new FirmataTCPServer({port: 3029});
	````

2. Build the Firmata server. Be sure to provide the Wi-Fi access point credentials:

	```
	mcconfig -d -m -p esp ssid="Moddable" password="secret"
	```

The `FirmataTCPServer` class allows only a single connection at a time. If there is an active connection, incoming connection requests are refused. Once the active connection is closed, the next incoming connection request is accepted.

The `FirmataTCPServer` was tested using the `FirmataTCPClient` as Firmata.js does not yet support connecting to a Firmata Server waiting for an incoming connection.

<a id="server-implementation-notes"></a>

### Implementation Notes
The Moddable implementation of the Firmata server reports the following:

- **Protocol, major version**: 2
- **Protocol, minor version**: 6
- **Firmware, major version**: 2
- **Firmware, minor version**: 10
- **Firmware name**: moddable

Digital reports are driven from the interrupt that detects digital input changes, not by polling.

The ESP8266 has one digital pin, GPIO 16, that is connected to a different hardware unit than all the others. The difference is because this pin has a special use for waking the microcontroller from deep sleep. The server implementation does not support digital reports on this pin.

The I2C bus on Moddable One uses pin 5 for data and pin 4 for a clock.

The primary serial connection on ESP8266 uses pin 1 for TX and pin 3 for RX.

The Moddable Firmata Server implementation combines a general purpose Firmata server with specific knowledge of the ESP2866 pin configuration. The ESP8266 knowledge is largely isolated and should eventually migrate to a separate file to ease supporting additional microcontrollers. 

The Firmata Server implements the optional `STRING_DATA` message with the `doSendString` function which allows the server implementation to send short text strings to the client. These messages have no meaning defined in the protocol. They proved useful for debugging to generate a simple console trace from the server to the client, which is not possible when communicating with Firmata over serial which makes the xsbug debugging connection unavailable. To output these messages to the console using Firmata.js, add this line:

```js
board.on("string", msg => console.log(`Board message: ${msg}`));
```

<a id="firmata-client"></a>
## Firmata Client
The Firmata Client example works by establishing a connection to a Firmata server at start-up (the Firmata Client acts as a TCP client). The IP address of the server is defined in the code. Modify the following line in the `main.js` file of the client example to match the IP address of the server being used.

```js
const ServerAddress = "10.0.1.36";
```

To build and run the Firmata client, execute the following commands:

	cd $MODDABLE/examples/io/firmata/client
	mcconfig -d -m -p esp ssid="Moddable" password="secret"

This  builds and deploys the Firmata client to an ESP8266. Note that unlike the server, this a debug build, as the examples will only use Firmata over TCP, not serial. 

The Client API is the proposed TC53 IO provider class. An IO provider is an object that gives access to one or more kinds of IO. The IO it provides access to may be remote, as is the case with the Firmata use described here, or local, for example an GPIO expander connected over I2C.

To use the Firmata provider, the first step is to import the class. The example uses a TCP client, so it imports `FirmataClientTCP`.

```js
import {FirmataClientTCP} from "firmataclient";
```

The `FirmataClientTCP` provider is then instantiated. There are two parts to the instantiation: the configuration and the callbacks. The configuration tells the provider how to connect to the hardware resource. In this case, the configuration is the IP address.

```js
const firmata = new FirmataClientTCP({
	address: ServerAddress,
	onReady() {
		...
	},
}
```

The constructor argument accepts optional properties, including `port` which indicates the remote port to connect to if not using the default port 3030. The polling interval, in milliseconds, used by Firmata to report I2C and analog pin data is configured with the optional `interval` property.

```js
const firmata = new FirmataClientTCP({
	address: ServerAddress,
	port: 3029,
	interval: 100,
	onReady() {
		...
	},
}
```

The configuration and callbacks of a provider are set when the constructor returns and may not be modified afterwards. The `FirmataClientTCP` has a single callback, `onReady`, which is invoked when the connection is established. Adding an `onError` callback to invoke when the connection is dropped is a future work item.

The provider implementation may not know what IO is available until it establishes a connection with the remote device. This is the case in the Firmata protocol, where an initial set of messages are exchanged (e.g. `CAPABILITY_QUERY`, `ANALOG_MAPPING_QUERY`, `REPORT_FIRMWARE`, etc.) for the client to learn the capabilities of the server. When `onReady` is invoked, this process is complete. The provider instance contains one or more constructors which the script uses to access the hardware resources of the provider.

<a id="client-example-code"></a>
### Example Code

The following code samples show how to use the IO constructors provided by the `FirmataClientTCP` client. They must execute inside the `onReady` callback or afterwards. If executed before that time, the examples will fail as the constructors are not yet available. These examples assume the provider is connected to an ESP8266; in the case of the I2C examples, it assumes a Moddable One.

#### Blink Remote LED

```js
let led = new firmata.Digital({
	pin: 2,
	mode: firmata.Digital.Output,
});
	
let value = 0;
System.setInterval(() => {
	led.write(value);
	value ^= 1;
}, 500);
```

#### Monitor Remote Flash Button
	
```js
let remoteButton = new firmata.Digital({
	pin: 0,
	mode: firmata.Digital.Input,
	onReadable() {
		trace(`Remote Button: ${this.read()}\n`);
	}
});
```

Having an `onReadable` callback is optional in this example because the `read` call always returns the most recent value available to the provider. For example, a script that already has a polling loop might choose to simply read the value directly at that time rather than using the `onReadable` callback.

#### Monitor Remote Analog Pin

```js
let analog = new firmata.Analog({
	pin: 17,
	onReadable() {
		const value = this.read();
		trace(`Analog: ${value}\n`);
		if (value >= 1023)
			this.close();
	}
});
```

When this example receives a peak reading, it closes the analog monitor. To test this code on a Moddable One, place your finger near or on the analog pin trace to see the range of values.

#### Monitor Remote Digital Bank
In Firmata, pins are organized into ports, which are groups of eight pins.  The TC53 IO class proposal uses the term `bank` for what Firmata calls a `port`. This example  creates a single monitor for pins Digital pins 12, 13, 14, 15 which are the top four pins of bank 1.

```js
let remoteBank = new firmata.DigitalBank({
	pins: 0xF0,
	bank: 1,
	mode: firmata.Digital.Input,
	onReadable() {
		trace(`Change: ${this.read().toString(2)}\n`);
	}
});
```

#### I2C
I2C is a bit more complicated than Analog and Digital pins as it is a transaction-based hardware protocol: requests are made to the hardware to read and write bytes. The provider is fully asynchronous. This is not a problem for write operations as the Firmata protocol assumes reliable delivery, so the write request will eventually be delivered to the write pins. 

```js
let i2c = new firmata.I2C({
	address: 0x38,
}
i2c.write(Uint8Array.of(3, 11, 5));
```

For an I2C read, it is necessary to indicate how many bytes to read when making the read call. To address this situation within the IO Class API, the read is issued as usual, with the requested number of bytes. The `onReadable` callback is invoked when the bytes read are available. Invoking the `read` call without any parameters returns the result of the read. If multiple `read` calls are issued, their results are returned in the same order requested.

The following example works with the capacitive touch controller on Moddable One. It reads the touch count register, which indicates whether 0, 1, or 2 fingers are currently detected by the touch sensor.

```js
let touchController = new firmata.I2C({
	address: 0x38,
	onReadable() {
		const result = new Uint8Array(this.read());
		trace(`${result[0]} touch points\n`);
	}
});
touchController.write(Uint8Array.of(2));
touchController.read(1);
```

<a id="client-implementation-notes"></a>
### Implementation Notes
The Firmata Client provides the start of the `FirmataClientSerial` class. This has not been tested. It requires the addition of code to manage the initial handshake to work with the Moddable Firmata Server.

The implementation does not yet support serial.

<a id="firmata-graphics-using-poco"></a>
## Firmata Graphics using Poco
The Moddable One hardware has an integrated touch screen. To allow Firmata client code to draw to the display, a simple graphics protocol has been added to the Firmata protocol. The graphics implementation uses the [Poco graphics engine](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/commodetto/poco.md) from the [Commodetto graphics library](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/commodetto/commodetto.md) in the Moddable SDK. The integration is a small subset of the full capabilities, more or less the ability to fill rectangles. However, the approach is designed to support additional drawing functionality in the future.

It is theoretically possible to render to the Moddable One display using the SPI pins with Firmata. However, there is no good reason to replace the optimized native display driver with a comparatively slow data transfer over over Firmata. Further, the large amount of data required to deliver rendered graphics is not what Firmata is best at. Instead, the implementation marshals Poco drawing commands over the Firmata protocol, with the rendering occurring efficiently on the Firmata server.

<a id="poco-example-code"></a>
### Example Code
The following examples show how to use Poco graphics with the Firmata Client when communicating to the Firmata Server. The Firmata Server must have been built with display driver support to use Poco. For Moddable One, build the Firmata Server with the `esp/moddable_one` target:

	cd $MODDABLE/examples/io/firmata/server
	mcconfig -m -p esp/moddable_one ssid="Moddable" password="secret"

#### Connect and Erase Screen
The following example creates a Firmata provider which connects to a Moddable One. When the connection is established, the remote display is erased to blue.

````js
new FirmataClientTCP({
	address: "10.0.1.22",
	onReady() {
		const poco = new this.Poco;
		const blue = poco.makeColor(0, 0, 255);
		poco.begin();
			poco.fillRectangle(blue, 0, 0, poco.width, poco.height);
		poco.end();
	}
}
````

The instance created by calling the `this.Poco` constructor presents a subset of the full Poco JavaScript API. The code to erase the local display on a Moddable One is identical, beyond the constructor:

````js
const poco = new Poco(screen);
const blue = poco.makeColor(0, 0, 255);
poco.begin();
	poco.fillRectangle(blue, 0, 0, poco.width, poco.height);
poco.end();
````

#### Remote Button with Display
The following example reads the remote button and updates a rectangle on the screen to be red when the button is pressed and gray otherwise.

```js
new FirmataClientTCP({
	address: "10.0.1.22",
	onReady() {
		const poco = new this.Poco;
		let gray = poco.makeColor(128, 128, 128);
		let red = poco.makeColor(255, 0, 0);
		let remoteButton = new this.Digital({
			pin: 0,
			mode: this.Digital.Input,
			onReadable() {
				const size = 80;
				poco.begin((poco.width - size) >> 1,
								(poco.height >> 2) - (size >> 1), size, size);
					poco.fillRectangle(this.read() ? gray : red,
								0, 0, poco.width, poco.height);
				poco.end();
			}
		});
	}
}
```

<a id="poco-implementation-notes"></a>
### Implementation Notes
The Firmata support for Poco implements the following calls:

- `constructor`
- `begin`
- `end`
- `makeColor`
- `fillRectangle`
- `drawPixel`
- `clip`
- `origin`

Colors are transmitted from client to server using seven bits per channel (R, G, and B are each 7 bits). This is done as Firmata is designed to carry seven bit data, and many embedded displays use only 5 or 6 bits of color channel information.

The Firmata Server informs the client that the Poco protocol extension is supported by sending a `STRING_DATA` message with the value `hasPoco` following the response to the `CAPABILITY_QUERY` message. A client which is unaware of the feature will ignore the message. This is a temporary ad-hoc approach.

All Poco messages contained within the Sysex message `User Command 1` to avoid conflicting with any existing or proposed extension to Firmata. 

<a id="conclusion"></a>
## Conclusion
This project began as an experiment to implement a small Firmata server to exercise the ESP8266 implementation of the TC53 proposed IO classes. It succeeded in that goal, providing an excellent test bed to shake out bugs in the implementation and overlooked details in the design. The implementation is efficient. Its size and complexity seem appropriate to the task it performs. The code is focused on translating between Firmata and the IO class API, not in wrestling with the IO class to get the desired behavior. There is a reasonably strong correspondence between the capabilities Firmata requires the APIs the IO Class provides.

The Firmata client implementation began as a way to test `FirmataTCPServer`. Once the basics of that worked, it made sense to build it out using the TC53 IO Class provider model. As the first asynchronous provider implementation, it helped to formalize some of the ideas around how the IO class design should adapt to these scenarios.

Implementing the Firmata Server and Client in JavaScript is relatively straightforward because of the many capabilities built into the language itself. The resulting implementation is quite stable for code that is still quite new.

The Poco extension to Firmata is an experiment, which became possible once the Server and Client were working together. While drawing rectangles isn't quite enough functionality to be very useful, it is easy to using remote graphics over Firmata with a few more functions added. For now, it is a great way to build some visual Firmata examples and explore interactive scenarios involving a display.

Overall, this Firmata experiment has shown that the proposed IO Class is quite usable for implementing a very different IO model. That is significant as a key design point of the TC53 proposal is to define a low level IO Class that can be used as the foundation to build higher level frameworks that are focused on a focused use or market segment. It is exciting to glimpse a world where a such frameworks, as embodied by here by Firmata, are supported out-of-the-box by new hardware releases as they launch with support for TC53 APIs, such as the IO Class.
