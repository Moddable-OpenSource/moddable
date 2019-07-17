# Serial

This module provides UART access to Moddable scripts.

* [Serial](#serial)
* [Configuration](#configuration)
* [Blocking I/O](#blocking)
* [Asynchronous Read](#async)
* [Examples](#examples)

<a id="serial"></a>
## class Serial

* Source code:
* Relevant Examples: [echo](examples/echo), [write](examples/write), [poll](examples/poll), [arrayBuffer](examples/arrayBuffer)

The `Serial` object implements a serial connection using a UART.

```js
import Serial from "serial";
```

### constructor()

The `Serial` constructor takes its configuration information from the **manifest.json** file.

	let serial = new Serial();

<a id="configuration"></a>	
### Configuration

Configuration of the serial port is done in the **manifest.json** file.

```js
	"include": [
		"$(MODDABLE)/contributed/serial/manifest.json",
	],
	"platforms": {
		"esp32": {
			"defines": {
				"serial": {
					"interface": { "UART": 2 },
					"baud": 9600,
					"config": { "dataBits": 8, "parity": "#N", "stopBits": 1 },
					"tx_pin": 17,
					"rx_pin": 16,
					"trace": 0,
				},
			},
		},
	}
```

<a id="blocking"></a>
## Blocking I/O
	
A `timeout` can be set for blocking reads. The units are milliseconds:

	serial.setTimeout(10000);
	
> If a read operation is not fulfilled by the timeout, the call will return with a partial read.

#### Reading
	
You can read a number of bytes:

	let str = serial.readBytes(10);

You can read a line, which is a number of characters terminated by a carriage return `\r` or linefeed `\n`.

	let str = serial.readLine();

You can also read a number of bytes terminated by a length or one of a set of terminator characters. The read will return when the terminator is encountered, or the number of bytes is read, or the timeout expires.

	let str = serial.readBytesUntil("#$>", 100);

You can also read a line with additional terminators:

	let str = serial.readLineUntil("#$>");

##### Array Buffers

You can read into an array buffer. The second parameter is optional and specifies a maximum number of bytes to read. If the parameter is absent, the length of the array buffer will be used.

```js
let arrayBuffer = new ArrayBuffer(64);
serial.readBytes(arrayBuffer, 10);
```

You can also read into an array buffer, stopping at a terminator character. The number of bytes read is returned. This example stops reading after receiving an 'm' or an 'n'.

```js
let amt = serial.readBytesUntil(arrayBuffer, "mn");
```



#### Writing

You can write strings with an optional starting and ending position:

	serial.write("Hello");			// Hello
	serial.write("Hello", 3);		// llo
	serial.write("Hello", 1, 1);	// e 
	serial.write("Hello", -2);		// lo
	
You can write an array buffer with an optional starting and ending position.

	let buffer = new ArrayBuffer(6);
	let chars = new Uint8Array(buffer);
	
	// fill buffer with "test 1"
	chars[0] = 0x74;		chars[1] = 0x65;
	chars[2] = 0x73;		chars[3] = 0x74;
	chars[4] = 0x20;		chars[5] = 0x31;
	
	serial.write(buffer);			// test 1
	serial.write(buffer, 4);		// test
	serial.write(buffer, 1, 3);		// est
	serial.write(buffer, -3);		// t 1

<a id="async"></a>
### Asynchronous reading

You can set up a callback function which is called with data when a termination character is received, or when a number of characters is received.

Set an `onDataReceived` function to handle the data:

	serial.onDataReceived = function(data, len) {
		// implementation goes here...
	}

Call `serial.poll` with a dictionary of configuration values:

	serial.poll({ terminators: "\r\n", trim: 1 });

The dictionary can contain:

key | default | description
----|------|-----
`terminators` | "\r\n" | A string of termination characters, any of which will indicate the end of data.
`trim` | 0 | Remove terminator characters from data.
`chunkSize` | 256 | Data buffer size.

Call `serial.poll` with no dictionary to stop polling.

	serial.poll();
	
	
<a id="examples"></a>
## Examples

`examples/echo` demonstrates reading a string from the serial port and echoing it back out to the serial port. It also prints the line in the debugger.

`examples/poll` sets up an asynchronous reader which gets called back when data is available.

`examples/write` demonstrates writing strings and array buffers.

`drivers/sim7100/examples/gpsmap` demonstrates setting up the SIM7100 GPS, downloads tiles from OpenStreetMap.org and displays the map on a screen.

`drivers/sim7100/examples/sim7100` demonstrates setting up the SIM7100 GPS and sending the location to a SMS phone number.

`drivers/sim7100/examples/sim7100gps` demonstrates setting up the SIM7100 GPS and asynchronously receiving GPS updates.

`drivers/sim7100/examples/sim7100sms` demonsrates setting up the SIM7100 to send a SMS message.
