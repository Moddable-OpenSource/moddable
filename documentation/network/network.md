# Networking
Copyright 2017-2018 Moddable Tech, Inc.

Revised: September 28, 2018

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## class Socket

The Socket object implements a non-blocking network connection using a TCP or a UDP socket.

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

The `read` function receives data from the socket. Data is only available to read inside the callback function when it receives a `data` message; attempts to read data at other times will fail.

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

To determine the number of available bytes remaining in the buffer, call `read` with no arguments:

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
	
It is more efficient to make a single `write` call with several parameters instead of multiple calls to `write`. 

### callback(message [, value])

The user of the socket receives status information through the callback function. The callback receives messages and, for some messages, a data value. Positive `message` values indicate normal operation and negative `message` values indicate an error.

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

To request the "/weather.json" resource from a device with IP address "192.0.1.15" as a `String` object:

	let request = new Request({address: "192.0.1.15", path: "/weather.json", response: String});

To issue a DELETE request, set the `method` property in the dictionary:

	let request = new Request({address: "192.0.1.15", path: "/resource/to/delete", method: "DELETE"});

The complete list of properties the HTTP Request adds to the Socket dictionary is:

- `port` - The remote port number. Defaults to 80.
- `path` - The path, query, and fragment portion of the HTTP URL. Defaults to `/`.
- `method` - The method to use for this HTTP request. Defaults to `GET`.
- `headers` - An array containing the HTTP headers to add. Even number elements are header names and odd number elements are the corresponding header values. Defaults to an empty array (e.g. `[]`).
- `body` - Request body contents. Provide a `String` or `ArrayBuffer` with the complete body. Set to `true` to provide the request body in fragments via the callback. Defaults to `false`.
- `response` - The type of object to use for the response body passed to the callback when the request is complete. May be set to `String`, `ArrayBuffer`, or `undefined`. If set to `undefined`, the response body is delivered to the callback in fragments upon arrival. Defaults to `undefined`.

### close()

The `close` function immediately terminates the HTTP request, freeing the socket and any other associated memory.

### read(type [, until])

The `read` function behaves exactly like the read function of the `Socket` class. The `read` function can only be called inside the callback providing a response body fragment.

> **Note**: The HTTP read function implentation does not currently support passing a `String` for the `until` argument on a response that uses chunked transfer-encoding.

### callback(message, val1, val2)

Non-negative message values indicate normal operation, and negative message values indicate failure.

- `0` - Get request body fragment. This callback is only received if the `body` property in the dictionary is set to `true`. When called, `val1` is the maximum number of bytes that can be transmitted. Return either a `String` or `ArrayBuffer` containing the next fragment of the request body. Return `undefined` when there are no more fragments.
- `1` - Response status received with status code. This callback is invoked when the HTTP response status line is successfully received. When called, `val1` is the HTTP status code (e.g. 200 for OK).
- `2` - One header received. The callback is called for each header in the response. When called, `val1` is the header name in lowercase letters (e.g. `connection`) and `val2` is the header value (e.g. `close`).
- `3` - All headers received. When all headers have been received, the callback is invoked.
- `4` - Response body fragment. This callback is invoked when a fragment of the complete HTTP response body is received. `val1` is the number of bytes in the fragment which may be retrieved using the `read` function. This callback only invoked if the `response` property value is `undefined`.
- `5` - All response body received. This callback is invoked when the entire response body has been received. If the `response` property value is not  `undefined`, `val1` contains the response.

## class HTTP Server

The HTTP Server class implements a server to respond to HTTP requests. It is built on the Socket class. Like the Socket class, the HTTP Server uses a dictionary based constructor and a single callback.

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
				return String;

			case 6:		// request body received
				this.jsonRequest = JSON.parse(value);
				trace("received JSON: , value, "\n");
				break;

			case 8:		// prepare response body
				return {headers: ["Content-type", "application/json"],
							body: JSON.stringify(this.jsonRequest)};
		}
	}

