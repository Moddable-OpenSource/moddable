# TC53 IO: A New Take on JavaScript Input/Output on Microcontrollers 
Author: Peter Hoddie<BR>
Updated: April 7, 2021<BR>
Copyright 2019-2021 Moddable Tech, Inc.

This document introduces work on Input/Output (IO) under development by Ecma TC53 in the IO Class Pattern proposal and describes an implementation of the proposal that uses the XS JavaScript engine on the ESP8266 microcontroller. (The implementation also supports ESP32, but this document is focused on ESP8266.)

Ecma TC53 is a standards committee with a charter to define ECMAScript Modules for embedded systems. Its goal is to define standard JavaScript APIs for developing software for resource-constrained embedded hardware. This is analogous to the work of W3C and WHATWG to define JavaScript APIs for developing software for the web. The APIs defined by TC53 are intended to be vendor-neutral and, consequently, independent of the host operating system, CPU architecture, and JavaScript engine. IO was selected as the first area of work by TC53 because it is fundamental to nearly all uses of embedded systems. For example, IO is a precondition to implementing support for both sensors and communication.

A key characteristic of the initial TC53 work, including the work on IO, is that they are low-level APIs. They can be considered the minimal host for a JavaScript runtime. They will be used to build higher-level APIs, including frameworks for specific types of products, markets, and programming styles. The APIs are similar to drivers, where a small, simple API is important to achieving reliability. The APIs are designed to be straightforward to use in JavaScript. They have an additional design goal which is less common: they are intended to be straightforward to implement in native code. As these APIs make up a porting layer, they need to be clear to the implementors of the porting layer. They need to be small enough that the porting task is not an overwhelming amount of work. They need to be simple enough in their use of JavaScript that an embedded C developer can create an efficient port without first becoming an expert in the JavaScript language.

