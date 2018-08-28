# XS6 for ESP8266
Copyright 2016 Moddable Tech, Inc.

Revised: November 22, 2016

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

# Getting Started 

Building for ESP8266 is straightforward. Currently, Mac and Linux (Ubuntu 16.04) are supported build hosts. For incremental changes, the build, deploy, and run cycle is quick, 10 or 20 seconds.

A serial monitor is necessary to see debug diagnostic output. There are plenty. I use the serial monitor in the [Arduino IDE](https://www.arduino.cc/en/Main/Software).

## Prepare Workstation -- Mac

You will need the OSX developer tools installed. This is easily accomplished: simply open a Terminal and type `git` and it will kick off a dialog that will let you install what you need.

You also need CMake installed (to build the Kinoma JS host tools.) If you have Homebrew installed, you can `brew install cmake` and ensure that it's in your path. Otherwise you will need to [download CMake for OSX](https://cmake.org/files/v3.6/cmake-3.6.2-Darwin-x86_64.dmg) and install it in `/Applications` in the usual way.

The [HiLetgo NodeMCU](http://hiletgo.com/product/html/?26.html) board uses a USB part from Silicon Labs. Download and install the driver from the Silicon Labs' [CP210x driver page](http://www.silabs.com/products/mcu/Pages/USBtoUARTBridgeVCPDrivers.aspx). Once installed and the board is connected, the device path is `/dev/cu.SLAB.USBtoUART`. If your board is at a different device path, change the `UPLOAD_PORT` variable in `makeEspArduino.mk`. (That is, use a command like `make UPLOAD_PORT=/dev/your_port ...`)

## Prepare Workstation -- Linux

Building on Ubuntu 16.04 will require you to install the usual core development tools and 32-bit compatibility libraries, and also `cmake`. That is, something like:

        sudo apt-get -y install build-essential libc6-x32 libc6-dev-x32 lib32z1 cmake

You will also likely need a console program, such as `minicom` or [sconsole](https://github.com/swetland/sconsole).

## Download & Build

You can obtain the XS6 source code via git. The project includes all of its dependencies using git submodules, so you'll need the `--recursive` flag. Pick a directory where you want to place all your ES6 code for your application. This can be anywhere you like, but as an example this doc uses `~/Projects`. This will check out the code to `~/Projects/xs6-esp`:

    cd ~/Projects
    git clone --recursive git@moddable.tech:peter/xs6-esp.git 

The source code works like an SDK: you can put your application's code anywhere, and then point the SDK at your code. Assuming that your ES6 code is in `~/Projects/MyProject`, you can use these commands:

    cd ~/Projects
    make -f ~/Projects/xs6-esp/Makefile SCRIPT=MyProject upload

The first time you use the `xs6-esp` source to do a build, it will take a few minutes, as it must build the XS6 host tools (that is, the ES6/JavaScript compiler) from Kinoma. This will only affect the first use of the SDK; subsequent compiles should take around 30 seconds or less.

### Managing Projects

You can place each JavaScript project wherever you like. When you do a build, the `Makefile` looks for the code for the project you specified in a subdirectory of your current directory. For example, when you run `make` command above from your `~/Projects` directory, it will compile`~/Projects/MyProject/main.js` (and other `*.js` files) along with the SDK source to create a firmware image. Meanwhile `~/Projects/MyOtherProject/*` can be compiled as an unrelated project using `make -f ~/Projects/xs6-esp/Makefile SCRIPT=MyOtherProject`.

As another example, to try out the sample scripts provided in the `xs6-esp` source, you would do this:

    cd ~/Projects/xs6-esp/scripts
    make -f ~/Projects/xs6-esp/Makefile SCRIPT=helloworld upload
    make -f ~/Projects/xs6-esp/Makefile SCRIPT=pubnub upload

...and so on.

### Custom Compiler Toolchains

The `xs6-esp` source includes precompiled gcc `esp-open-sdk` compiler toolchains for Linux and Mac. If you want to build your own for any reason, however, you can use that instead. To do so, you can set the `TOOLS_DIR` variable for `make`:

    export TOOLS_DIR=/path/to/esp-open-sdk
    make -f ~/Projects/xs6-esp/Makefile SCRIPT=MyProject

### Dependencies

This project uses Arduino Core for ESP8266, `esptool`, `esp-open-sdk`, Kinoma, and Espressif's partially open-source SDK for the ESP8266. However, these are all included in the installation already, so you don't need to do anything. For details, see the section later in this doc.

## User App Scripts

A simple "hello world" script is provided at `xs6-esp/scripts/helloworld/main.js`. The XS6-ESP application loads and executes the usual C `main` function at launch. One application at a time is built and deployed to the device.

The `Makefile` builds all file with a .js extension in the scripts folder. The scripts are compiled as ES6 modules, and linked together into an XS6 archive. Compiling and linking is done on the computer to eliminate parsing the JavaScript source code on the ESP8266.  This reduces memory use and speeds launch. The XS6 archive is built into the application as a character array global variable.

As all scripts are compiled as ES6 modules, they access each other as ES6 modules. For example, `main.js` accesses a script named `blink.js` using `import`.

	import Blink from "blink";

XS6 on ESP8266 implements the `require` and `require.weak` functions. They provide more control over loading and unloading modules, which is useful in a memory constrained environment.

	let Blink = require("blink);
	let Blink = require.weak("blink");

See the the section on "require(module) and require.weak(module)" in the [Programmer's Guide to Kinoma Element](http://kinoma.com/develop/documentation/element/#programmers-guide-to-kinoma-element-javascript-implementation) for details.

The `scripts` subfolder of `xs6-esp` contains a number of helpful sample applications. You can build these and try them out by either running `make` from the `xs6-esp` directory, or by setting the `SCRIPT_ROOT` flag:

    cd ~/Projects
    make -f ~/Projects/xs6-esp/Makefile SCRIPT_ROOT=~/Projects/xs6-esp SCRIPT=helloworld upload

> **Note**: The ESPP8266 implements the spiffs flash file system so it may be possible to one day run the XS6 archive from a file.

The commands above using the `make upload` target automatically flash your device. But if you want to see the compiled firmware image, it is placed in a subdirectory called `build` of your script's directory. For instance, if you use `SCRIPT=MyProject`, then the build will look for scripts in `~/Projects/MyProject/*.js` (and subdirectories), and will place compiled output in `~/Projects/MyProject/build/*`.

## Flashing a Device

The settings to connect to the device to flash it over USB have defaults in the `Makefile`, but you can change them if necessary. The device path is `UPLOAD_PORT` and the baud rate is `UPLOAD_SPEED`.  The default `UPLOAD_SPEED` is 921600, which seems to work reliably, despite warnings from `esptool` to the contrary. If necessary, you can use a slower baud rate.

As an example, this will build your project and then flash it on `/dev/ttyUSB1` with a baud rate of 115200:

    cd ~/Projects
    make -f ~/Projects/xs6-esp/Makefile SCRIPT=MyProject UPLOAD_PORT=/dev/ttyUSB1 UPLOAD_SPEED=115200 upload

(Lower upload speeds will, naturally, make your build flashes longer. But this may be necessary for some boards depending on the UART part they use.)


# App Development Notes & API Docs

## Debugging

Xsbug, the XS6 debugger, works with XS6 for ESP8266. Building XS6 with debugging support enabled (e.g. with `mxDebug` defined) consumes an additional 1400 bytes of RAM. All xsbug socket i/o is done with [lwIP](http://lwip.wikia.com/wiki/LwIP_Wiki).

The IP address of xsbug is defined in a global variable contained in the application file `main.cpp`. The application must provide this variable.

	unsigned char gXSBUG[4] = {192, 168, 1, 30};

Xsbug may also be run over the serial connection by specifying an IP address of 127.0.0.7.

	unsigned char gXSBUG[4] = {127, 0, 0, 7};

Set the IP address to all zeros to prevent XS6 from attempting to connect to xsbug.

	unsigned char gXSBUG[4] = {0, 0, 0, 0};

**Note**: The source code to xsbug6 is in the [KinomaJS](https://github.com/Kinoma/kinomajs) repository. The [README.md](https://github.com/Kinoma/kinomajs/blob/master/README.md) file at the root of the repository includes build instructions for xsbug6 on Mac OS X, Windows, and Linux/GTK. 

> **Note**: The xsbug application must be running before launching the script on the ESP8266, or the connection attempt will fail following a long wait.

## Built-in objects

The following built-in objects are available:

- Object
- Function
- Array
- String
- Boolean
- Number
- Math
- Error
- JSON
- ArrayBuffer
- Global functions - trace

The following language capabilities are also available:

- Generator
- Module

The following are disabled to save memory. They can be enabled if needed:

- Date
- Promise
- Symbol
- Proxy
- Map, Set, WeakMap, WeakSet
- DataView, TypedArray
- Global functions - decodeURI, decodeURIComponent, encodeURI, encodeURIComponent, escape, unescape

The following are not implemented:

- RegExp - big and complicated
- Global functions - eval - big, complicated, deprecated

## Sample applications

A set of sample applications is located at `xs6-esp/scripts`. The `SCRIPT` variable in the make script selects the application to build:

	make -f makeEspArduino.mk upload SCRIPT=timers

The following sample applications are provided:

* `HMC5883L` - Three axis magnetometer connected over I2C.
* `TMP102` - Temperature sensor connected over I2C.
* `arducamframeserver` - Captures QVGA JPEG images using ArduCAM Mini and returns them in an HTTP response.
* `arducampreview` - Captures uncompressed 16-bit RGB QVGA images using ArduCAM Mini and displays them on an ILI9341 display
* `arducamstreamserver` - Continuously captures JPEG images using ArduCAM Mini and returns in a multipart MIME stream. Image dimensions specified with URL query parameters.
* `cryptdigest` - calculates the SHA1 hash of two strings and converts the result to Base64 encoding.
* `dnsresolve` - asynchronously resolves multiple domain names to IP addresses
* `helloworld` - a starter application that breaks into the debugger, if available, and then traces "Hello, world." to the console.
* `httpget` - HTTP request that retrieves the result to a `String`.
* `httpgetjson` -  HTTP request retrieves the result to a `JSON` object.
* `httpgetstreaming` -  HTTP request for a result that is too big to fit in memory. The result is retrieved in fragments.
* `httpgetxml` -  HTTP request retrieves an XML response and extracts data using the XML class.
* `httppost` -  HTTP request that uses the POST method to send a JSON object in the request body.
* `httpserver` -  HTTP server that responds by echoing the request path.
* `httpserverchunked` -  HTTP server that responds with a series of random integers using `transfer-encoding` of chunked.
* `httpserverputfile` -  HTTP server that implements file upload for PUT requests.
* `net` - examples of retrieving network and Wi-Fi status.
* `pubnub` - use module to publish and subscribe to a [PubNub](https://www.pubnub.com) realtime channel.
* `sntp` - retrieve the current time using the SNTP protocol
* `socketlistener` -  a trivial HTTP server implemented using the `Socket` and `Listener` classes.
* `socketreadwrite` - a trivial HTTP request implemented using the `Socket` class.
* `timers` - shows how to use the three types of timers: immediate, one-shot, and repeating.
* `uuid` - shows how to generate a UUID.
* `websocketclient` - a WebSocket client exchanges JSON messages with an echo server.
* `websocketserver` - a WebSocket echo server
* `xml` - parse and serialize XML documents

## class Timer

The Timer module provides both time based callbacks and a tick counter. The Timer module exports a class with only static functions, so it does not need to be instantiated using `new`.

	import Timer from "timer";

	let ms = Timer.ticks();

### Timer.ticks()

The `ticks` function returns the value of a millisecond counter. The value returned does not correspond to the time of day. The milliseconds are used to calculate time differences.

	let start = Timer.ticks();
	for (let i = 0; i < 1000; i++)
		;
	let stop = Timer.ticks();
	trace(`Operation took ${stop - start} milliseconds\n`);

### Timer.set(callback[, interval])

The `set` function is used to request a function be called once after a certain period.

An immediate timer is called on the next cycle through the run loop. To set an immediate timer, call `set` with a single argument.

	Timer.set(id => trace("immediate fired\n");

A one shot timer is called once after a specified number of milliseconds. If the number of milliseconds is zero, a one shot timer is equivalent to an immediate timer.

	Timer.set(id => trace("one shot fired\n"), 1000);

The callback function receives the timer id as the first argument.

### Timer.repeat(callback, interval)

A repeating timer is called continuously until stopped using the `Timer.clear` function. 

	Timer.repeat(id => trace("repeat fired\n"), 1000,;

The callback function receives the timer id as the first argument.

### Timer.clear(id)

The `clear` function cancels a timer. The `Timer.set` and `Timer.repeat` functions returns the ID for a timer, which is then passed to clear.

	let aTimer = Timer.set(id => trace("one shot\n"), 1000);
	Timer.clear(aTimer);

> **Note**: Immediate and one shot timers are automatically cleared after invoking their callback function. There is no need to call `clear` except to cancel the timer before it fires.

## class Socket

The Socket object implements a network connection using a TCP or UDP socket.

	import {Socket, Listener} from "socket";

### HTTP GET

The following sample shows using the Socket object to make a simple HTTP GET request, tracing the response, including headers, to the console. This example is not intended as a useful HTTP client.

	let host = "www.example.com";
	let port = 80;
	let socket = new Socket({host, port});

	socket.callback = function(message, value)
	{
		if (1 == message) {
			this.write("GET / HTTP/1.1\r\n");
			this.write("Host: ", host, "\r\n");
			this.write("Connection: close\r\n");
			this.write("\r\n");
		}
		else if (2 == message)
			trace(this.read(String));
	}

### new Socket(dictionary)

The `Socket` constructor takes a single argument, a object dictionary of initialization parameters. The constructor immediately initiates a connection to the remote host.

If the IP address is known, use the `address` property in the dictionary.

	let socket = new Socket({address "17.172.224.47", port: 80});

To initiate a connection to a remote server specified by a host name, include `host` and `port` properties in the dictionary. The socket resolves the host name to an IP address.

	let host = "www.moddable.tech";
	let port = 80;
	let socket = new Socket({host, port});

By default a new socket uses TCP. The socket kind can be set in the dictionary:

	let tcp = new Socket({host: "moddable.tech", port: 1234, kind: "TCP"});

	let udp = new Socket({port: 123, kind: "UDP"});

To accept a new connection request from a `Listener`, specify the `listener` property in the dictionary:

	let socket = new Socket({listener});

### close()

The `close` function immediately terminates the socket, freeing all resources associated with the socket.

### read(type [, until])

The `read` function receives data from the socket. Data is only available to be read from inside the callback function when it receives a `data` message; attempts to read data at other times will fail.

To read all available data into a `String`:

	let string = this.read(String);
	
To read all available data into an `ArrayBuffer`:

	let buffer = this.read(ArrayBuffer);

To read one byte into a `Number`:

	let byte = this.read(Number);

To read 12 bytes into a `String` or `ArrayBuffer`:

	let string = this.read(String, 12);
	let buffer = this.read(ArrayBuffer, 12);

To read up to the next space character into `String` or `ArrayBuffer`. If there is no space character found, the remainder of the available data is read:

	let string = this.read(String, " ");
	let buffer = this.read(ArrayBuffer, " ");

To skip data in the read buffer, read to `null`:

	this.read(null, 5);		// skip ahead 5 bytes

To skip to the next carriage-return (or the end of the buffer, if none found):

	this.read(null, "\n");

When reading to `null`, the return value is the count of bytes skipped.

To determine the number of available bytes remaining in the buffer, call read with no arguments:

	let bytesAvailable = this.read();

### write(data [, data1, ...])

The `write` function sends data on the socket. One or more arguments may be passed to `write` for transmission.

For a TCP socket, all parameters are data to be transmitted.

	socket.write("Hello");
	socket.write(32);
	socket.write("world.", 13);

	socket.write(JSON.stringify(obj));

`String` and `ArrayBuffer` values are transmitted as-is. A `Number` value is transmitted as a byte.

If the socket has insufficient buffer space to transmit the data, none of the data is sent. To determine the number of bytes that can be transmitted, call `write` with no arguments:

	let bytesToSend = socket.write();

For a UDP socket, the first two parameters are the IP address and port to transmit the packet to. The third parameters is the data to transmit as an `ArrayBuffer`:

	socket.write("1.2.3.4", 1234, packet);

### callback(message [, value])

The user of the socket receives status information through the callback function. The callback receives messages and, for some messages, a data value:

* `error` (-2): An error occurred. The socket is no longer usable. 
* `disconnect` (-1): The socket disconnected from the remote host.
* `connect` (1): The socket successfully connected to the remote host.
* `dataReceived` (2): The socket has received data. The `value` argument contains the number of bytes available to read.
* `dataSent` (3): The socket has successfully transmitted some or all of the data written to it. The `value` argument contains the number of bytes that can be safely written.

## class Listener

The `Listener` class implements a network socket listener to accept new connections. The `Listener` class is used together with the `Socket` class.

	import {Socket, Listener} from "socket";

> **Note**: At this time, the `Listener` object supports only TCP sockets, not UDP.

### HTTP Server

The following example implements a trivial HTTP server using `Listener` and `Socket`. The server is truly trivial, not even parsing the client request.

	let count = 0;
	let listener = new Listener({port: 80});
	listener.callback = function() {
		let socket = new Socket({listener});
		let message = `Hello, server ${++count}.`;
		socket.write("HTTP/1.1 200 OK\r\n");
		socket.write("Connection: close\r\n");
		socket.write(`Content-Length: ${message.length}\r\n`);
		socket.write("Content-Type: text/plain\r\n");
		socket.write("\r\n");
		socket.write(message);
		socket.close();
	}

### new Listener(dictionary)

The `Listener` constructor takes a single argument, a object dictionary of initialization parameters.

To listen, use the `port` property to specify the port to listen on:

	let telnet = new Listener({port: 23});

### callback()

The user of the listener is notified through the callback function. The callback function accepts the connection request and instantiates a new socket by invoking the Socket constructor with the listener instance.

	telnet.callback = function() {
		let socket = new Socket({listener: this});
		...
	}

## class HTTP Request

The HTTP class implements a client for making HTTP requests. It is built on the Socket class. Like the Socket class, the HTTP Request uses a dictionary based constructor and a single callback.

	import {Request} from "http"

<!-- Maybe body property should be named request to parallel response. And HTTP Request should be renamed HTTP Client -->

### new Request(dictionary)

A new HTTP Request is configured using a dictionary of properties. The dictionary is a super-set of the `Socket` dictionary.

To request the root "/" resource on port 80 as a `String`:

	let request = new Request({host: "www.example.com", response: String});

To request the "/info.dat" resource from port 8080 as an `ArrayBuffer`:

	let request = new Request({host: "www.example.com", path: "/info.dat", port: 8080, response: ArrayBuffer});

To request the "/weather.json" resource from a device with IP address "192.0.1.15" as a `JSON` object:

	let request = new Request({address: "192.0.1.15", path: "/weather.json", response: JSON});

To issue a DELETE request, set the `method` property in the dictionary:

	let request = new Request({address: "192.0.1.15", path: "/resource/to/delete", method: "DELETE"});

The complete list of properties the HTTP Request adds to the Socket dictionary is:

- `port` - The remote port number. Defaults to 80.
- `path` - The path, query, and fragment portion of the HTTP URL. Defaults to `/`.
- `method` - The method to use for this HTTP request. Defaults to `GET`.
- `headers` - An array containing the HTTP headers to add. Even number elements are header names and odd number elements are the corresponding header values. Defaults to an empty array (e.g. `[]`).
- `body` - Request body contents. Provide a `String` or `ArrayBuffer` with the complete body. Set to `true` to provide the request body in fragments via the callback. Defaults to `false`.
- `response` - The type of object to use for the response body passed to the callback when the request is complete. May be set to `String`, `ArrayBuffer`, `JSON`, or `undefined`. If set to `undefined`, the response body is delivered to the callback in fragments upon arrival. Defaults to `undefined`.

### close()

The `close` function immediately terminates the HTTP request, freeing the socket and any other associated memory.

### read(type [, until])

The `read` function behaves exactly like the read function of the `Socket` class. The `read` function can only be called inside the callback providing a response body fragment.

> **Note**: The HTTP read function does not currently support passing a `String` for the `until` argument on a response that uses chunked transfer-encoding.

### callback(message, val1, val2)

- `0` - Get request body fragment. This callback is only received if the `body` property in the dictionary is set to `true`. When called, `val1` is the maximum number of bytes that can be transmitted. Return either a `String` or `ArrayBuffer` containing the next fragment of the request body. Return `undefined` when there are no more fragments.
- `1` - Response status received with status code. This callback is invoked when the HTTP response status line is successfully received. When called, `val1` is the HTTP status code (e.g. 200 for OK).
- `2` - One header received. The callback is called for each header in the response. When called, `val1` is the header name in lowercase letters (e.g. `connection`) and `val2` is the header value (e.g. `close`).
- `3` - All headers received. When all headers have been received, the callback is invoked.
- `4` - Response body fragment. This callback is invoked when a fragment of the complete HTTP response body is received. `val1` is the number of bytes in the fragment which may be retrieved using the `read` function. This callback only invoked if the `response` property value is `undefined`.
- `5` - All response body received. This callback is invoked when the entire response body has been received. If the `response` property value is not  `undefined`, `val1` contains the response.

## class HTTP Server

The HTTP Server class implements a server to response to HTTP requests. It is built on the Socket class. Like the Socket class, the HTTP Server uses a dictionary based constructor and a single callback.

	import {Server} from "http"

### Simple HTTP server

The following example implements an HTTP server that responds to all requests by echoing the requested path.

	(new Server({})).callback = function(message, value) {
		switch (message) {
			case 2:		// HTTP status line received
				this.path = value;
				break;

			case 8:		// prepare response body
				return {headers: ["Content-type", "text/plain"], body: this.path};
		}
	}

The server instance has a single callback function which responds to messages corresponding to the steps in fulfilling an HTTP request. A new request instance is created for each request, so the callback receives a unique "this" for each request. In this example, when the HTTP status line of a new request is received (message 2), the callback stores the path of the request. When the server is ready to transmit the body of the response (message 8), the callback returns the HTTP headers and response body (the path, in this case). The server adds the Content-Length header.

### HTTP Server with chunked response

The following example implements an HTTP server that responds to requests with a sequence of random numbers of random length.

	(new Server({})).callback = function(message, value) {
		switch (message) {
			case 8:	// prepare response body
				return {headers: ["Content-type", "text/plain"], body: true};
			
			case 9:	// provide response body fragment
				let i = Math.round(Math.random() * 20);
				if (0 == i)
					return;
				return i + "\n";
		}
	}

In this example, when the server is ready to transmit the response body (message 8), the callback returns the HTTP headers, and `true` for the body indicating the response body will be provided in fragments. In this case, the server adds a `Transfer-encoding` header with the value `chunked`. When the server is ready to transmit the next chunk of the response, the callback is invoked (message 9) to return the chunk. In this example, it returns a random number. When the random number is 0, the server returns `undefined` indicating the request is complete.

### HTTP Server receiving a JSON PUT

The following example implements an HTTP server that receives a JSON request, and echoes the JSON back in the response body.

	(new Server({})).callback = function(message, value) {
		switch (message) {
			case 4:		// request headers received, prepare for request body
				return JSON;

			case 6:		// request body received
				this.jsonRequest = value;
				trace("received JSON: ");
				trace(JSON.stringify(value));
				trace("\n");
				break;

			case 8:		// prepare response body
				return {headers: ["Content-type", "application/json"],
							body: JSON.stringify(this.jsonRequest)};
		}
	}

The callback is invoked when the request headers have been received (message 4), and returns JSON indicating it wants to receive the request body as a JSON object. When the request body has been received and parsed to a JSON object, the callback is invoked (message 6). The callback retains a reference to the JSON object in the `jsonRequest` property of the request instance. When the callback is invoked to transmit the response body (message 8), it serializes the JSON object to a string to transmit as the message body.

### HTTP Server streaming PUT to file

The following example implements an HTTP server that receives PUT requests, and streams the request body to a file using the HTTP request path as the local file path.

	import {File} from "file";

	(new Server({})).callback = function(message, value) {
		switch (message) {
			case 2:								// request status received
				let path = value;				// file path is HTTP path
				File.delete(path);
				this.file = new File(path, true);
				break;

			case 4:								// prepare for request body
				return true;					// provide request body in fragments
		
			case 5:								// request body fragment
				this.file.write(this.read(ArrayBuffer));
				break;

			case 6:								// request body received
				this.file.close();
				break;
		}
	}

To try the code, use the `curl` tool as follows, substituting the file path and IP address as necessary:

	curl --data-binary "@/users/hoddie/projects/test.txt"  http://192.168.1.37/test.txt

### new Server(dictionary)

A new HTTP server is configured using a dictionary of properties. The dictionary is a super-set of the `Socket` dictionary.

To open an HTTP server, on the default port (80):

	let server = new Server({});

To open an HTTP server on port 8080:

	let server = new Server({port: 8080});

### close()

The `close` function immediately terminates the HTTP server, freeing the server listener socket and any other associated memory.

> **Note:** The `close` function does not close active connections to the server.

### callback(message, val1, val2)

A new HTTP request is instantiated for each incoming request. The callback is invoked with `this` set to the callback instance for the request. The callback function may attach properties related to handling a specific request to `this`, rather than using global variables, to ensure there are no state collisions when there are multiple active requests.

- `-1` -- Disconnected. The request disconnected before the complete response could be delivered. Once disconnected, the request is closed by the server.
- `1` -- New connection received. A new requested has been accepted by the server.
- `2` -- Status line of request received. The `val1` argument contains the request path (e.g. `index.html`) and `val2` contains the request method (e.g. `GET`).
- `3` -- One header received. A single HTTP header has been received, with the header name in lowercase letters in `val1` (e.g. `connection`) and the header value (e.g. `close`) in `val2`.
- `4` -- All headers received. All HTTP headers have been received.
- `8` -- Prepare response. The server is ready to send the response. Callback returns a dictionary with the response status (e.g. 200) in the `status` property, HTTP headers in an array on the `headers` property, and the response body on the `body` property. If the status property is missing, the default value of `200` is used. If the body is a `String` or `ArrayBuffer`, it is the complete response. The server adds the `Content-Length` HTTP header. If the body property is set to `true`, the response is delivered using the `Transfer-encoding` mode `chunked`, and the callback is invoked to retrieve each response fragment.
- `9` -- Get response fragment. The server is ready to transmit another fragment of the response. The val1 argument contains the number of bytes that may be transmitted. The callback returns either a `String` or `ArrayBuffer`. When all data of the request has been returned, the callback returns `undefined`.
- `10` -- Request complete. The request has successfully completed.

## class WebSocket Client

The WebSocket Client class implements a client for communicating with a WebSocket server. It is built on the Socket class. Like the Socket class, the WebSocket Client uses a dictionary based constructor and a single callback.

	import {Client} from "websocket"

The WebSocket client implementation is designed for sending and receiving small messages. It has the following limitations:

- Each message must be a single frame. Fragmented messages are not supported.
- Messages are not masked when sent.

### new Client(dictionary)

A new WebSocket Client is configured using a dictionary of properties. The dictionary is a super-set of the `Socket` dictionary.

To connect to a server on port 80 at the root path "/":

	let ws = new Client({host: "echo.websocket.org"});

To connect to a server by IP address on port 8080:

	let ws = new Client({address: "174.129.224.73", port: 8080});

The complete list of properties the WebSocket Client adds to the Socket dictionary is:

- `port` - The remote port number. Defaults to 80.
- `path` - The path, query, and fragment portion of the HTTP URL. Defaults to `/`.

### close()

The `close` function immediately terminates the WebSocket connection, freeing the socket and any other associated memory.

### write(message)

The write function transmits a single WebSockets message. The message is either a `String`, which is sent as a text message, or an `ArrayBuffer`, which is sent as a binary message.

	ws.write("hello");
	ws.write(JSON.stringify({text: "hello"}));

### callback(message, value)

- `1` - Socket connected. This callback is received when the client has connected to the WebSocket server.
- `2` - WebSocket handshake complete. This callback is received after the client has successfully completed the handshake with the WebSocket server to upgrade from the HTTP connection to a WebSocket connection.
- `3` - Message received. This callback is received when a complete new message arrives from the server. The `value` argument contains the message. Binary messages are delivered in an `ArrayBuffer` and text messages in a `String`.
- `4` - Closed. This callback is received when the connection closes, either by request of the server or a network error. `value` contains the error code, which is 0 if the connection was closed by the server and non-zero in the case of a network error.

## class WebSocket Server

The WebSocket Server class implements a server for communicating with WebSocket clients. It is built on the Socket class. Like the Socket class, the WebSocket Server uses a dictionary based constructor and a single callback.

	import {Server} from "websocket"

The WebSocket server implementation is designed for sending and receiving small messages. It has the following limitations:

- Each message must be a single frame. Fragmented messages are not supported.

### new Server(dictionary)

A new WebSocket Server is configured using a dictionary of properties. The dictionary is a super-set of the `Listener` dictionary. The server is a Socket Listener. If no port is provided in the dictionary, port 80 is used.

At this time, the WebSocket Server does not define any additional properties for the dictionary.

<!-- to do: WebSocket subprotocol -->

### close()

The `close` function immediately terminates the WebSocket server listener, freeing the socket and any other associated memory. Active connections remain open.

### write(message)

The write function transmits a single WebSockets message. The message is either a `String`, which is sent as a text message, or an `ArrayBuffer`, which is sent as a binary message.

	ws.write("hello");
	ws.write(JSON.stringify({text: "hello"}));

### callback(message, value)

The WebSocket server callback is the same as the WebSocket client callback with the exception of the "Socket connected" (`1`) message. The socket connected message for the server is invoked when the server accepts a new incoming connection. The value of `this` is unique for each new server connect to a client. Like the WebSocket client callback, messages cannot be sent until after the callback receives the WebSocket handshake complete message.

>**Note**: Text and binary messages received with the mask bit set are unmasked by the server before delivering them to the callback.

## class Digest (Crypt)

The Digest class creates cryptographic hashes using a variety of algorithms.

	import {Digest} from "crypt";

### MD5 Hash

	let digest = new Digest("MD5");
	digest.write("hello, world);
	trace(`MD5 Hash: ${digest.close()}\n`);

### SHA1 Hash

	let digest = new Digest("SHA1");
	digest.write("hello,");
	digest.write(" world");
	trace(`SHA1 Hash: ${digest.close()}\n`);

### new Digest(type)

The `Digest` constructor takes the type of the hash to calculate as its sole argument.

	let digest = new Digest("SHA1");

The following hash functions are supported:

* MD5
* SHA1
* SHA224
* SHA256
* SHA384
* SHA512

### write(message)

The `write` function adds a message to the hash being calculated. There is no restriction on the length of the message. The message argument to write may be a `String` or `ArrayBuffer`. The `write` function may be called more than once for a given digest calculation.

	digest.write("123");
	digest.write("456");

### close()

The `close` function returns the the calculated hash. The hash is returned as an `ArrayBuffer`. The `close` function may only be called once, as it frees all resources associated with the digest calculation.

## class Base64

The `Base64` class provides static functions to encode and decode using the Base64 algorithm defined in [RFC 4648](https://tools.ietf.org/html/rfc4648).

	import Base64 from "base64";

### decode(str)

The `decode` function takes a `String` encoded in Base64 and returns an `ArrayBuffer` with the decoded bytes.

	trace(Base64.decode("aGVsbG8sIHdvcmxk") + "\n");
	// output: "hello, world"

### encode(data)

The `encode` function takes a `String` or `ArrayBuffer` and returns an `ArrayBuffer` containing the data with Base64 encoding applied.

	trace(Base64.encode("hello, world") + "\n");
	// output: "aGVsbG8sIHdvcmxk"

> **Note**: When a string is provided, its contents are treated as UTF-8 encoded characters when performing Base64 encoding.

## class Hex

The `Hex` class provides static functions to convert between an `ArrayBuffer` and hexadecimal encoded `String` values.

	import Hex from "hex";

### toBuffer(string [, separator])

The `toBuffer` function converts a [hexadecimal](https://en.wikipedia.org/wiki/Hexadecimal) encoded `String`, with optional separator, to an `ArrayBuffer`.

	let b0 = Hex.toBuffer("0123456789abcdef");
	let b1 = Hex.toBuffer("01:23:45:67:89:AB:CD:EF", ":");

The hexadecimal digits may be uppercase or lowercase. If the optional separator argument is provided, it must appear between each pair of hexadecimal digits.

### toString(buffer [, separator]);

The `toString` function converts an `ArrayBuffer` to a hexadecimal encoded `String`.

	let buffer = Hex.toBuffer("0123456789abcdef");
	let s0 = Hex.toString(buffer, ".");
	// s0 is 01.23.45.67.89.AB.CD.EF
	let s1 = Hex.toString(buffer);
	// s1 is 0123456789ABCDEF

## function UUID

The `UUID` function is available from the UUID module:

	import UUID from "uuid";

The function generates a new [UUID](https://en.wikipedia.org/wiki/Universally_unique_identifier). There are no arguments.

	trace(`UUID: ${UUID()}\n`);

## class Net

The Net class provides access to status information about the active network connection.

	import Net from "net";

### get(property)

The `get` function returns properties of the active network connection.

	trace(`Connect to Wi-Fi access point: ${Net.get("SSID")\n`);

The following properties are available:

- `IP` - the IP address of the network connection as a String in the form "10.0.1.4"
- `MAC` - the MAC address of the device as String in the form "A4:D1:8C:DB:C0:20"
- `SSID` - the name of the Wi-Fi access point as a String (e.g. "Moddable Wi-Fi")
- `BSSID` - the MAC address of the Wi-Fi access point as a String in the form "18:64:72:47:d4:32"
- `RSSI` - the Wi-Fi [received signal strength](https://en.wikipedia.org/wiki/Received_signal_strength_indication) as Number

### resolve(host, callback)

The `resolve` function performs performs an asynchronous [DNS](https://en.wikipedia.org/wiki/Domain_Name_System) look-up for the specified `host` and invokes the `callback` to deliver the result.

	Net.resolve("moddable.tech", (name, address) => trace(`${name} IP address is ${address}\n`);

The IP address is provided as a `String` in dotted IP address notation. If `host` cannot be resolved, the `address` parameter is `undefined`.

The DNS implementation in lwIP supports a limited number of simultaneous DNS look-ups. The number depends on the specific platform deployment. On the ESP8266 it is 4. If the DNS resolve queue is full, resolve throws an exception.

## class WiFi

The `WiFi` class provides access to use and configure the Wi-Fi capabilities of the host device.

	import WiFi from "wifi";

> **Note**: The `WiFi` class implements a Wi-Fi client (e.g. station mode). A future update will support access point mode.

### Scanning for available access points

The following example performs a scan for access points, tracing their name (SSID) as they are found.

	WiFi.scan({}, item => {
		if (item)
			trace(item.ssid + "\n");
		else
			trace("scan complete.\n");
	});

### Connecting to an access point

The following example initiates a connection to an access point. A repeating timer polls to determine when the connection attempt is complete.

	WiFi.connect({ssid: "moddable", password: "tech"});

	Timer.repeat(id => {
		let status = WiFi.status();
		if (1 == status)
			return;		// still trying to connect

		Timer.clear(id)
		if (5 == status)
			trace("connection established\n");
		else
			trace("connection failed\n");
	}, 100);

### connect(dictionary)

The `connect` static function initiates a connection attempt to the Wi-Fi access point specified in the dictionary.

	// connect to an open access point
	WiFi.connect({ssid: "guest");

	// connect to a password protected access point
	WiFi.connect({ssid: "private, password: "secret");

To disconnect from the current access point, call `connect` with no arguments.

	WiFi.connect();

> **Note:** A future update will support connecting to an access point by  BSSID.

### scan(dictionary, callback)

The `scan` static function initiates a scan for available Wi-Fi access points.   The callback function is invoked once for each access point found. When the scan is complete, the callback function is invoked a final time with a `null` argument.

	WiFi.scan({}, item => {
		if (item)
			trace(`name: ${item.ssid}, password: ${item.authentication != "none"}\n`, rssi: {$item.rssi});
	});

> **Note** The dictionary parameter is currently unused. In the future it will be used to filter the scan results.

### status()

The `status` static function returns an integer indicating the current state of the Wi-Fi connection.

- 0: Idle
- 1. Connecting
- 2. Password not accepted by access point
- 3. Specified access point not found
- 4. Connection attempt failed
- 5. Connection established

## class SNTP

The [SNTP](https://en.wikipedia.org/wiki/Network_Time_Protocol#SNTP) class provides implements an SNTP client ([RFC 4330](https://tools.ietf.org/html/rfc4330)) to retrieve a real time clock value.

### Retrieving the time
The following example retrieves the current time value from the NTP server at 216.239.36.15.

	import SNTP from "sntp";
	
	let sntp = new SNTP("216.239.36.15");
	sntp.callback = function(message, value) {
		if (1 == message)
			trace(`time value is ${value}\n`);
	}

The SNTP constructor accepts a IP address as the sole parameter. To use a domain name, use `Net.resolve` to resolve the IP address, and then invoke SNTP.

	Net.resolve("pool.ntp.org", (name, address) => {
		(new SNTP(address)).callback = function(message, value) {
			if (1 == message)
				trace(`time value is ${value}\n`);
		}
	});

### constructor(address)

The constructor accepts the IP address of the SNTP server as a `String`.

### apply(time)

The apply function sets the current time of the device to the time value specified.

	(new SNTP("216.239.36.15")).callback = function(message, value) {
		if (1 == message)
			this.apply(value);
	}

> **Note**: The `apply` function will move to a system object rather than remaining part of SNTP.
 
### start()

The SNTP constructor calls the `start` function to initiate retrieval of the current time using SNTP. Subclasses of SNTP defer initiation of the SNTP retrieval by overriding `start`.

### callback(message, value)

The `callback` receives the message parameter as a `Number`. The following messages are provided:

- `1` -- Time retrieved. The `value` parameter is the time in seconds since 1970, appropriate for passing to the Date constructor
- `-1` -- Unable to retrieve time. The value parameter may contain a `String` with the reason for the failure.

## class Digital

The `Digital` class provides access to the GPIO pins.

	import {Digital} from "pins";

Pin numbers are device dependent.
	
The Digital class provides only static functions. It is not instantiated.

### Reading a button

The following example configures pin 0 as an input and then tests to see if a button connected to the input is pressed. On an ESP8266 NodeMCU board pin 0 is the built-in user button.

	Digital.configure(0, 1);		// built in button
	if (Digital.read(0))
		trace("button is pressed\n");

### Blinking an LED

The following example configures pin 5 as an output and then blinks it one per second. On the ESP8266 NodeMCU board, pin 5 is the built-in LED.

	Digital.configure(5, 0);

	let blink = 1;
	Digital.write(5, blink);
	
	Timer.repeat(id => {
		blink = blink ^ 1;
		Digital.write(5, blink);
	}, 500);

### configure(pin, mode)

The `configure` function sets a pin as either an input or output. It should be called before `read` and `write`. Set the mode to 0 for an output, 1 for an input, and 2 for input with pull-up resistor.

### read(pin)

The `read` function samples the value of the specified pin, returning 0 or 1.

### write(pin)

The `write` function sets the value of the specified pin to either 0 or 1.

## class Analog

The `Analog` class provides access to the analog input pins.

	import {Analog} from "pins";

Pin numbers are device dependent.
	
The Analog class provides only static functions. It is not instantiated.

### Reading analog temperature sensor

The following example samples an analog temperature sensor, converting the result to celsius degrees. The ESP8266 NodeMCU board has a single analog input pin, analog pin number 0.

	let temperature = (Analog.read(0) / 1024) * 330;
	trace(`Temperature is ${temperature} degrees celsius\n");

The example works with a widely-available low-cost [temperature sensor](https://learn.adafruit.com/tmp36-temperature-sensor/overview).

### read(pin)

The `read` function samples the value of the specified pin, returning a value from 0 to 1023.

## class PWM

The `PWM` class provides access to the PWM output pins.

	import {PWM} from "pins";

PWM pins are Digital pins configured for output that are toggled between 0 and 1 at a specified frequency.

Pin numbers are device dependent.

The PWM class provides only static functions. It is not instantiated.

### Fan controller

The following example uses two PWM pins to control the direction and speed of a [fan motor](http://www.jameco.com/z/FAN-01-OSEPP-Fan-Motor-Module_2258970.html). The script toggles the fan between clockwise and counterclockwise rotation every three-quarters of a second. The fan motor's two PWMs are connected to Digital pins 4 and 2.

	let fan = +1;
	Digital.configure(4, 0);
	Digital.configure(2, 0);
	Timer.repeat(() => {
		fan = -fan;
		if (fan > 0) {
			PWM.write(4, 384);
			PWM.write(2, 0);
		}
		else {
			PWM.write(2, 384);
			PWM.write(4, 0);
		}
	}, 750);

### write(pin, value)

The write function sets the frequency of the PWM pin. The value is between 0 and 1023.

## class I2C

The `I2C` class provides access to the I2C bus connected to a pair of pins

	import {I2C} from "pins";

Pin numbers are device dependent.

## Reading an I2C temperature sensor

The following example instantiates an I2C connection to a [temperature sensor](https://www.sparkfun.com/products/11931) with address 0x48 connected to pins 4 and 5. 

	let sensor = new I2C({sda: 5, clock: 4, address: 0x48});
	sensor.write(0);					// set register address 0
	let value = sensor.read(2);	// read two bytes

	// convert data to celsius
	value = (value[0] << 4) | (value[1] >> 4);
	if (value & 0x800) {
		value -= 1;
		value = ~value & 0xFFF;
		value = -value;
	}
	value *= 0.0625;

	trace(`Celsius temperature: ${value}\n`);

### constructor(dictionary)

The I2C constructor takes a dictionary which contains the pins numbers to use for clock and data, as well as the I2C address of the target device.

	let sensor = new I2C({sda: 5, clock: 4, address: 0x48});

### read(count)

The `read` function reads `count` bytes from the target device, returning them in an `Array`. The maximum value of `count` is 34.

> **Note**: In the future, an `ArrayBuffer` may be optionally returned.

### write(...value)

The `write` function writes up to 34 bytes to the target device. The write function accepts multiple arguments, concatenating them together to send to the device. The values may be numbers, which are transmitted as bytes, and strings (which are transmitted as UTF-8 bytes). 

> **Note**: In the future, `ArrayBuffers` may also be used as values.

## class SMBus

The `SMBus` class implements support for the [System Management Bus](https://www.bing.com/search?q=smbus&form=APMCS1&PC=APMC) protocol, which is commonly used with I2C devices. The SMBus class is extends the `I2C` class with additional functions.

	import SMBus from "smbus";

The `SMBus` constructor is identical to the `I2C` constructor.

### Initializing a triple axis magnetometer

The following example establishes an SMBus connection to a [triple axis magnetometer](https://www.sparkfun.com/products/10530). Once connected, it checks the device ID to confirm that it is the expected device model. It them puts the device into continuous measure mode.

	let sensor = new SMBus({sda: 5, clock: 4, address: 0x1E});
	
	let id = sensor.readBlockDataSMB(10, 3);
	id = String.fromCharCode(id[0]) + String.fromCharCode(id[1]) + String.fromCharCode(id[2]);
	if ("H43" != id)
		throw new Error("unable to verify magnetometer id");

	sensor.writeByteDataSMB(2, 0);	// continuous measurement mode

### readByteDataSMB(register)

Reads a single byte of data from the specified register.

### readWordDataSMB(register)

Reads a 16-bit data value starting from the specified register. The data is assumed to be transmitted in little-endian byte order.

### readBlockDataSMB(register, count)

Reads count bytes of data starting from the specified register. Up to 34 bytes of data may be read. The data is returned in an `Array`.

### writeByteDataSMB(register, value)

Writes a single byte of data to the specified register.

### writeWordDataSMB(register, value)

Writes a 16-bit data value starting at the specified register. The value is transmitted in little-endian byte order.

### writeBlockDataSMB(register, ...value)

Writes the provided data values starting at the specified register. The value arguments are handled in the same way as the arguments to the `write` function of the `I2C` class.

## class File

The `File` class provides access to files in the [SPIFFS](https://github.com/pellepl/spiffs) file system.

	import {File} from "file";

The SPIFFS file system requires some additional memory. Including SPIFFS in the build increase RAM use by about 500 bytes. Using the SPIFFS file system requires about another 3 KB of RAM. To minimize the memory impact, the File class only instantiates the SPIFFS file system when necessary - when a file is open and when a file is deleted. The SPIFFS file system is automatically closed when no longer in use.

If the SPIFFS file system has not been initialized, it is formatted with SPIFFS_format when first used.

### Get file size

This example opens a file in read-only mode to retrieve the file's length. If the file does not exist, it is not created and an exception is thrown.

	let file = new File("test.txt");
	trace(`File length ${file.length}\n`);
	file.close();

### Read file as String

This example retrieves the entire content of a file into a `String`. If there is insufficient memory available to store the string or the file does not exist, an exception is thrown.

	let file = new File("test.txt");
	trace(file.read(String));
	file.close();

### Read file into ArrayBuffers

This example reads a file in 1024 byte ArrayBuffers. The final ArrayBuffer is smaller than 1024 when the file size is not an integer multiple of 1024.

	let file = new File("test.txt");
	while (file.position < file.length) {
		let buffer = file.read(ArrayBuffer, 1024);
	}
	file.close();

### Write string to file

This example deletes a file, opens it for write (which creates a new empty file), and then writes two `String` values to the file. The script then moves the read/write position to the start of the file, and reads the entire file contents into a single `String`, which is traced to the console.

	File.delete("test.txt");
	
	let file = new File("test.txt", true);
	file.write("This is a test.\n");
	file.write("This is the end of the test.\n");
	
	file.position = 0;
	let content = file.read(String);
	trace(content);
	
	file.close();

### new File(path [, write])

The `File` constructor opens a file for read or write. The optional write argument selects the mode. The default value for write is `false`. When opened, the file position is 0.

If the file does not exist, an exception is thrown when opening in read mode. When opening in write mode, a new file is created if it does not already exist.

### read(type [, count])

The `read` function reads from the current position. The data is read into a `String` or `ArrayBuffer` based on the value of the `type` argument. The `count` argument is the number of bytes to read. The default value of `count` is the number of bytes between the current `position` and the file `length`.

	let file = new File("preferences.json");
	let preferences = JSON.parse(file.read(String));
	file.close();

### write(value [, ...values])

The `write` function writes one or more values to the file starting at the current `position`. The values may be either a `String` or `ArrayBuffer`.

	File.delete("preferences.json");
	let file = new File("preferences.json", true);
	file.write(JSON.stringify(preferences));
	file.close();

### length property

The `length` property is a number indicating the number of bytes in the file. It is read-only.

### position property

The `position` property is a number indicating the byte offset into the file, for the next read or write operation.

### delete(path)

The static `delete` function removes the file at the specified path.

	File.delete("test.txt");

## class File Iterator

The Iterator class enumerates the files and subdirectories in a directory. 

	import {Iterator} from "file";

> **Note**: The SPIFFS file system used on ESP8266 is a flat file system, so there are no subdirectories.

### List contents of a directory

This example lists all the files and subdirectories in a directory.

	let root = new Iterator("/");
	let item;
	while (item = root.next()) {
		if (undefined == item.length)
			trace(`Directory: ${item.name}\n`);
		else
			trace(`File: ${item.name}, ${item.length} bytes\n`);
	}

The iterator's `next` function returns an object.  If the object has a `length` property, it is a file; if the `length` property is undefined, it is a directory.

### new Iterator(path)

The constructor takes as its sole argument the path of the directory to iterate over.

> **Note**: For the SPIFFS flat file system, always pass "/" for the `path` argument to the constructor. This ensures compatibility with file systems that implement directories.

### next()

The `next` function is called repeatedly, each time retrieving information about one file. When all files have been returned, the `next` function returns `undefined`. For each file and subdirectory, next returns an object. The object always contains a `name` property with the file name. If the object contains a `length` property, it references a file and the `length` property is the size of the file in bytes. If the `length` property is absent, it references a directory.

## class ZIP

The ZIP class implements read-only file system access to the contents of a ZIP file stored in memory. Typically these are stored in flash memory.

The ZIP implementation requires all files in the ZIP file to be uncompressed. The default in ZIP files is to compress files, so special steps are necessary to build a compatible ZIP file.

The [`zip`](https://linux.die.net/man/1/zip) command line tool creates uncompressed ZIP files when a compression level of zero is specified. The following command line creates a ZIP file named `test.zip` with the uncompressed contents of the directory `test`.

	zip -0r test.zip test

### Instantiate ZIP archive

A ZIP archive is stored in memory. If it is ROM, it will be accessed using a Host Buffer, a variant of an `ArrayBuffer`. The host platform software provides the Host Buffer instance through a platform specific mechanism. This example assumes the ESP8266 object provides that.

	import {ZIP} from "zip";

	let archive = new ZIP(ESP8266.getZIP());

### Reading a file from ZIP archive

The ZIP instance's file function provides an instance used to access a file. Though instantiated differently, the ZIP file instance shares the same API with the `File` class.

	let file = archive.file("small.txt");
	trace(`File size: ${file.length} bytes\n`);
	let string = file.read(String);
	trace(string);
	file.close();

### List contents of a ZIP archive's directory

The following example iterates the files and directories at the root of the archive. Often the root contains only a single directory.

	let root = archive.iterate("/");
	let item;
	while (item = root.next()) {
	    if (undefined == item.length)
	        trace(`Directory: ${item.name}\n`);
	    else
	        trace(`File: ${item.name}, ${item.length} bytes\n`);
	}

The ZIP iterator expects directory paths to end with a slash ("`/`"). To iterate the contents of a directory named "test" at the root, use the following code:

	let iterator = archive.iterate("test/");

### new ZIP(buffer)

The ZIP constructor instantiates an ZIP object to access the contents of the buffer as a read-only file system. The buffer may be either an `ArrayBuffer` or a Host Buffer.

The constructor validates that the buffer contains a ZIP archive, throwing an exception if it does not.

### file(path)

The `file` function instantiates an object to access the content of the specified path within the ZIP archive. The returned instance implements the same API as the `File` class.

### iterate(path)

The `iterate` function instantiates an object to access the content of the specified directory path within the ZIP archive. The returned instance implements the same API as the Iterator class. Directory paths end with a slash ("`/`") character and, with the exception of the root path, do not begin with a slash.

### map(path)

The `map` function returns a Host Buffer that references the bytes of the file at the specified path.

## class ArduCAM

The ArduCAM class provides access to raw image previews and JPEG image captures from the ArduCAM Mini camera.

	import ArduCAM from "arducam";

### Capture uncompressed frame

The following example captures a frame with a width of 320 and height of 240. The pixels are retrieved from the camera one scan line at a time.

	let camera = new ArduCAM({width: 320, height: 240, format: "rgb565be"});
	let pixels = new ArrayBuffer(320 * 2);
	
	camera.capture();
	for (let y = 0; y < 240; y++)
		camera.read(pixels);

### Capture JPEG frame

The following example captures a JPEG compressed frame with a width of 800 and height of 600. The compressed JPEG data is retrieved 1024 bytes at a time.

	let camera = new ArduCAM({width: 800, height: 600, format: "jpeg"});
	let data = new ArrayBuffer(1024);
	
	let bytes = camera.capture();
	while (bytes)
		bytes -= camera.read(data);

### new ArduCAM(dictionary)

The ArduCAM constructor takes a single argument, a dictionary, to configure the camera. Only a single instance of the camera may be active at a time.

The dictionary may contain the following properties:

- `width` - width of the captured image in pixels
- `height` - height of the captured image in pixels
- `format` format of the captures data. Use "jpeg" for JPEG compressed images, "rgb565le" for 16 bit little-endian 565 pixels, and "rgb565be" for 16 bit big-endian 565 pixels.

### capture()

The `capture` function records a single frame into the memory of the ArduCAM. It returns the number of bytes captured.

### read(buffer [,count])

The `read` function retrieves bytes from the recorded frame. The bytes are returned serially, from first to last, and may only be read one time for each frame captured.

The `buffer` argument is an `ArrayBuffer` to hold the bytes that are read. The actual number of bytes read is returned by the read function. The optional count argument is used to request the number of bytes to read into the start of the buffer.

### close()

The `close` function immediately releases all resources in use by the ArduCAM.

# Implementation Notes 

## List of Dependencies

The following repositories are needed to build.

* `xs6-esp` - [http://moddable.tech:3000/peter/xs6-esp](http://moddable.tech:3000/peter/xs6-esp).
* `ESP8266_RTOS_SDK` - [https://github.com/espressif/ESP8266_RTOS_SDK](https://github.com/espressif/ESP8266_RTOS_SDK).
* `Arduino core` for ESP8266 WiFi chip - [https://github.com/esp8266/Arduino/archive/2.2.0.zip](https://github.com/esp8266/Arduino/archive/2.2.0.zip). Note: this is version 2.2.0, as version 2.3.0 leaves far less free memory and crashes configuring Wi-Fi. Follow the instructions in the [README.md](https://github.com/esp8266/Arduino/blob/master/README.md) under "Download binary tools".
* `esp-open-sdk` for the cross-compiler and toolchain for the Tensilica CPU. Follow the [`esp-open-sdk` build instructions](https://github.com/pfalcon/esp-open-sdk)
* [KinomaJS](https://github.com/kinoma/kinomajs) host tools. Currently only the XS6 compiler and `xsbug6` debugger are used. Note that you'll need to make a small change to KinomaJS before building it; see below.

The following repositories are used but do not need to be downloaded. Modified versions are in the `xs6-esp/etc` directory.

* `makeEspArduino` - [https://github.com/plerup/makeEspArduino](https://github.com/plerup/makeEspArduino). A significantly modified version of this is present as `xs6-esp/Makefile`
* `spiffs_hal.cpp` - The C++ HAL layer for SPIFFS (SPI file system) have been extended with functions callable from C. These changes have been committed to the copy of Arduino Core used by the `xs6-esp` build.
* `tinyprintf` - [https://github.com/cjlano/tinyprintf](https://github.com/cjlano/tinyprintf). This code is present inline as `xs6-esp/3p/tinyprintf`.

Here is a list of locations of these projects (or copies of these projects containing the necessary patches):

* `3p/ESP8266_RTOS_SDK` -- https://github.com/espressif/ESP8266_RTOS_SDK
* `3p/esp-open-sdk` -- https://github.com/pfalcon/esp-open-sdk
* `3p/Arduino-2.2.0` -- git@moddable.tech:morrildl/Arduino.git (branch 2.2.0-xs6)
* `3p/kinomajs` -- git@moddable.tech:morrildl/kinomajs.git (branch xs6-esp)

Each of these can be `git clone`d. Note that in the case of `kinomajs`, the intent is to eventually upstream the patches, allowing the forked copy to be retired, and replaced with the upstream URL.

### Source change to Arduino Core SDK

The Arduino Core libraries need an additional small change to work around a WiFi bug.

Edit `Arduino-2.2.0/libraries/ESP8266WiFi/src/ESP8266WiFiSTA.cpp` lines 142:
	
	if(WiFi._persistent) {
		// workaround for #1997: make sure the value of ap_number is updated and written to flash
		// to be removed after SDK update
		wifi_station_ap_number_set(2);
		wifi_station_ap_number_set(1);
	
		wifi_station_set_config(&conf);

> **Note**: This appears to be a bit of voodoo but it seems to prevent the device's Wi-Fi settings from getting corrupted.

> **Note**: This code is taken from Arduino core version 2.3.0.

This patch is applied to the copy of `3p/Arduino-2.2.0` cited above.

### Copy Files to Arduino Core SDK

The Arduino Core libraries used by this project require two changes.

First, the default linker script has been modified to build xs6. The original version at `~/esp8266/tools/sdk/ld/eagle.app.v6.common.ld` must be replaced with the modified file from `xs6-esp/etc/ld/eagle.app.v6.common.ld`.

Second, a file adding support for SPIFFS as an Arduino module must be copied into the appropriate location in the Arduino Core tree, to be properly found and compiled.

Assuming the source tree layout from earlier, the following commands will accomplish this:

    cd ~/Projects
    cp xs6-esp/etc/ld/eagle.app.v6.common.ld Arduino-2.2.0/tools/sdk/ld/eagle.app.v6.common.ld
    cp xs6-esp/etc/spiffs/spiffs_hal.cpp Arduino-2.2.0/cores/esp8266/

These files are included in the patch applied to the copy of `3p/Arduino-2.2.0` cited above.

### Source change to XS6 Tools

The XS6 tools that run on your host machine (i.e. Mac or Linux) must be updated to work with XS6 for ESP. Apply two required changes by copying the following files from the `xs6-esp/sources` directory to your host `${XS6}` directory:

- Copy `xsl6.c` to make output `*.xs.c` use ROM sections where possible.
- Copy `xs6Code.c` to apply byte-code compatible with host function primitives.

After copying these two files into place, rebuild the XS6 tools on the host, as below.

    cd ~/Projects
    cp xs6-esp/sources/xs6Code.c kinomajs/xs6/sources/xs6Code.c
    cp xs6-esp/sources/xsl6.c kinomajs/xs6/sources/xsl6.c
    mkdir ~/Projects/tmp && cd ~/Projects/tmp 
    F_HOME=~/Projects/kinomajs cmake ~/Projects/kinomajs/xs6
    cmake --build ~/Projects/tmp --config Release

At this point, you should now have the XS6 host tools, and may now successfully build `xs6-esp`.

These changes are intended to be eventually upstreamed, and for now are in the copy of `3p/kinomajs` cited above.


## Initialization

The C code shows initialization of XS6 on ESP8266 using an archive. The call to `fxRunProgram` assumes that the archive contains a program at the path `test.xsb`.

	extern const unsigned char test_xsa[] ICACHE_STORE_ATTR ICACHE_RODATA_ATTR;
	extern const unsigned int test_xsa_len ICACHE_STORE_ATTR ICACHE_RODATA_ATTR;
	
	void loop() {
		xsCreation creation = {
			4 * 1024,	/* initial chunk size */
			1024,		/* incremental chunk size */
			512,		/* initial heap count */
			64,			/* incremental heap count */
			128,		/* stack count */
			512,		/* key count */   //@@
			97,			/* name modulo */
			127			/* symbol modulo */
		};
		void *archive = fxMapArchive(test_xsa, test_xsa_len, (const xsStringValue)"", NULL);
	
		xsMachine *the = xsCreateMachine(&creation, archive, (const xsStringValue)"esp8266", NULL);

		xsBeginHost(the);

		fxRunProgram(the, (const xsStringValue)"test.xsb");

		xsEndHost(the);
		xsDeleteMachine(the);

## setjmp / longjmp

XS6 uses `setjmp` and `longjmp` for exception handling. These functions are not built into the Arduino core SDK, but are part of the ESP8266 RTOS SDK. The archive tool can be used as shown below to extract these functions into a file to link with XS6.

	cd ~/Projects/ESP8266_RTOS_SDK/lib
	~/esp8266/tools/xtensa-lx106-elf/bin/xtensa-lx106-elf-ar -xv libcirom.a lib_a-setjmp.o

Note that this library is extracted automatically by the build, from the original Espressif SDK library.

## Application provided functions

The ESP8266 implementation of XS6 relies on the application to provide two functions for diagnostic output. The function `doESP_REPORT` outputs a string, which may be in ROM. `ESP_putc` outputs a single character. The simplest implementation of these functions provides no output, which may be appropriate for release builds.

	void doESP_REPORT(const char *msg) {}
	void ESP_putc(int c) {}

A more typical implementation sends the output to the Arduino serial output. `doESP_REPORT` copies the string to an internal buffer using  `espMemCpy`, to ensure the Arduino serial output can access the bytes safely when the string is in flash.

	void doESP_REPORT(const char *msg)
	{
		char buffer[128];
		char *m = (char *)msg;
		int len = espStrLen(msg);
		while (len) {
			int use = len;
			if (use > (sizeof(buffer) - 1))
				use = sizeof(buffer) - 1;
			espMemCpy(buffer, m, use);
			buffer[use] = 0;
			USE_SERIAL.printf("%s", buffer);
			m += use;
			len -= use;
		}
		USE_SERIAL.printf("\n");
		USE_SERIAL.flush();
	}
	
	void ESP_putc(int c)
	{
		char str[2] = {(char)c, 0};
		USE_SERIAL.print(str);
	}


## Diagnostic traces

For debugging convenience, a trivial trace macro is defined for native code:

	ESP_REPORT("reached this point in the code");
	ESP_REPORT_VAR(aStringPtr);

These are routed through the Arduino serial output, so execution progress can be monitored over a USB serial console on a computer.

String literals in `ESP_REPORT` are stored in flash, so they don't consume RAM.

To minimize reports, reports may be disabled:

	xESP_REPORT("this won't be compiled");

## Flash ROM

The most common PCB including the ESP8266 (including the very common NodeMCU DevKit) is the ESP-12E module, which has lots of flash - 4 MB. The second most-common variant has 1MB, which is still quite a bit. An important aspect of modifying XS6 for use on ESP8266 is to make use of this flash.

The compiler is very conservative about what data it puts into ROM. It is often necessary to explicitly mark the structure. There are macros to do this:

    static const txHostFunctionBuilder  gx_Function_prototype_builders[] ICACHE_FLASH_ATTR = {

    static const txHostFunctionBuilder ICACHE_XS6RO_ATTR gx_JSON_builders[] = {

There are some subtleties to applying these macros. In general, use the `ICACHE_FLASH_ATTR`. If that fails to link, use `ICACHE_XS6RO_ATTR`.

The flash has one challenging constraint: it must be accessed 32-bits at a time, on 32-bit boundaries. This is a consequence of the [Tensilica](https://en.wikipedia.org/wiki/Tensilica) instruction set.

Functions to access data in flash are in `xs6Host.c`:

	uint8_t espRead8(const void *addr);
	uint16_t espRead16(const void *addr);
	uint32_t espRead32(const void *addr);

To allow function builder data to reside in ROM, the `sxHostFunctionBuilder` structure has the `id` field changed from `txID` to `txIntegerID` to allow it to be accessed as a 32-bit value:

	struct sxHostFunctionBuilder {
		txCallback callback;
		txInteger length;
		txInteger id;		// force access as 4-byte read
	};

## Byte code

XS6 byte code can be executed from flash. `xs6Run.c` already uses macros in most places to retrieve byte code. The places where the byte code was accessed directly have been modified to use the macros. On ESP8266 the macros are redefined to access the byte code as 32-bit longs.

For example, `mxNextCode` changes from

	#define mxNextCode(OFFSET) { \
		mxCode += OFFSET; \
		byte = *((txU1*)mxCode); \
	}

To:

	#define mxNextCode(OFFSET) { \
		mxCode += OFFSET; \
		byte = espRead8(mxCode); \
	}

## Strings

Byte code contains strings. The virtual machine references these strings with `XS_STRING_X_KIND` slots.

C code contains strings. All strings in files with a name starting with `xs6` are moved to ROM by the make file using the [objcopy](https://www.sourceware.org/binutils/docs/binutils/objcopy.html) tool to remap the `.rodata.str1.1` section to `.irom0.str.1`. If you author a C source file that contains many strings, you should do the same.

Strings are accessed both directly (e.g. one character at a time to calculate a symbol hash) and indirectly (using `c_memcpy`, `c_strlen`, etc.). Each place a string is accessed directly has been modified to use `espRead*` functions. All functions used to indirectly access strings have been replaced with implementations that access the strings in a flash-safe manner. These functions (`espStrLen`, `espMemCpy`, `vprintf`, etc) are in `xs6Host.c` and `tinyprintf.c`.

## Symbols

XS6 supports storing symbol slots and strings in ROM. This is particularly valuable when combined with XS6 archives, as it moves nearly all memory associated with symbols out of RAM. Only symbols dynamically created at runtime are in RAM.

Because the Tensilica CPU requires ROM to be read as long-aligned long words, the slots in ROM cannot be used by XS6 directly as it would cause exceptions when accessing the `ID`, `flags`, and `kind` fields. Fortunately, not much code in XS6 need directly interact with key slots. New functions have been added to XS6 to allow most code to access symbols through function calls rather than slots. For the general case, `fxGetKey` has been modified to copy symbol slots in ROM to a temporary `keyScratch` slot. The new `fxGetKeyName` is used in many places `fxGetKey` was previously used.

All functions that create a new symbol (e.g. `fxNewName`, `fxNewNameC`, `fxNewNameX`, etc.) now return the symbol ID rather than the slot. This avoids unintentional access to a key slot in ROM. Nearly all callers of these functions use only the ID from the returned slot. The sole location in the existing code where the caller needs the slot has been modified to call `fxGetKey` to retrieve it.

Each slot, whether in ROM or RAM, also uses 4 bytes in the the `keyArray`. The XS6 common built-in symbols list is nearly 400 elements long, before any script symbols are added. This list of keys has been split into two parts. There is now a `keyArrayHost` which contains the list of keys in ROM, so `keyArray` only contains keys created dynamically at runtime. The boundary between these is the existing `keyOffset` variable, which the garbage collector uses to skip sweeping of ROM slots.

## Host Primitive type

XS6 for ESP8266 implements a new primitive type in the virtual machine. The Host Primitive allows many of the host functions of built-in objects and modules to fit in a single slot.

## To do

### Memory

* How much more can be squeezed out of predefined object functions?

### Functionality

* Enable DataView for 8 bit values
