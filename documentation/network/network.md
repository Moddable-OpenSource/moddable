# Networking
Copyright 2017 Moddable Tech, Inc.

Revised: October 29, 2017

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## class Socket

The Socket object implements a network connection using a TCP or a UDP socket.

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

The `Listener` class implements a network socket listener to accept new TCP connections. The `Listener` class is used together with the `Socket` class.

	import {Socket, Listener} from "socket";

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

[ EXAMPLE NEEDED - see setup.js]

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
	
	let sntp = new SNTP({address: "216.239.36.15"}, (message, value) => {
		if (1 == message)
			trace(`time value is ${value}\n`);
	});

The SNTP constructor requires the IP address of a time server. To use a domain name, use `Net.resolve` to resolve the IP address, and then invoke SNTP.

	Net.resolve("pool.ntp.org", (name, address) => {
		new SNTP({address}, (message, value) => {
			if (1 == message)
				trace(`time value is ${value}\n`);
		});
	});

### constructor(dictionary, callback)

The constructor accepts a dictionary which contains the IP address of the SNTP server as a `String`, and a callback function.

### callback(message, value)

The `callback` receives the message parameter as a `Number`. The following messages are provided:

- `1` -- Time retrieved. The `value` parameter is the time in seconds since 1970, appropriate for passing to the Date constructor
- `-1` -- Unable to retrieve time. The value parameter may contain a `String` with the reason for the failure.