The IO Class Pattern proposal appears to balance these requirements well. The design takes inspiration from a range of JavaScript projects including [Johnny Five](http://johnny-five.io/), [Firmata](http://firmata.org/wiki/Main_Page), [Node.js](https://nodejs.org/en/), and the [Moddable SDK](https://github.com/Moddable-OpenSource/moddable), among others. These have provided ideas grounded in real-world experience about how to interact with various kinds of IO in JavaScript. The effort to implement the APIs proved to be very manageable, with a result that operates efficiently in terms of both CPU utilization, latency, and memory use.

The basic definition of the IO Class Pattern has been in place since mid-2019, with refinements to the design settling into place. The [current draft](https://ecmatc53.github.io/spec/web/spec.html) of the proposed specification is now available. To better understand the design, an implementation effort was undertaken. The ESP8266 microcontroller was selected as a testbed because it is supported by the XS JavaScript engine and its hardware resources are on the low end of the devices that currently support modern JavaScript. Further, its low cost and wide availability make it feasible for many developers to experiment with and contribute to the effort.

The implementation itself tries to be "bare metal" as much as feasible. The digital and serial IO is implemented by directly manipulating hardware registers, for example. This approach was taken to explore what a truly focused port of the IO Class looks like. To ease porting, future work may include a native porting layer.

The remainder of this document describes the IO Class Pattern and how it is applied to each IO type in the ESP8266 implementation.

## The Basic IO Pattern
The IO Class Pattern design starts with the idea that the majority of IO operations on a microcontroller are described by four basic operations:

1. **Create** -- Establish and configure a connection to an IO resource. 
1. **Read** -- Get data from the IO resource.
1. **Write** -- Send data to the IO resource.
1. **Close** -- Release the IO resource.

Of course, not every kind of IO uses all four operations. An analog input does not use write. A digital output does not use read. These differences are why this is called a "Class Pattern" and not simply a "Class". Each IO type defines how it uses the pattern. The analog input, for example, defines that the write operation is not supported.

The IO Class Pattern adopts the long-standing JavaScript convention that all operations are non-blocking. This behavior is particularly important on resource-constrained devices as there may be insufficient resources available to create even a single parallel execution context for a blocking operation. This is not to say that all operations complete instantaneously, but that they complete quickly enough that they do not inhibit the ability of the system to respond to incoming events in a timely manner.

### Create Operation
The create operation is performed by the constructor of an IO class. The constructor takes a single argument, an object that contains properties to configure the IO. This is sometimes called an *options object*. For example, the following creates an instance of an analog input bound to pin 16.

```js
let analogIn = new Analog({
	pin: 16,
});
```

The IO configuration depends on the host hardware. The API must accommodate the significant variation present in hardware capabilities. For example, some hardware supports configuring the number of bits of resolution an analog input provides. Such a host might add a `resolution` property to the IO configuration for this purpose.

```js
let analogIn = new Analog({
	pin: 16,
	resolution: 12,
});
```

The IO class may provide notification of certain events, for example when new data is available to read. When a notification is available, it invokes a callback function. This example creates a Serial IO instance with an `onReadable` callback function that is invoked when the serial instance has new data available to read.

```js
let serial = new Serial({
	baud: 57600,
	onReadable() {
		trace("serial input available\n");
	}
})
```

This way of providing the callback as a property of the configuration is often convenient. The same approach is used by streams in Node.js as described in [Simplified Construction](https://nodejs.org/api/stream.html#stream_simplified_construction). 

Each IO implementation defines the notifications it supports. The IO Class Pattern proposal defines four notifications:

- `onReadable` -- New data is available to read.
- `onWritable`-- The output buffer is able to accept more data.
- `onError` -- A problem occurred.
- `onReady` -- The instance is initialized and ready for use.

The callbacks are usually optional. There is often a runtime cost associated with each callback. Therefore, it is recommended that scripts only install the callbacks they require. That said, the use of callbacks is generally preferred to polling.

Once the create operation completes, the configuration of the IO instance is locked. This helps simplify the API and its implementation. It also simplifies the work to secure IO with [Secure ECMAScript](https://blog.moddable.com/blog/secureprivate/) (a topic detailed elsewhere). In the rare situation where it needs to be changed, the instance is closed and re-created with the desired changes.

In the case of hardware resources, the create operation usually establishes exclusive access to the hardware. This prevents two instances from interfering with one another. In some cases shared access is desirable, for example two different parts of a project may want to check the status of the same digital input. Rather than requiring the porting layer to manage the complexity of multiple clients of a single hardware resource, this is deferred to the JavaScript layer where mechanisms can be implemented to support this in a way that is tailored to the specific project need.

The create operation is often the largest part of the implementation of an IO class. This is because it must validate the configuration, initialize the IO resource, and bind the IO resource to the JavaScript object. Fortunately, create operations are usually infrequent compared to read and write operations. Create can take the time to set up data structures so that the read and write operations can execute relatively quickly.

### Read Operation
The read operation returns data from an IO resource.

The data returned by the read operation depends on the IO class definition. For example, a digital input returns a number with the value of 0 or 1.

```js
let input = new Digital({
	pin: 0,
	mode: Digital.Input,
});
console.log(input.read());
```

The kind of data returned by the read operation is determined by the `format` property of the IO instance, which is explained below in the Data Format section. Each IO class defines a default format that is suitable for many situations.

If no data is available, the result of the read operation is `undefined`. No exception is thrown in this situation.

Scripts that perform read operations may call read at any time. To avoid polling, the IO class for a given input may support the `onReadable` callback. The `onReadable` callback is a notification that new data is available to be read, but it does not provide the data. The read operation is the only way to read data.

Most kinds of IO have one of the following behaviors for their read operation:

- Return the current value of the IO resource. 

	Examples of this include digital inputs and analog inputs. Performing a read operation does not change what will be returned by the next read operation. Only changes to the IO resource itself change the value. The `onReadable` callback is invoked when the value of the IO resource changes.
- Return data from the input buffer. 

	Examples of this include serial and TCP network connections. Once data is read from a serial connection, that data is removed from the input buffer. A subsequent read will receive the next data in the buffer. The `onReadable` callback is invoked when new data is received.

### Write Operation
The write operation sends data to an IO resource.

The data accepted by the write operation depends on the IO class definition. For example, a digital output expects a number with the value of 0 or 1.

```js
let output = new Digital({
	pin: 2,
	mode: Digital.Output,
});
output.write(1);
```

The kind of data accepted by the write operation is determined by the `format` property of the IO instance, which is explained below in the Data Format section. Each IO class defines a default format that is suitable for many situations.

Scripts that perform write operations may call write at any time. The IO instance may not always be able to accept new data, such as when its output buffer is full. If write is called in this situation, an exception is thrown. Such IO instances generally support the `onWritable` callback which indicates when space is available in the output buffer. The following example uses the `onWritable` callback to transmit a continuous stream of asterisk (ASCII 42) characters.

```js
new Serial({
	baud: 921600,
	onWritable(count) {
		while (count--)
			this.write(42);
	}
});
```

Most kinds of IO have one of the following behaviors for their write operation:

- Change the current value output by the IO resource. 

	A digital output is an example of this behavior. Performing a write operation immediately changes what is output by the IO resource. The `onWritable` callback is not useful for this case, as the value may be changed at any time.
- Add data to the output buffer. 

	Examples of this include serial and TCP network connections. Once data is written from a serial connection, that data is transmitted over a period of time. The `onWritable` callback is invoked when space has been freed in the output buffer.

### Close Operation
The close operation releases all hardware resources associated with the IO instance.

```js
serial.close();
```

The specific IO class defines additional details of its close behavior. For example, a digital output may become a digital input when not in use to reduce power or a serial port may terminate any pending output.

No callbacks are invoked once the close operation begins. Any pending callbacks are canceled.

### Data Formats
The read and write operations operate on some kind of data. Because JavaScript is an untyped language, the kind of data may be anything supported by the language. Each kind of IO defines the data format or data formats that it supports. If the IO kind supports more than one data format, it also defines the default data format.

The IO Class Pattern defines that the format used is managed through the `format` property, present on all IO instances. The value of the `format` property is a string.  The `format` may be changed at runtime.

The serial IO type supports two data formats. The first is `number`, which is used to read and write a single byte at a time. The second is `buffer`, which is used to read and write buffers of bytes. Depending on the situation, one or the other is more convenient or efficient.

For example, consider the following example which reads one byte from serial as a byte and then uses that value to read the following bytes into a buffer:

```js
serial.format = "number";
let count = serial.read();
serial.format = "buffer";
let data = serial.read(count);
```

A "string" data format, while not yet implemented, seems necessary in some cases. The data format specifier would need to include the text encoding, for example "string;ascii" or "string;utf8".

This approach to data formats has similarities with [streams in Node.js](https://nodejs.org/api/stream.html), which may return strings of various encodings, buffers, and objects.

### Beyond Callback Functions
The IO Class Pattern uses callback functions to deliver notifications. One motivation for this choice is that it closely reflects the common native implementations of IO, which helps to simplify porting. Another reason is that callbacks may be used to implement other common forms of notifications including events and promises. The original [IO Class Pattern proposal](https://gist.github.com/phoddie/166c9c17b2f31d0beda9f2410a219268) explores this area in depth, giving examples of how to apply mixins to IO Classes to provide an async- and event-based API. The simplicity and consistency of the IO Class makes the implementation of general purpose mixins small and straightforward.

## IO Kinds
The IO Class Pattern, as described above, defines the fundamental behavior of an IO Class. Each particular kind of IO applies that definition to its specific characteristics and needs to create a class definition for that particular kind of IO. This section describes the different kinds of IO Classes implemented for ESP8266 and the specific adaptations.

### Digital
The built-in `Digital` IO class is used for digital inputs and outputs.

```js
import Digital from "embedded:io/digital";
```

#### Constructor Properties

| Property | Description |
| :---: | :--- |
| `pin` | A number from 0 to 16 indicating the GPIO number to control. This property is required.
| `mode` |A value indicating the mode of the IO. May be `Digital.Input`, `Digital.InputPullUp`, `Digital.InputPullDown`, `Digital.InputPullUpDown`, `Digital.Output`, or `Digital.OutputOpenDrain`. This property is required.
|`edge` | A value indicating the conditions for invoking the `onReadable` callback. Values are `Digital.Rising`, `Digital.Falling`, and `Digital.Rising | Digital.Falling`. This value is required if `onReadable` is used and ignored otherwise.

#### Callbacks

**`onReadable()`**

Invoked when the input value changes depending on the value of the `mode` property.

#### Data Format
The `Digital` class always uses a data format of `number` with values of 0 and 1.

#### Use Notes
A digital IO instance configured as an input does not implement write; one configured as an output does not implement read.

#### Examples

The following example creates a digital output to control the built-in LED on the ESP8266 board, and turns it off by writing a 1 to it.

```js
const led = new Digital({
   pin: 2,
   mode: Digital.Output,
});
led.write(1);		// off
```

The following example uses the built-in flash button on the ESP8266 to control the `led` created in the preceding example.

```js
let button = new Digital({
	pin: 0,
	mode: Digital.InputPullUp,
	edge: Digital.Rising | Digital.Falling,
	onReadable() {
		led.write(this.read());
	}
});
```

> Note: The `Digital` class is implemented in JavaScript using the more general `DigitalBank` IO class described in the following section.

### Digital Bank
The built-in `DigitalBank` class provides simultaneous access to a group of digital pins.

```js
import DigitalBank from "embedded:io/digitalbank";
```

Many microcontrollers, including the ESP8266, provide access to their digital pins through unified memory mapped hardware ports that make it possible to read and write several pins as a single operation. The `DigitalBank` IO provides direct access to this capability.

#### Constructor Properties

| Property | Description |
| :---: | :--- |
| `pins` |A bit mask with pins to include in the bank set to 1. For example, the bit mask for a bank to access pins 2 and 3 is 0x0C (0b1100). This property is required.
| `mode` | A value indicating the mode of the IO, May be `Digital.Input`, `Digital.InputPullUp`, `Digital.InputPullDown`, `Digital.InputPullUpDown`, `Digital.Output`, or `Digital.OutputOpenDrain`. All pins in the bank use the same mode. This property is required.
| `rises` | A bit mask indicating the pins in the bank that should trigger an  `onReadable` callback when transitioning from 0 to 1. When an `onReadable` callback is provided, at least one pin must be set in `rises` and `falls`.
| `falls` | A bit mask indicating the pins in the bank that should trigger an  `onReadable` callback when transitioning from 1 to 0. When an `onReadable` callback is provided, at least one pin must be set in `rises` and `falls`.

#### Callbacks

**`onReadable(triggers)`**

Invoked when the input value changes depending on the value of the `mode`, `rises`, and `falls` properties. The `onReadable` callback receives a single argument, `triggers`, which is a bit mask indicating each pin that triggered the callback with a 1.

#### Data Format
The `DigitalBank` class always uses a data format of `number`. The value is a bit mask. On a read operation, any bit positions that are not included in the `pins` bit mask are set to 0. This requirement is important as otherwise the state of reserved pins or pins used by another bank may be leaked.

#### Use Notes
A digital IO bank instance configured as an input does not implement write; one configured as an output does not implement read.

Multiple `DigitalBank` instances may be created, however none of them may use the same pins.

#### Implementation Notes
Because the ESP8266 provides access to (all but one) of its digital pins through a single hardware port, `DigitalBank` is the foundation for Digital IO and the single-pin `Digital` IO class builds on that. On a microcontroller that only provides access to each pin independently, not as a group, it would make sense to implement `DigitalBank` using `Digital`. The API for both `Digital` and `DigitalBank` remain consistent regardless of the implementation choice.

#### Examples
The following example creates a `DigitalBank` to output to pins 12 through 15.

```js
let leds = new DigitalBank({
	pins: 0xF000,
	mode: DigitalBank.Output,
});
leds.write(0xF000);
```

The following example is functionally equivalent to the LED example in the Digital section, but uses `DigitalBank`.

```js
let button = new DigitalBank({
	pins: 1 << 0,
	mode: DigitalBank.InputPullUp,
	rises: 1 << 0,
	falls: 1 << 0,
	onReadable() {
		led.write(this.read() ? 1 : 0);
	}
});
```

The following examples uses a `DigitalBank` to monitor pins 1 and 15 as inputs. It reports when pin 1 rises or pin 15 falls.

```js
let buttons = new DigitalBank({
	pins: (1 << 1) | (1 << 15),
	mode: DigitalBank.Input,
	rises: 1 << 1,
	falls: 1 << 15,
	onReadable(triggered) {
		if (triggered & (1 << 1))
			trace("Pin 1 rise\n");
		if (triggered & (1 << 15))
			trace("Pin 15 fall\n");
	}
});
```

### Analog Input
The built-in `Analog` IO class represents an analog input source.

```js
import Analog from "embedded:io/analog";
```

#### Constructor Properties

| Property | Description |
| :---: | :--- |
|  `pin` | The number of the analog input. The ESP8266 has only a single analog input so this property is unused.

#### Callbacks
There are no callbacks supported. Analog inputs are generally continuously fluctuating so the value is always changing which would cause onReadable to be invoked continuously.

#### Data Format
The data format is always a number. The value returned is an integer from 0 to a maximum value based on the resolution of the analog input.

#### Usage Notes
The analog input on the ESP8266 always provides 10-bit values. The analog input devices have a read-only `resolution` property which indicates the number of bits of resolution provided by values returned by the instance.

#### Implementation Notes
An `onReadable` callback may be useful. It could trigger based on various conditions, such as changing by more than a certain amount or entering a certain range of values. This is similar to triggers used in energy management work with very-low-power co-processors. This is an area for future work.

#### Example
The following example displays the value of an analog input as a floating point number from 0 to 1. The `resolution` property is used to scale the result of the `read` call.

```js
let analog = new Analog({});
trace(analog.read() / (1 << analog.resolution), "\n");
```

### PWM
The built-in `PWM` IO class provides access to the pulse-width modulation capability of pins.

```js
import PWM from "embedded:io/pwm";
```

#### Constructor Properties

| Property | Description |
| :---: | :--- |
|  `pin` | A number from 0 to 16 indicating the GPIO number to operate as a PWM output. This property is required.
|  `hz` | A number specifying the frequency of the PWM output in Hz. This property is optional.

#### Callbacks
There are no callbacks supported.

#### Data Format
The data format is always a number. The `write` call accepts integers between 0 and a maximum value based on the resolution of the PWM output.

#### Use Notes
PWM instances have a read-only `resolution` property which indicates the number of bits of resolution accepted on writes. PWM outputs on the ESP8266 always use 10-bit values.

The ESP8266 supports only a single PWM output frequency across all PWM output pins. Attempts to construct a PWM with `hz` specified when an existing PWM has already specified a different frequency will fail. A new frequency may be specified if all PWM instances that requested the original frequency have been closed.

When a PWM instance is created, it defaults to a duty cycle of 0% until a `write` is performed.

#### Example
The following example creates a PWM output on pin 5 with a 10 kHz output frequency and sets it to a 50% duty cycle. The `resolution` property is used to scale the argument to `write`.

```js
let pwm = new PWM({ pin: 5, hz: 10000 });
pwm.write(0.5 * ((1 << pwm.resolution) - 1));
```

### I<sup>2</sup>C 
The built-in `I2C` class implements an I<sup>2</sup>C Master to communicate with one address on an I<sup>2</sup>C bus.

```js
import I2C from "embedded:io/i2c";
```

#### Constructor Properties
| Property | Description |
| :---: | :--- |
| `data` | A number from 0  to 16 indicating the GPIO number of the I<sup>2</sup>C data pin. This property is required.
| `clock` | A number from 0 to 16 indicating the GPIO number of the I<sup>2</sup>C clock pin. This property is required.
| `hz` | The speed of communication on the I<sup>2</sup>C bus. This property is required.
| `address` | The 7-bit address of the I<sup>2</sup>C slave device to communicate with.

#### Callbacks
There are no callbacks for the built-in `I2C`. All operations are performed synchronously.

#### Data Format
The data format is always a buffer. The `write` call accepts an `ArrayBuffer` or a `TypedArray`. The `read` call always returns an `ArrayBuffer`.

#### Use Notes
Many I<sup>2</sup>C buses use the higher-level SMB protocol, an extension to the I<sup>2</sup>C protocol that simplifies its use.  The `SMBus` class is a subclass of the `I2C` class that provides support for working with SMBus devices.

The I<sup>2</sup>C protocol is transaction-based. At the end of each read and write operation, a stop bit is sent. If the stop bit is 1, it indicates the end of the transaction; if 0, it indicates that there are additional operations on the transaction. The `read` and `write` calls set the stop bit to 1 by default. An optional second parameter to the `read` and `write` allows the stop bit to be specified. Pass `false` to set the stop bit to 0, and `true` to set the stop bit to 1. 

#### Example
The following example reads the number of touch points from an FT6206 touch sensor, and then retrieves the X and Y coordinates for the active touch points.

```js
let touch = new I2C({
	data: 4,
	clock: 5,
	hz: 600000,
	address: 0x38
});

touch.write(Uint8Array.of(2));

let count = touch.read(1);
count = (new Uint8Array(count))[0];
trace(`Touch points ${count}.\n`);

if (count)
	touch.write(Uint8Array.of(3), false);
	const data = new Uint8Array(touch.read(6 * count));
	// decode touch data points...
}
```

### Serial
The built-in `Serial` class implements bi-directional communication over serial port at a specified baud rate.

```js
import Serial from "embedded:io/serial";
```

#### Constructor Properties
| Property | Description |
| :---: | :--- |
| `baud` | A number specifying the baud rate of the connection. This property is required.

> **Note**: No pins are specified because there is only a single full-duplex hardware serial port on the ESP8266, which is always connected to GPIO pins 1 and 3. 

#### Callbacks

**`onReadable(bytes)`**

The `onReadable` callback is invoked when new data is available to read. The callback receives a single argument that indicates the number of bytes available.

**`onWritable(bytes)`**

The `onWritable` callback is invoked when space has been freed in the output buffer. The callback receives a single argument that indicates the number of bytes that may be written to the output buffer without overflowing.

#### Data Format
The data format is either `number` for individual bytes, or `buffer` for groups of bytes. The default data format is `number`. When using the `buffer` format, the `write` call accepts an `ArrayBuffer` or a `TypedArray`. The `read` call always returns an `ArrayBuffer`.

#### Use Notes
If the `onWritable` callback is provided, it is invoked immediately following instantiation.

When a `write` is attempted, it will fail with an exception if there is insufficient space in the output buffer to hold all the data to be written. Partial data is never written.

When using the `buffer` data format, calling read with no arguments returns all bytes available in the FIFO. The number of bytes to read may be passed. If fewer bytes are available in the FIFO than requested, only the bytes available are returned -- no exception is thrown and the `read` call will not wait for additional data to arrive.

#### Implementation Notes
The ESP8266 has a 128-byte FIFO on both the serial input and output. The implementation does not add any additional buffers.

An `onError` callback could be supported to report receive buffer overflows and other errors detected by the hardware.

If no callbacks are specified, the implementation reduces its memory allocation by eliminating the storage used to maintain references to the callbacks.

The API should include the ability to flush the input and output FIFOs.

#### Examples
The following example implements a simple serial echo. It uses the default data format of `number` to read and write individual bytes.

```js
let serial = new Serial({
	baud: 921600,
	onReadable: function(count) {
		while (count--)
			this.write(this.read());
	},
});
```

The following example continuously outputs text to the serial output. It uses the `onWritable` callback to write data as quickly as possible without overflowing the output FIFO. The example uses the `buffer` data format to maximize throughput.

```js
const message = ArrayBuffer.fromString("Since publication of the first edition in 1997, ECMAScript has grown to be one of the world's most widely used general-purpose programming languages. It is best known as the language embedded in web browsers but has also been widely adopted for server and embedded applications.\r\n");

let offset = 0;
	
const serial = new Serial({
	baud: 921600,
	onWritable: function(count) {
		do {
			const use = Math.min(count, message.byteLength - offset);
			this.write(message.slice(offset, offset + use));
			count -= use;
			offset += use;
			if (offset >= message.byteLength)
				offset = 0;
		} while (count);
	},
});
serial.format = "buffer";
```

### TCP Socket
The built-in `TCP` network socket class implements a general purpose, bi-directional TCP connection. 

```js
import TCP from "embedded:io/socket/tcp";
```

The TCP socket is only a TCP connection. It is not a TCP listener, as in some networking libraries. The TCP listener is a separate class.

#### Constructor Properties
| Property | Description |
| :---: | :--- |
| `address` | A string with the IP (v4) address of the remote endpoint to connect to. This property is required.
| `port` | A number specifying the remote port to connect to. This property is required.
| `nodelay` | A boolean indicating whether to disable Nagle's algorithm on the socket. This property is equivalent to the `TCP_NODELAY` option in the BSD sockets API. This property is optional and defaults to false.
| `from` | An existing TCP socket instance from which the native socket instance is taken to use with the newly created socket instance. This property is optional and designed for use with a TCP listener. An example is given in the TCP listener section. When using the `from` property, the `address` and `port` properties are not required, and are ignored if specified.

#### Callbacks

**`onReadable(bytes)`**

Invoked when new data is available to be read. The callback receives a single argument that indicates the number of bytes available to read.


**`onWritable(bytes)`**

Invoked when space has been made available to output additional data. The callback receives a single argument that indicates the number of bytes that may be written to the TCP socket without overflowing the output buffers.

**`onError`**

Invoked when an error occurs. Once `onError` is invoked, the connection is no longer usable. Reporting the error type is an area for future work.

#### Data Format
The data format is either `number` for individual bytes, or `buffer` for groups of bytes. The default data format is `buffer`. When using the `buffer` format, the `write` call accepts an `ArrayBuffer` or a `TypedArray`. The `read` call always returns an `ArrayBuffer`.

#### Use Notes
When the socket successfully connects to the remote host, the `onWritable` callback is invoked as it is now possible to write data.

The `onError` callback is invoked when the remote socket disconnects for any reason, including a clean TCP disconnect.

If there is insufficient buffer space available for a `write` request, no data is written and an exception is thrown.

There is usually no need for scripts using TCP socket to combine multiple write operations into a single `write` call. When possible, the TCP socket implementation combines writes that occur within a single turn of the JavaScript event loop.

#### Implementation Notes
The TCP socket is implemented using the [lwip](https://savannah.nongnu.org/projects/lwip/) networking library. It uses the lowest-level public API, the callback API.

Support for the `number` data format used to read/write bytes instead of buffers has proven convenient when implementing protocols that use TCP in place of serial. It is not an essential feature. On the other hand, direct support for strings is important and an area for future work.

The TCP socket accepts an `address` property to specify the remote host. That is necessary for some situations, but often the host name is known. Currently the host name is resolved externally to the socket. It would be convenient to pass the host name as an alternative to the address. For security reasons, it may be necessary to use the host name to allow a white or black list to be applied to limit access to hosts.

Defining optional keep-alive properties for the constructor is a topic for future work.

#### Example
The following examples connects to an HTTP server, sends a GET request for the root, and traces the response to the debug console.

```js
new TCP({
	address: "93.184.216.34",	// www.example.com resolved outside this example
	port: 80,
	onWritable() {
		if (this.requested)
			return;

		this.write(ArrayBuffer.fromString("GET / HTTP/1.1\r\n"));
		this.write(ArrayBuffer.fromString("Host: www.example.com\r\n"));
		this.write(ArrayBuffer.fromString("Connection: close\r\n"));
		this.write(ArrayBuffer.fromString("\r\n"));
		this.requested = true;
	}
	onReadable(count) {
		trace(String.fromArrayBuffer(this.read()));
	}
	onError() {
		trace("\n\n** Disconnected **\n");
	}
});
```

### TCP Listener
The built-in TCP `Listener` class provides a way to listen for and accept incoming TCP connection requests.

```js
import Listener from "embedded:io/socket/listener";
```

#### Constructor Properties
| Property | Description |
| :---: | :--- |
| `port` | A number specifying the port to listen on. This property is optional.

#### Callback

**`onReadable(requests)`**

Invoked when one or more new connection requests are received. The callback receives a single argument that indicates the total number of pending connection requests.

#### Data Format
The TCP `Listener` class uses `socket/tcp` as its sole data format.

#### Use Notes
The `read` function returns a `TCP` Socket instance. The instance is already connected to the remote host. The `read` and `write` functions operate as usual. There are no callback functions installed, so the script cannot receive `onReadable`, `onWritable`, or `onError` notifications. To configure the socket, pass it to the `TCP` Socket constructor using the optional `from` argument. An example is shown below.

#### Implementation Notes
The constructor should support an optional `address` property to bind to a specific network interface.

#### Example
The following example implements a simple HTTP echo server. It accepts incoming requests and sends back the complete request (including the request headers) as the response body. The `TCPEcho` class reads the request and generates the response.

```js
class TCPEcho {
	constructor(options) {
		new TCP({
			...options,
			onReadable: this.onReadable
		});
	}
	onReadable() {
		const response = this.read();
	
		this.write(ArrayBuffer.fromString("HTTP/1.1 200 OK\r\n"));
		this.write(ArrayBuffer.fromString("connection: close\r\n"));
		this.write(ArrayBuffer.fromString("content-type: text/plain\r\n"));
		this.write(ArrayBuffer.fromString(`content-length: ${response.byteLength}\r\n`));
		this.write(ArrayBuffer.fromString("\r\n"));
		this.write(response);
	
		this.close();
	}
}

new Listener({
	port: 80,
	onReadable(count) {
		while (count--) {
			new TCPEcho({
				from: this.read()
			});
		}
	}
});
```

### UDP Socket
The built-in `UDP` network socket class implements the sending and receiving of UDP packets. 

```js
import UDP from "embedded:io/socket/udp";
```

#### Constructor Properties
| Property | Description |
| :---: | :--- |
| `port` | The local port number to bind the UDP socket to. This property is optional.

#### Callback

**`onReadable(packets)`**

Invoked when one or more packets are received. The callback receives a single argument that indicates the total number of packets available to read.

#### Data Format
The data format is always `buffer`. The `write` call accepts an `ArrayBuffer` or a `TypedArray`. The `read` call always returns an `ArrayBuffer`.

#### Use Notes
The `read` call returns a complete UDP packet as an `ArrayBuffer`. Partial reads are not supported. The returned packet data has two properties attached to it: 

- `address`, a string containing the packet sender's address
- `port`, the port number used to send the packet.

The `write` call takes three arguments: remote address string, remote port number, and the packet data as an `ArrayBuffer` or `TypedArray`. If there is insufficient memory to transmit the packet, the `write` call throws an exception.

#### Implementation Notes
The UDP socket is implemented using the [lwip](https://savannah.nongnu.org/projects/lwip/) networking library. It uses the lowest-level lwip public API, the callback API.

Specifying optional properties to the constructor to support multicast is an area for future work.

As with the TCP socket, it would be useful to be able to specify a host name for the remote end point.

#### Example
The following example implements a simple SNTP client to retrieve the current time from a network time server at address 208.113.157.157. The UDP socket is closed when a response is received. The example shows how to access the `address` and `port` properties that indicate the sender of a received UDP packet.

```js
let sntpClient = new UDP({
	onReadable: function(count) {
		const buffer = this.read();
		trace("Packet from ${buffer.address}:${buffer.port}\n`);

		const packet = new DataView(buffer);
		let milliseconds = (packet.getUint32(40) - 2208988800) * 1000;
		trace("SNTP time " + (new Date(milliseconds)) + "\n");

		this.close();
	},
 });

const packet = new Uint8Array(48);
packet[0] = (4 << 3) | (3 << 0);		// version 4, mode 3 (client)
sntpClient.write("208.113.157.157", 123, packet);
```

<!--
### Wakeable Digital
The built-in `WakeableDigital` class represents a digital input source used in energy management. This IO kind applies the IO Class Pattern in a somewhat unusual way.

```js
import WakeableDigital from "embedded:io/wakeabledigital";
```

The ESP8266 has a deep sleep feature where the microcontroller turns off, but a small amount of memory (256 bytes) is retained. When the reset pin is pulled low, for example by a sensor configured to trigger an interrupt under a certain condition, the microcontroller reboots, still retaining the small memory area. The Wakeable Digital pin provides a way for scripts to know whether the most recent boot of the microcontroller is due to a wake from deep sleep or a conventional hard reset (e.g. power applied after being off). The script uses this information to change its behavior.

Putting the ESP8266 into deep sleep is out of scope for IO. The `System.deepSleep()` JavaScript function is provided for development purposes.

#### Constructor Properties
| Property | Description |
| :---: | :--- |
| `pin` | The pin to use to detect if the most recent wake was due to a cold boot or a wake from deep sleep. For the ESP8266, this must be set to the string "RST" for the reset pin.

#### Callbacks

**`onReadable()`**

Invoked following instantiation if the device was woken from deep sleep. 

#### Data Format
The Wakeable Digital IO always uses a data format of `number`. A value of 0 indicates the device did not wake from a deep sleep and a value of 1 indicates that it did wake from a deep sleep.

#### Use Notes
The `read` call is available immediately following instantiation. Consequently, the `onReadable` callback is not strictly required as the state of the pin cannot change after the microcontroller boots. The `onReadable` callback is useful with light sleep, which does not terminate program execution as does deep sleep.

#### Example
The following example uses the Wakeable Digital pin to check whether the device was hard reset or woken from deep sleep.

```js
let wakeable = new WakeableDigital({
	pin: "RST",
});
trace(wakeable.read() ? "Woke from deep sleep\n" : "Hard reset\n");
```
-->

## IO Providers
IO providers access IO resources that are external to the built-in IO resources. IO providers often use the built-in IO resources to access their external IO resources. The definition of "external" encompasses a wide range of possibilities.

- A separate component on the same board as the microcontroller. 

	Examples of this include GPIO and Analog expanders. These operate over shared-bus protocols like I<sup>2</sup>C and SPI to provide additional IO pins.
- A separate board physically connected to the board holding the microcontroller. 

	An example of this is an Arduino connected to a microcontroller over a serial connection as used by the Firmata protocol.
- A separate physical device in close proximity. 

	Examples of this include peripherals connected by Bluetooth LE and Decentralized Ambient Synchronization ([DAS](https://blog.moddable.com/blog/das/)) using mDNS over a UDP network connection.
- A separate physical device at a physically remote location. 

	Examples of this include the Firmata protocol running over a TCP connection and many IoT cloud services operating over protocols including HTTP/REST, MQTT, and WebSocket.

### Instantiating a Provider
The provider constructor has the same API as an IO kind, a single object containing properties to configure the provider. The following example instantiates the [MCP23017](https://www.microchip.com/wwwproducts/en/MCP23017) GPIO expander, a component that provides 16 GPIO pins through an I<sup>2</sup>C interface.

```js
import Expander from "expander";

const expander = new Expander({
	sda: 5,
	scl: 4,
	hz: 1000000,
	address: 0x20,
});
```

The constructor receives all properties necessary to establish a connection to the external IO resource. As with an IO resource, these properties are fixed at the time of construction. In this example, the properties passed to the constructor are identical to those required to initialize an I<sup>2</sup>C  connection, as the component communicates over I<sup>2</sup>C . Additional properties may be defined as needed to configure the connection to the external IO resources.

When a script no longer needs to use the provider, it should close the instance to tear down the connection and free any resources it has reserved. In the case of the MCP23017 Expander, the close operation frees the `I2C` instance used to communicate with the component.

```js
expander.close();
```

### IO Operations with Providers
The IO resources available from the provider follow the IO Class Pattern with their constructors located on the provider instance. The following example performs a write operation to pin 13 of the expander.

```js
let led = new expander.Digital({
	pin: 13,
	mode: expander.Digital.Output,
});
led.write(1);
```

Similarly, several digital pins may be accessed together through a `DigitalBank`. The following example reads the values of pins 8 through 15 (inclusive).

```js
let buttons = new expander.DigitalBank({
	pins: 0xFF00,
	mode: expander.Digital.Input,
});
let result = buttons.read();
```

This method of accessing IO constructors from an instance is similar to that used by the Johnny-Five robotics framework. Here is a fragment from the [hello world](http://johnny-five.io/#hello-world) example in the Johnny-Five documentation.

```js
var led = new five.Led(13);
led.blink(500);
```

The MCP23017 expander has an option to trigger an interrupt when the value of one or more inputs changes. To use this capability, the constructor must be configured with the `interrupt` property, which indicates the built-in GPIO pin the interrupt is connected to. In the following example, the `interrupt` property is set to 0, indicating the interrupt is connected to digital pin 0.

```js
const expander = new Expander({
	sda: 5,
	scl: 4,
	hz: 1000000,
	address: 0x20,
	interrupt: 0,
});
```

With the interrupt configured, the `onReadable` callback may be used. 

```js
let buttons = new expander.DigitalBank({
	pins: 0xFF00,
	rises: 0xFF00,
	falls: 0xFF00,
	mode: expander.Digital.Input,
	onReadable(pins) {
		const result = this.read();
		trace(`Pins ${pins.toString(2)} changed. Buttons now ${result.toString(2)}.`);			
	}
});
```

**Note**:  By convention, implementations of the IO Class Pattern directly represent the hardware they are associated with, both the features and limitations. For example, it is possible to support the `onReadable` callback without using the interrupt pin by polling. However, this is discouraged to accurately reflect the hardware capabilities to higher layers. This helps to keep low level implementations small, maintainable, and efficient. Higher layers, of course, may add such functionality as needed, consistent with the programming model they support.

### Synchronous and Asynchronous IO with Providers
IO resources accessed through a provider may support synchronous and/or asynchronous operation, as long as the general rule about non-blocking IO is respected. The IO Class Pattern defines callbacks to invoke when asynchronous operations complete. Providers are not required to implement these, but are encouraged to do so when the IO resources they represent may take some time to complete.

#### Asynchronous Constructors
Some IO resources are not available for use immediately after the constructor returns. The TCP client constructor is an example of one such constructor, as it is necessary to wait for the TCP connection to be established before any IO operations may occur.

The IO provider may not know what IO resources are available until it has successfully established a connection to the remote resource. For this reason, a provider may not have any IO constructors on its instance until some time after its constructor completes. Such providers should support the `onReady` callback to notify scripts when the provider is ready for use.

```js
let provider = new CloudProvider({
	url: "mqtt://www.example.com",
	onReady() {
		let led = new this.Digital({
			pin: 13,
			mode: this.Digital.Output,
		});
		led.write(1);
	}
});
```

Note that the MCP23017 expander does not implement the `onReady` callback as it supports a separate component on the same board as the microcontroller accessing it, so there are no significant latencies.

#### Asynchronous I<sup>2</sup>C 
All of the IO kinds defined earlier in this document implement asynchronous IO by following the IO Class Pattern directly. It is less obvious how to implement an I<sup>2</sup>C  Master. An implementation of the Firmata Client through the IO Provider API provided a motivation to explore the problem and to find a solution.

I<sup>2</sup>C  performs read and write operations with buffers of bytes, much like serial and TCP IO. Serial and TCP (once the connection is established) are essentially peer protocols -- either side of the connection may initiate a write operation at any time. I<sup>2</sup>C , by contrast, is a master/slave protocol. The slave may only send bytes for the master to read when requested to do so. That requires the master to issue a read request to the slave device to receive data.

To support the master/slave protocol of I<sup>2</sup>C  asynchronously, the read operation is broken into two steps. The first step is issuing the read request. With I<sup>2</sup>C , the master specifies the number of bytes it will read. The call to the `read` function therefore must include the number of bytes to read. In the following example, the number of bytes to read is 4.

```js
i2c.read(4);
```

This enqueues a read operation but does not return the data (it returns `undefined`, the result when no data is available). When the data is available, the provider invokes the `onReadable` callback. The script using the `I2C` instance retrieves the result of the read operation by calling the `read` function with no arguments.

```js
let i2c = new provider.I2C({
	onReadable() {
		let data = this.read();
		trace(`I2C read returned ${data.byteLength} bytes.\n`);
	}
});
i2c.write(Uint8Array.of(4));
i2c.read(2);
```

A `read` call with no arguments returns the result of the earliest pending read operation requested. If no result is available, it returns `undefined`. This first-in, first-out rule ensures a predictable behavior when multiple asynchronous read operations are outstanding.

## Conclusion
The IO Class Pattern is a small API designed to address a wide range of IO uses in JavaScript. The core API contains only four functions — the `constructor`, `close`, `read`, and `write` — together with a handful of supporting callback functions — `onReady`, `onReadable`, `onWriteable`, and `onError`. From this foundation, implementations have been created for individual digital inputs & output, digital banks of inputs & outputs, analog input, serial, I<sup>2</sup>C  master, TCP socket, TCP listener, UDP socket, and a wake pin. The Provider Class Pattern extends the IO Class Pattern to work with remote IO resources of all kinds.

Implementing broad support for the IO Class Pattern for the ESP8266 microcontroller using the XS engine in the Moddable SDK provides experience using the API from several perspectives. The implementation itself is straightforward, focused on connecting the raw hardware I/O resources to the JavaScript language using the [XS in C API](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/xs/XS%20in%20C.md). In no case is a translation layer required to adapt the native behavior of the IO to the JavaScript APIs. This is desirable as such translation can be difficult to implement reliably.

From the API perspective, the IO Class Pattern provides clear guidance to the designer adding support for a new IO type. The design starts from consideration of how the IO capabilities can fit into the pattern rather than defining an API from scratch for each IO type. The pattern has proven itself adaptable to a range of different IO kinds. Future implementation work will explore and, no doubt, extend this.

Perhaps the most interesting perspective is as a script writer using IO classes that follow the pattern. The small API size is easy to remember. This makes it quick and comfortable to work with a range of IO. There are, of course, details that differ from one IO type to another. A digital input is quite different from a UDP socket. Still, these differences are consistent with needs of the IO, not arbitrary differences because their APIs happened to be designed by different individuals at different times with different priorities or different programming style preferences. Overall, this makes it relatively easy to both read and write code that applies the IO Class Pattern.

Based on this exercise of building an implementation of the IO Class Pattern for a microcontroller, the design achieves its goals well. The API meets the needs of low-level script developers to access IO, it is possible to implement efficiently on resource-constrained embedded hardware, and the implementation/porting effort is focused and manageable. 

There is a great deal of work remaining to fully explore the IO Class Pattern. More will be learned from future work, and those lessons will lead to refinements in the design. Areas for future work include ports to other microcontrollers, support for other runtime environments beyond the Moddable SDK, and implementations of other kinds of IO and providers.