The callback is invoked when the request headers have been received (message 4), and returns String indicating it wants to receive the request body as a String object. When the complete request body has been received, the callback is invoked (message 6). The callback retains a reference to the JSON object in the `jsonRequest` property of the request instance. When the callback is invoked to transmit the response body (message 8), it serializes the JSON object to a string to transmit as the message body.

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

Positive message values indicate normal operation; negative message values indicate an error.

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

### Scanning for available access points

The following example performs a scan for access points, tracing their name (SSID) as they are found.

	WiFi.scan({}, item => {
		if (item)
			trace(item.ssid + "\n");
		else
			trace("scan complete.\n");
	});

### Connecting to an access point

The following example begins the process of connecting to a Wi-Fi access point and waits for the connection to succeed with an IP address being assigned to the device.

	let monitor = new WiFi({ssid: "Free Wi-Fi, password: "secret"}, msg => {
		switch (msg) {
			case "connect":
				break; // still waiting for IP address
			case "gotIP":
				trace(`IP address ${Net.get("IP")}\n`);
				break;
			case "disconnect":
				break;  // connection lost
		}
	}

### scan(dictionary, callback)

The `scan` static function initiates a scan for available Wi-Fi access points.   The callback function is invoked once for each access point found. When the scan is complete, the callback function is invoked a final time with a `null` argument.

	WiFi.scan({}, item => {
		if (item)
			trace(`name: ${item.ssid}, password: ${item.authentication != "none"}, rssi: ${item.rssi}, bssid: ${(new Uint8Array(item.bssid)).join(".")}\n`);
	});

The dictionary parameter supports two optional properties.

- `hidden` - When `true`, hidden access point are included in the scan results. Defaults to `false`.
- `channel` - The Wi-Fi channel number to scan. When this property is not present, all channels are scanned.

The Wi-Fi scan runs for a fixed period of time, approximately two seconds. During that time, not all access points may be found. It may be necessary to call scan several times to create a complete list of visible access points.

**Note**: Only one scan may be active at a time. Starting a new scan while one is still active will throw an exception.

### status

The `status` property returns an integer indicating the current state of the Wi-Fi connection.

- Idle (0) 
- Connecting (1)
- Password not accepted by access point (2)
- Specified access point not found (3)
- Connection attempt failed (4)
- Connection established (5)

### mode

The `mode` property is set to 1 for station mode (e.g. device acts as Wi-Fi client) and 2 for access point mode (e.g. device acts as Wi-Fi base station).

### connect(dictionary)

The `connect` function begins the process of establishing a connection. The connection process is asynchronous and may be monitored by polling `WiFi.status` or by creating a new WiFi instance. The dictionary contains either `ssid` or `bssid` properties indicating the base station to connect to, and an optional `password`.

To disconnect from the current Wi-Fi base station, call `connect` with no parameters.

### accessPoint(dictionary)

The `accessPoint` function configures the device as a Wi-Fi access point. Depending on the device, this may exit station mode.

The dictionary implements the following properties:

- `ssid` - A string giving the name of the access point. This parameter is required.
- `password` - An optional string indicating the password of the access point. If no password is provided, the access point will be open.
- `channel` - An optional number indicating the channel to use for the access point. Defaults to 1.
- `hidden` - An optional boolean indicating if the channel should be hidden. Defaults to false.
- `interval` - An optional number indicating the beacon interval in milliseconds. Defaults to 100.
- `max` - An optional number indicating the maximum number of simultaneous connections. Defaults to 4.

## class SNTP

The [SNTP](https://en.wikipedia.org/wiki/Network_Time_Protocol#SNTP) class implements an SNTP client ([RFC 4330](https://tools.ietf.org/html/rfc4330)) to retrieve a real time clock value.

	import SNTP from "sntp";

The SNTP client implementation fail-over mechanism allows additional servers to be queried in case of failure.

### Retrieving the time
The following example retrieves the current time value from the NTP server at `pool.ntp.org`.

	import SNTP from "sntp";
	
	new SNTP({host: "pool.ntp.org"}, (message, value) => {
		if (1 === message)
			trace(`time value is ${value}\n`);
	});

The SNTP constructor requires the host name or IP address of a time server. If a host name is provided, the SNTP client first resolves that to an IP address using `Net.resolve`.

### constructor(dictionary, callback)

The constructor accepts a dictionary which contains the host name or IP address of the SNTP server as a `String` in the dictionary's `host` property, and a callback function.

### callback(message, value)

The `callback` receives the message parameter as a `Number`. The following messages are provided:

- `2` -- Retry. The time has not yet been retrieved and the SNTP client is making an additional request.
- `1` -- Time retrieved. The `value` parameter is the time in seconds since 1970, appropriate for passing to the Date constructor
- `-1` -- Unable to retrieve time. The value parameter contains a `String` with the reason for the failure. The callback function may return a `String` with the host name or IP address of another SNTP server to try; otherwise, the SNTP client closes itself and may not be used for additional requests. See the [SNTP example](https://github.com/Moddable-OpenSource/moddable/blob/public/examples/network/sntp/main.js) for an implementation of fail-over handling.

## class DNS constants

The DNS module contains constants that are useful when implementing code that interacts directly with the DNS protocol. It is used by the DNS Parser, DNS Serializer, and mDNS implementation.

	import DNS from "dns";
	
`DNS.RR` contains constants for resource record types, such as `DNS.RR.PTR`. `DNS.OPCODE` contains values for `DNS.OPCODE.QUERY` and `DNS.OPCODE.UPDATE`. `DNS.CLASS` contains values for `DNS.CLASS.IN`, `DNS.CLASS.NONE`, and `DNS.CLASS.ANY`. `DNS.SECTION` contains values that include `DNS.QUESTION` and `DNS.ANSWER`.

## class DNS Parser

The DNS Parser class extracts JavaScript objects from a binary DNS record.

	import Parser from "dns/parser";

The DNS parser class parses and returns a single resource record at a time to minimize memory use. It has parsers for the resource data of A, AAAA, PTR, SRV, TXT resource record types.

> **Note**: The DNS Parser is a low level class used to build higher level services, such as mDNS.

### Parsing a DNS packet

DNS packets are typically received as UDP packets. The `Socket` object provides each DNS packet in an `ArrayBuffer`. The follow example creates a DNS parser instance for an `ArrayBuffer`:

	let packet = new Parser(dnsPacket);

The `Parser` constructor does not validate the packet. If the packet is invalid, errors will be reported when extracting records from it.

### Reading header fields

The parser instance has properties for the `id` and `flags` fields in the DNS packet:

	let id = packet.id;
	let flags = packet.flags;
	
### Determining the number of records

The parser instance has properties that provide the number of resource records in each section.

	let total = packet.questions + packet.answers +
					packet.authorities + packet.additionals;

### Retrieving a resource record

A JavaScript object containing a single resource record is retrieved by calling the function corresponding to the resource record's section. The following example retrieves the second question resource record (indices start at 0):

	let rr = packet.question(1);

There are also `answers`, `authorities`, and `additionals` functions.

### new Parser(buffer)

The DNS Parser constructor is initialized with an ArrayBuffer containing a single DNS packet.

No validation is performed by the constructor. Errors, if any, are reported when extracting resource records.

### questions(index)
Returns the question resource record corresponding to the index argument. Indices are numbered from 0. Returns `null` if index is greater than number of question records in the packet. 

### answers(index)
Returns the answer resource record corresponding to the index argument. Indices are numbered from 0. Returns `null` if index is greater than number of answer records in the packet. 

### authorities(index)
Returns the authority resource record corresponding to the index argument. Indices are numbered from 0. Returns `null` if index is greater than number of authority records in the packet.

### additionals(index)
Returns the additional resource record corresponding to the index argument. Indices are numbered from 0. Returns `null` if index is greater than number of additional records in the packet. 

## class DNS Serializer

The DNS Serializer class implements a DNS record serializer.

	import Serializer from "dns/serializer";

The DNS Serializer class is able to serialize A, NSEC, PTR, SRV, and TXT resource record types. Clients may perform their own serialization of other resource record types and provide the result to the DNS Serializer class to include in the generated DNS packet.

> **Note**: The DNS Serializer is a low level class used to build higher level services, such as mDNS.

### Building a DNS query

The following example uses the DNS Serializer to create a DNS packet querying for an A record for the "example.com" domain:

	import DNS from "dns";

	let serializer = new Serializer({query: true, opcode: DNS.OPCODE.QUERY});
	serializer.add(DNS.SECTION.QUESTION, "example.com", DNS.RR.A, DNS.CLASS.IN);
	let buffer = serializer.build();

The `build` function returns a DNS packet suitable for sending using the `write` function of the `Socket` class.

### new Serializer(dictionary)
The DNS Serializer constructor accepts a dictionary with properties to configure the DNS packet to be created. By default the following properties are defined

- `opcode` - The numeric value of the `opcode` header field, for example `DNS.OPCODE.QUERY`.  
- `query` - A boolean that indicates whether this packet contains a query or response. Defaults to `true`.
- `authoritative` - A boolean indicating the value of the `authoritative` bit in the header. Defaults to `false`.
- `id` - A numeric value for the ID field. Defaults to 0.

### add(section, name, type, clss, ttl, ...)
The `add` function adds a resource record to be serialized into the DNS packet. The first five arguments to `add` are the same for all resource records.

- `section` - The section to add this resource record to, `DNS.SECTION.ANSWER`.
- `name` - A string containing the DNS QNAME.
- `type` - A number representing the resource record type, e.g. `DNS.RR.A`.
- `clss` - A number containing the resource record class field value, typically `DNS.CLASS.IN`.
- `ttl` - A number containing the time-to-live value in seconds for this resource record.

The optional `data` argument is used to build the resource data portion of the resource record. If not present, the resource data is empty. If it is an `ArrayBuffer`, its contents are used for the resource data. The `data` argument is interpreted these resource record types:

- `A` - A string containing the IP address.
- `NSEC` - A dictionary with two keys. The first is `next` containing a string with the next hostname value. The second is a Uint8Array containing the bit-mask.
- `PTR` - A string with the PTR value.
- `SRV` - A dictionary with four keys. The `priority`, `weight`, and `port` fields are numbers with the value of the corresponding field. The `target` property is a string containing the name of the target. 
- `TXT` - A dictionary of key / value pairs for the TXT record. The property name is the key. Only string values are supported at this time.

### build()
The `build` function generates a DNS packet based on the previous calls to the serializer instance. The packet is returned as an `ArrayBuffer`.

> **Note**: The current implementation does not compress QNAMES, resulting in a larger DNS packet than necessary.

## class DNS Server

The DNS Server class implements a simple DNS server.

	import Server from "dns/server";

The server is indicated for use in devices in Wi-Fi access point mode that wish to act as a captive portal. The DNS server is used to direct look-ups for certain domains to an IP address, typically the device running the DNS server.

### Simple DNS server

The following example redirects all DNS look-ups to the IP address of the device running the server.

	new Server((message, value) => {
		if (1 == message)
			return Net.get("IP");
	})

### DNS server for a single host name

The following example redirects all DNS look-ups for "example.com" to the IP address of the device running the server. All other look-ups are ignored.

	new Server((message, value) => {
		if ((1 == message) && ("example.com" == value))
			return Net.get("IP");
	})

### new Server(callback)

The DNS server constructor takes a single argument, a function to call when a look-up request is received. The callback receives two arguments. The first, `message`, is set to 1 when a look-up is performed. The second argument, `value`, is set to the name to be resolved when a look-up request is made.

### close()

When the DNS server is no longer needed, call `close` to terminate it and free its resources.

## class MDNS

The MDNS class implements services for working with mDNS discovery and services. It includes claiming `.local` names, advertising mDNS service availability, and scanning for available mDNS services.

	import MDNS from "mdns";
	
### Claiming a local name

The following example shows how to claim the name "mydevice" on the local network.

	const mdns = new MDNS({hostName: "mydevice"});

The claiming process takes some time, usually under one second. Claiming the name may not succeed because the name may already be in use. An optional callback function provides status on the claim:
	
	const mdns = new MDNS({hostName: "mydevice"}, function(message, value) {
		switch (message) {
			case 1:
				trace(`MDNS - claimed hostname is "${value}"\n`);
				break;
			case 2:
				trace(`MDNS - failed to claim "${value}", try next\n`);
				break;
			default:
				if (message < 0)
					trace("MDNS - failed to claim, give up\n");
				break;
		}
	});

### Advertising an mDNS service

The following example announces the availability of an http service on the current host.

	mdns.add({
		name: "http",
		protocol: "tcp",
		port: 80,
		txt: {
			url: `/index.html`,
		}
	});

### Scanning for mDNS services

The following example continuously scans for http services available on the local network:

	mdns.monitor("_http._tcp", (service, instance) => {
		trace(`Found ${service}: ${instance.name}\n`);
	});


### new MDNS(dictionary [, callback]);

The MDNS constructor takes a dictionary to configure the mDNS instance and an optional `callback` to receive information about the instance status.

If the dictionary contains a `hostName` property, the MDNS instance will attempt to claim the name in the `.local` domain on the active network connection. The optional callback function receives information on the claiming process:

	callback(message, value)
	
The following messages are defined:

(1) Probing. If `value` is an empty string, claiming is underway; otherwise `value` contains the claimed name.

(2) Conflict found. The result of the callback function determines what happens next. If the result is undefined, a new name will be created automatically and the claiming process continues. If a string is returned, the claiming process continues with the string used as the target hostname. Returning true causes the claiming process to end.

(Any negative number) - error. Claiming process terminated.

The `hostName` is not required to monitor for available mDNS services.

### monitor(serviceType, callback)

The `monitor` function continuously scans the network for mDNS services of the type indicated by the `serviceType` parameter. The callback function is invoked for each unique service instance found:

	callback(service, instance)
	
The instance argument to the callback contains `name`, `protocol`, `port`, and `txt` properties describing the service. The callback is also invoked when the service announces changes to its TXT resource record.

### add(service)

The `add` function registers an mDNS service description to be advertised. The service record contains the following properties:

- `name` - the service's name (e.g. "http")
- `protocol` - the service's protocol (e.g. "tcp" or "udp")
- `port` - the service's port
- `txt` - an an optional JavaScript object with name value pairs used to populate the TXT resource record of the service

`add` may only be called after the hostname claiming process has completed successfullyy.

### update(service)

The `update` function tells the MDNS implementation that the contents of the TXT record have changed. This causes the new TXT record to be announced to the local network. The `service` object passed must be the same object provided to `add`.

### remove(service) or remove(serviceType)

The `remove` function is used both to unregisters the service and to cancel monitoring for a service type.

To unregister a service, pass the service description. This announces to the network that it is no longer available. The `service` object passed must be the same object provided to `add`.

To cancel monitoring for a service type, pass the name of the service type.


## class Telnet

The Telnet class implements a simple telnet server. The commands supported by the telnet server are determined by the `CLI` classes registered with the Console `module`.

### Starting a telnet server

To start a telnet server, invoke the `Telnet` constructor:

	new Telnet({port: 2300});

### new Telnet(dictionary)

The `Telnet` constructor takes a single argument, a dictionary. The dictionary has a single property, `port`, which indicates the port to listen on for new connections. If the `port` is not included in the dictionary, it defaults to 23.

### close()

When the Telnet server is no longer needed, call `close` to terminate it and free its resources.
