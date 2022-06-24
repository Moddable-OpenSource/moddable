# Networking
Copyright 2017-2022 Moddable Tech, Inc.<BR>
Revised: March 8, 2022

## Table of Contents

* [Socket](#socket)
* [Listener](#listener)
* HTTP
	* [Request](#http-request)
	* [Server](#http-server)
* WebSocket 
	* [Client](#websocket-client)
	* [Server](#websocket-server)
* [Net](#net)
* [WiFi](#wifi)
* [SNTP](#sntp)
* DNS
	* [Constants](#dns)
	* [Parser](#dns-parser)
	* [Serializer](#dns-serializer)
	* [Server](#dns-server)
* [MDNS](#mdns)
* [Telnet](#telnet)
* [Ping](#ping)
* [MQTT](#mqtt)

<a id="socket"></a>
## class Socket

- **Source code:** [socket](../../modules/network/socket)
- **Relevant Examples:** [socket](../../examples/network/socket/socket), [socketreadwrite](../../examples/network/socket/socketreadwrite)

The `Socket` class implements a non-blocking network connection using a TCP or a UDP socket.

```js
import {Socket, Listener} from "socket";
```

### `constructor(dictionary)`

The `Socket` constructor takes a single argument, a dictionary of initialization parameters. The constructor immediately initiates a connection to the remote host.

If the IP address is known, use the `address` property in the dictionary.

```js
let socket = new Socket({address "17.172.224.47", port: 80});
```

To initiate a connection to a remote server specified by a host name, include `host` and `port` properties in the dictionary. The socket resolves the host name to an IP address.

```js
let host = "www.moddable.tech";
let port = 80;
let socket = new Socket({host, port});
```

By default a new socket uses TCP. The socket kind can be set in the dictionary:

```js
let tcp = new Socket({host: "moddable.tech", port: 1234, kind: "TCP"});
let udp = new Socket({port: 123, kind: "UDP"});
let raw = new Socket({kind: "RAW", protocol: 1});
```

To accept a new connection request from a `Listener`, specify the `listener` property in the dictionary:

```js
let listener = new Listener({port: 80});
let socket = new Socket({listener});
```
For TCP sockets, the dictionary supports two option properties:

- `noDelay` - A Boolean value to control whether the Nagle Algorithm is enabled (`TCP_NODELAY`). It is enabled by default on most platforms. For some situations, better write performance may be achieved by disabling it.

```js
	{...., noDelay: true}
```

- `keepalive` - An object to control the keep alive behavior of the socket. The `idle` and `interval` properties are in milliseconds. For example:

```js
	{...., keepalive: {idle: 60 * 1000, interval: 30 * 1000, count: 4}}
```

***

### `close()`

The `close` function immediately terminates the socket, freeing all resources associated with the socket.

```js
socket.close();
```

***

### `read(type [, until])`

The `read` function receives data from the socket. Data is only available to read inside the callback function when it receives a `data` message; attempts to read data at other times will fail.

To read all available data into a `String`:

```js
let string = this.read(String);
```

To read all available data into an `ArrayBuffer`:

```js
let buffer = this.read(ArrayBuffer);
```

To read one byte into a `Number`:

```js
let byte = this.read(Number);
```

To read 12 bytes into a `String` or `ArrayBuffer`:

```js
let string = this.read(String, 12);
let buffer = this.read(ArrayBuffer, 12);
```

To read up to the next space character into `String` or `ArrayBuffer`. If there is no space character found, the remainder of the available data is read:

```js
let string = this.read(String, " ");
let buffer = this.read(ArrayBuffer, " ");
```

To skip data in the read buffer, read to `null`:

```js
this.read(null, 5);		// skip ahead 5 bytes
```

To skip to the next carriage-return (or the end of the buffer, if none found):

```js
this.read(null, "\n");
```

When reading to `null`, the return value is the count of bytes skipped.

To determine the number of available bytes remaining in the buffer, call `read` with no arguments:

```js
let bytesAvailable = this.read();
```

***

### `write(data [, data1, ...])`

The `write` function sends data on the socket. One or more arguments may be passed to `write` for transmission.

For a TCP socket, all parameters are data to be transmitted.

```js
socket.write("Hello");
socket.write(32);
socket.write("world.", 13);
socket.write(JSON.stringify(obj));
```

`String` and `ArrayBuffer` values are transmitted as-is. A `Number` value is transmitted as a byte.

If the socket has insufficient buffer space to transmit the data, none of the data is sent. To determine the number of bytes that can be transmitted, call `write` with no arguments:

```js
let bytesToSend = socket.write();
```

For a UDP socket, the first two parameters are the IP address and port to transmit the packet to. The third parameters is the data to transmit as an `ArrayBuffer`:

```js
socket.write("1.2.3.4", 1234, packet);
```

For a RAW socket, the first parameter is IP address to transmit the packet to. The second parameter is the data to transmit as an `ArrayBuffer`:

```js
socket.write("1.2.3.4", packet);
```

It is more efficient to make a single `write` call with several parameters instead of multiple calls to `write`. 

***

### `get(what)`

The `get` method returns state information about the socket. The `what` argument is a string indicating the state requested. If the state is unavailable, `get` returns `undefined`.

| `what` | Description |
| :---: | :--- |
| `"REMOTE_IP"` | Returns the IP address of the remote endpoint. Only available for TCP sockets. |

***

### `callback(message [, value])`

The user of the socket receives status information through the callback function. The first argument to the callback is the messages identifier. Positive `message` values indicate normal operation and negative `message` values indicate an error. Depending on the message, there may be additional arguments.

| `message` | Description |
| :---: | :--- |
| -2 | **error:** An error occurred. The socket is no longer usable. |
| -1 | **disconnect:** The socket disconnected from the remote host. |
| 1 | **connect:** The socket successfully connected to the remote host.
| 2 | **dataReceived:** The socket has received data. The `value` argument contains the number of bytes available to read.
| 3 | **dataSent:** The socket has successfully transmitted some or all of the data written to it. The `value` argument contains the number of bytes that can be safely written.

For UDP sockets, the callback for `dataReceived` has three additional arguments after the message identifier . The first is the number of bytes available to read, as with TCP sockets. The second is a string containing the IP address of the sender. The third is the port number of the sender.

```js
callback(message, byteLength, remoteIP, remotePort) {}
}
```

For RAW sockets, the callback for `dataReceived` has two additional arguments after the message identifier . The first is the number of bytes available to read, as with TCP sockets. The second is a string containing the IP address of the sender.

```js
callback(message, byteLength, remoteIP) {}
```

***

### Example: HTTP GET

The following sample shows using the `Socket` object to make a simple HTTP GET request and trace the response, including headers, to the console. This example is not intended as a useful HTTP client.

```js
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
```

***

<a id="listener"></a>
## class Listener

- **Source code:** [socket](../../modules/network/socket)
- **Relevant Examples:** [socketlistener](../../examples/network/socket/socketlistener)

The `Listener` class implements a network socket listener to accept new TCP connections. The `Listener` class is used together with the `Socket` class.

```js
import {Socket, Listener} from "socket";
```

### `constructor(dictionary)`

The `Listener` constructor takes a single argument, a object dictionary of initialization parameters.

To listen, use the `port` property to specify the port to listen on:

```js
let telnet = new Listener({port: 23});
```

***

### `callback()`

The user of the listener is notified through the callback function. The callback function accepts the connection request and instantiates a new socket by invoking the Socket constructor with the listener instance.

```js
telnet.callback = function() {
	let socket = new Socket({listener: this});
	...
}
```

***

### Example: HTTP server

The following example implements a trivial HTTP server using `Listener` and `Socket`. The server is truly trivial, not even parsing the client request.

```js
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
```

***

<a id="http-request"></a>
## class HTTP Request

- **Source code:** [http](../../modules/network/http)
- **Relevant Examples:** [httpget](../../examples/network/http/httpget), [httppost](../../examples/network/http/httppost), [httpsget](../../examples/network/http/httpsget), and [many more](../../examples/network/http/)

The HTTP `Request` class implements a client for making HTTP requests. It is built on the `Socket` class. Like the `Socket` class, the HTTP `Request` uses a dictionary-based constructor and a single callback.

```js
import {Request} from "http"
```

<!-- Maybe body property should be named request to parallel response. And HTTP Request should be renamed HTTP Client -->

### `constructor(dictionary)`

A new HTTP `Request` is configured using a dictionary of properties. The dictionary is a super-set of the `Socket` dictionary.

The complete list of properties the HTTP `Request` adds to the `Socket` dictionary is:

| Parameter | Default Value | Description |
| :---: | :---: | :--- |
| `port` | 80 | The remote port number |
| `path` | `/` | The path, query, and fragment portion of the HTTP URL |
| `method` | `GET` | The method to use for this HTTP request|
| `headers` |  Defaults to an empty array (e.g. `[]`) | An array containing the HTTP headers to add. Even number elements are header names and odd number elements are the corresponding header values.
| `body` | `false` | Request body contents. Provide a `String` or `ArrayBuffer` with the complete body. Set to `true` to provide the request body in fragments via the callback. |
| `response` | `undefined` | The type of object to use for the response body passed to the callback when the request is complete. May be set to `String`, `ArrayBuffer`, or `undefined`. If set to `undefined`, the response body is delivered to the callback in fragments upon arrival. |

To request the root "/" resource on port 80 as a `String`:

```js
let request = new Request({host: "www.example.com", response: String});
```

To request the "/info.dat" resource from port 8080 as an `ArrayBuffer`:

```js
let request = new Request({host: "www.example.com", path: "/info.dat", port: 8080, response: ArrayBuffer});
```

To request the "/weather.json" resource from a device with IP address "192.0.1.15" as a `String` object:

```js
let request = new Request({address: "192.0.1.15", path: "/weather.json", response: String});
```

To issue a DELETE request, set the `method` property in the dictionary:

```js
let request = new Request({address: "192.0.1.15", path: "/resource/to/delete", method: "DELETE"});
```

***

### `close()`

The `close` function immediately terminates the HTTP request, freeing the socket and any other associated memory.

```js
request.close();
```

***

### `read(type [, until])`

The `read` function behaves exactly like the read function of the `Socket` class. The `read` function can only be called inside the callback providing a response body fragment.

> **Note**: The HTTP read function implentation does not currently support passing a `String` for the `until` argument on a response that uses chunked transfer-encoding.

***

### `callback(message, val1, val2)`

The user of the `Request` object receives status information through the callback function. The callback receives messages and, for some messages, additional data values. Non-negative `message` values indicate normal operation and negative `message` values indicate an error.


| `message` | `Request.` | Description |
| :---: | :---: | :--- |
|-2 | `error` | 
| 0 | `requestFragment` | Get request body fragment. This callback is only received if the `body` property in the dictionary is set to `true`. When called, `val1` is the maximum number of bytes that can be transmitted. Return either a `String` or `ArrayBuffer` containing the next fragment of the request body. Return `undefined` when there are no more fragments.
| 1 | `status` | Response status received with status code. This callback is invoked when the HTTP response status line is successfully received. When called, `val1` is the HTTP status code (e.g. 200 for OK).
| 2 | `header` | One header received. The callback is called for each header in the response. When called, `val1` is the header name in lowercase letters (e.g. `connection`) and `val2` is the header value (e.g. `close`).
| 3 | `headersComplete` | All headers received. When all headers have been received, the callback is invoked.
| 4 | `responseFragment` | Response body fragment. This callback is invoked when a fragment of the complete HTTP response body is received. `val1` is the number of bytes in the fragment which may be retrieved using the `read` function. This callback only invoked if the `response` property value is `undefined`.
| 5 | `responseComplete` | All response body received. This callback is invoked when the entire response body has been received. If the `response` property value is not  `undefined`, `val1` contains the response.

***

<a id="http-server"></a>
## class HTTP Server

- **Source code:** [http](../../modules/network/http)
- **Relevant Examples:** [httpserver](../../examples/network/http/httpserver), [httpserverbmp](../../examples/network/http/httpserverbmp), [httpserverchunked](../../examples/network/http/httpserverchunked), [httpserverputfile](../../examples/network/http/httpserverputfile)

The HTTP `Server` class implements a server to respond to HTTP requests. It is built on the `Socket` class. Like the `Socket` class, the HTTP `Server` class uses a dictionary-based constructor and a single callback.

```js
import {Server} from "http"
```

### `constructor(dictionary)`

A new HTTP server is configured using a dictionary of properties. The dictionary is a super-set of the `Socket` dictionary. 

To open an HTTP server, on the default port (80):

```js
let server = new Server({});
```

To open an HTTP server on port 8080:

```js
let server = new Server({port: 8080});
```

***

### `close()`

The `close` function immediately terminates the HTTP server, freeing the server listener socket and any other associated memory.

```js
server.close();
```

> **Note:** The `close` function does not close active connections to the server.

***

### `detach(connection)`

The `detach` function accepts an active HTTP connection of the server instance and removes it from the server, returning the socket instance of the connection. This is useful for implementing an HTTP endpoint that accepts both HTTP and WebSocket connections by allowing the existing connection of HTTP server to be handed off to the WebSocket server.

```js
server.detach(connnection);
```

***

### `callback(message, val1, val2)`

The user of the server receives status information through the callback function. The callback receives messages and, for some messages, additional data values. Positive `message` values indicate normal operation and negative `message` values indicate an error.

| `message` | `Server.` | Description |
| :---: | :---: | :--- |
| -1| `error`  | Disconnected. The request disconnected before the complete response could be delivered. Once disconnected, the request is closed by the server.
| 1 | `connection` | New connection received. A new requested has been accepted by the server.
| 2 | `status` | Status line of request received. The `val1` argument contains the request path (e.g. `index.html`) and `val2` contains the request method (e.g. `GET`).
| 3 | `header` | One header received. A single HTTP header has been received, with the header name in lowercase letters in `val1` (e.g. `connection`) and the header value (e.g. `close`) in `val2`.
| 4 | `headersComplete` | All headers received. All HTTP headers have been received. Return `String` or `ArrayBuffer` to receive the complete request body as an argument to the `requestComplete` message as the corresponding type; return `true` to have `requestFragment` invoked as the fragments arrrive. Return `false` or `undefined` to ignore the request body. The behavior for ohter return values is undefined.
| 5 | `requestFragment` |
| 6 | `requestComplete` |
| 8 | `prepareResponse` | Prepare response. The server is ready to send the response. Callback returns a dictionary with the response status (e.g. 200) in the `status` property, HTTP headers in an array on the `headers` property, and the response body on the `body` property. If the status property is missing, the default value of `200` is used. If the body is a `String` or `ArrayBuffer`, it is the complete response. The server adds the `Content-Length` HTTP header. If the body property is set to `true`, the response is delivered using the `Transfer-encoding` mode `chunked`, and the callback is invoked to retrieve each response fragment.
| 9 | `responseFragment` | Get response fragment. The server is ready to transmit another fragment of the response. The `val1` argument contains the number of bytes that may be transmitted. The callback returns either a `String` or `ArrayBuffer`. When all data of the request has been returned, the callback returns `undefined`.
| 10| `responseComplete`  | Request complete. The request has successfully completed.

A new HTTP `Request` is instantiated for each incoming request. The callback is invoked with `this` set to the callback instance for the request. The callback function may attach properties related to handling a specific request to `this`, rather than using global variables, to ensure there are no state collisions when there are multiple active requests.

***

### Example: Simple HTTP server

The following example implements an HTTP server that responds to all requests by echoing the requested path.

```js
(new Server({})).callback = function(message, value) {
	switch (message) {
		case 2:		// HTTP status line received
			this.path = value;
			break;

		case 8:		// prepare response body
			return {headers: ["Content-type", "text/plain"], body: this.path};
	}
}
```

The server instance has a single callback function which responds to messages corresponding to the steps in fulfilling an HTTP request. A new request instance is created for each request, so the callback receives a unique `this` for each request. In this example, when the HTTP status line of a new request is received (message 2), the callback stores the path of the request. When the server is ready to transmit the body of the response (message 8), the callback returns the HTTP headers and response body (the path, in this case). The server adds the `Content-Length` header.

***

### Example: HTTP Server with chunked response

The following example implements an HTTP server that responds to requests with a sequence of random numbers of random length.

```js
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
```

In this example, when the server is ready to transmit the response body (message 8), the callback returns the HTTP headers, and `true` for the body indicating the response body will be provided in fragments. In this case, the server adds a `Transfer-encoding` header with the value `chunked`. When the server is ready to transmit the next chunk of the response, the callback is invoked (message 9) to return the chunk. In this example, it returns a random number. When the random number is 0, the server returns `undefined` indicating the request is complete.

***

### Example: HTTP Server receiving a JSON PUT

The following example implements an HTTP server that receives a JSON request, and echoes the JSON back in the response body.

```js
(new Server({})).callback = function(message, value) {
	switch (message) {
		case 4:		// request headers received, prepare for request body
			return String;

		case 6:		// request body received
			this.jsonRequest = JSON.parse(value);
			trace(`received JSON: ${value}\n`);
			break;

		case 8:		// prepare response body
			return {headers: ["Content-type", "application/json"],
						body: JSON.stringify(this.jsonRequest)};
	}
}
```

The callback is invoked when the request headers have been received (message 4), and returns String indicating it wants to receive the request body as a String object. When the complete request body has been received, the callback is invoked (message 6). The callback retains a reference to the JSON object in the `jsonRequest` property of the request instance. When the callback is invoked to transmit the response body (message 8), it serializes the JSON object to a string to transmit as the message body.

***

### Example: HTTP Server streaming PUT to file

The following example implements an HTTP server that receives PUT requests, and streams the request body to a file using the HTTP request path as the local file path.

```js
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
```

To try the code, use the `curl` tool as follows, substituting the file path and IP address as necessary:

	curl --data-binary "@/users/hoddie/projects/test.txt"  http://192.168.1.37/test.txt

***

<a id="websocket-client"></a>
## class WebSocket Client

- **Source code:** [websocket](../../modules/network/websocket)
- **Relevant Examples:** [websocketclient](../../examples/network/websocket/websocketclient)

The WebSocket `Client` class implements a client for communicating with a WebSocket server. It is built on the `Socket` class. Like the `Socket` class, the WebSocket `Client` uses a dictionary-based constructor and a single callback.

```js
import {Client} from "websocket"
```

The WebSocket client implementation is designed for sending and receiving small messages. It has the following limitations:

- Each message must be a single frame. Fragmented messages are not supported.
- Messages are not masked when sent.

### `constructor(dictionary)`

A new WebSocket `Client` is configured using a dictionary of properties. The dictionary is a super-set of the `Socket` dictionary.

The complete list of properties the WebSocket `Client` adds to the `Socket` dictionary is:

| Property | Default Value | Description |
| :---: | :---: | :--- |
| `port` | 80 | The remote port number |
| `path` | `/` | The path, query, and fragment portion of the HTTP URL |

To connect to a server on port 80 at the root path "/":

```js
let ws = new Client({host: "echo.websocket.org"});
```

To connect to a server by IP address on port 8080:

```js
let ws = new Client({address: "174.129.224.73", port: 8080});
```

***

### `close()`

The `close` function immediately terminates the WebSocket connection, freeing the socket and any other associated memory.

```js
ws.close();
```

***

### `write(message)`

The write function transmits a single WebSockets message. The message is either a `String`, which is sent as a text message, or an `ArrayBuffer`, which is sent as a binary message.

```js
ws.write("hello");
ws.write(JSON.stringify({text: "hello"}));
```

***

### `callback(message, value)`

The user of the WebSocket client receives status information through the callback function. The callback receives messages and, for some messages, a data value. Positive `message` values indicate normal operation and negative `message` values indicate an error.

| `message` | `Client.` | Description |
| :---: | :---: | :--- |
| 1 | `connect` | Socket connected. This callback is received when the client has connected to the WebSocket server. |
| 2 | `handshake` | WebSocket handshake complete. This callback is received after the client has successfully completed the handshake with the WebSocket server to upgrade from the HTTP connection to a WebSocket connection. |
| 3 | `receive` | Message received. This callback is received when a complete new message arrives from the server. The `value` argument contains the message. Binary messages are delivered in an `ArrayBuffer` and text messages in a `String`. |
| 4 | `disconnect` | Closed. This callback is received when the connection closes, either by request of the server or a network error. `value` contains the error code, which is 0 if the connection was closed by the server and non-zero in the case of a network error. |

***

<a id="websocket-server"></a>
## class WebSocket Server

- **Source code:** [websocket](../../modules/network/websocket)
- **Relevant Examples:** [websocketserver](../../examples/network/websocket/websocketserver)

The WebSocket `Server` class implements a server for communicating with WebSocket clients. It is built on the `Socket` class. Like the `Socket` class, the WebSocket `Server` uses a dictionary-based constructor and a single callback.

```js
import {Server} from "websocket"
```

The WebSocket server implementation is designed for sending and receiving small messages. It has the following limitations:

- Each message must be a single frame. Fragmented messages are not supported.

### `constructor(dictionary)`

A new WebSocket `Server` is configured using a dictionary of properties. The dictionary is a super-set of the `Listener` dictionary. The server is a Socket Listener. If no port is provided in the dictionary, port 80 is used. If port is set to `null`, no listener is created which is useful when sharing a listener with an http server (see `attach` below).

At this time, the WebSocket `Server` does not define any additional properties for the dictionary.

```js
let ws = new Server({});
```

<!-- to do: WebSocket subprotocol -->

***

### `close()`

The `close` function immediately terminates the WebSocket server listener, freeing the socket and any other associated memory. Active connections remain open.

```js
ws.close();
```

***

### `attach(socket)`

The `attach` function creates a new incoming WebSockets connection from the provided socket. The server issues the `Server.connect` callback and then performs the WebSockets handshake. The status line has been read from the socket, but none of the HTTP headers have been read as these are required to complete the handshake. 

See the [httpserverwithwebsockets](../../examples/network/http/httpserverwithwebsockets/main.js) for an example of sharing a single listener socket between the HTTP and WebSockets servers.

***

### `callback(message, value)`

The WebSocket server callback is the same as the WebSocket client callback with the addition of the "Socket connected" (`1` or `Server.connect`) message. The socket connected message for the server is invoked when the server accepts a new incoming connection.

The value of `this` is unique for each connection made to the server. Messages cannot be sent until after the callback receives the WebSocket handshake complete message (`Server.handshake`).

The `this` instance of the callback has the same `write` and `close` methods as the WebSocket Client. These methods are used to send data and to close the connection.

>**Note**: Text and binary messages received with the mask bit set are unmasked by the server before delivering them to the callback.

***

<a id="net"></a>
## class Net

- **Source code:** [net](../../modules/network/net)
- **Relevant Examples:** [net](../../examples/network/net)

The `Net` class provides access to status information about the active network connection.

```js
import Net from "net";
```

### `static get(property [, interface])`

The `get` function returns properties of the active network connection.

The following properties are available:

| Property | Description | 
| :---: | :--- |
| `IP` | The IP address of the network connection as a `String`, e.g. "10.0.1.4". These may be IPv4 or IPv6 addresses.
| `MAC` | The MAC address of the device as a `String`, e.g. "A4:D1:8C:DB:C0:20"
| `SSID` | The name of the Wi-Fi access point connected to as a `String`, e.g. "Moddable Wi-Fi"
| `BSSID` | The MAC address of the Wi-Fi access point connected to as a `String`, e.g. "18:64:72:47:d4:32"
| `RSSI` | The Wi-Fi [received signal strength](https://en.wikipedia.org/wiki/Received_signal_strength_indication) as a `Number`
| `CHANNEL` | The Wi-Fi channel currently in use as a `Number`
| `DNS` | The DNS server(s) used to resolve domain names to IP addresses by `Net.resolve` as an `Array` of IP address strings. These may be IPv4 or IPv6 addresses.


```js
trace(`Connected to Wi-Fi access point: ${Net.get("SSID")}\n`);
```

For a device operating as both a Wi-Fi station (client) and a Wi-Fi access point, the static `get` method accepts an optional second argument to indicate if the request is for the station or access point interface. The interface accepts values of `"station"` and `"ap"`. It is used for the `IP` and `MAC` properties. 

On ESP32, the optional second argument can also be used to explicitly request information about the Ethernet interface by providing the value `"ethernet"`.

```
trace(`IP default ${Net.get("IP")}\n`);
trace(`IP station ${Net.get("IP", "station")}\n`);
trace(`IP AP ${Net.get("IP", "ap")}\n`);
```
In station mode, the default value for the interface is `"station"`; in access point mode, `"ap"`. In the combined station and access point mode, there is no default value (because it is ambiguous). Requesting the `IP` or `MAC` properties in this mode returns `undefined`.

***

### `static resolve(host, callback)`

The `resolve` function performs performs an asynchronous [DNS](https://en.wikipedia.org/wiki/Domain_Name_System) look-up for the specified `host` and invokes the `callback` to deliver the result.

```js
Net.resolve("moddable.tech", (name, address) => trace(`${name} IP address is ${address}\n`);
```

The IP address is provided as a `String` in dotted IP address notation. If `host` cannot be resolved, the `address` parameter is `undefined`.

The DNS implementation in lwIP supports a limited number of simultaneous DNS look-ups. The number depends on the specific platform deployment. On the ESP8266 it is 4. If the DNS resolve queue is full, resolve throws an exception.

***

<a id="wifi"></a>
## class WiFi

- **Source code:** [wifi](../../modules/network/wifi)
- **Relevant Examples:** [wifiaccesspoint](../../examples/network/wifi/wifiaccesspoint), [wifiscan](../../examples/network/wifi/wifiscan)

The `WiFi` class provides access to use and configure the Wi-Fi capabilities of the host device.

```js
import WiFi from "wifi";
```

### `constructor(dictionary, callback)`

The `WiFi` constructor takes a single argument, a dictionary of initialization parameters. The constructor begins the process of establishing a connection. 

The dictionary always contains the required `ssid` property with the name of the base station to connect to. The optional `password` property is included when the base station requires a password. When the optional `bssid` property is included, it may accelerate connecting to Wi-Fi on device targets that support it.

The connection process is asynchronous and may be monitored using the callback function.

The following example begins the process of connecting to a Wi-Fi access point and waits for the connection to succeed with an IP address being assigned to the device.

```js
let monitor = new WiFi({ssid: "My Wi-Fi", password: "secret"}, msg => {
	switch (msg) {
		case WiFi.connected:
			break; // still waiting for IP address
		case WiFi.gotIP:
			trace(`IP address ${Net.get("IP")}\n`);
			break;
		case WiFi.disconnected:
			break;  // connection lost
	}
});
```

The following example initiates a connection to a Wi-Fi access point with no password. Because there is no callback function to monitor connection progress, polling is necessary to determine when the connection is ready. Poll by getting the IP address of the device using the `Net` class. When there is no connection, the results is `undefined`.

```js
let monitor = new WiFi({ssid: "Open Wi-Fi"});
```

### `close()`

The `close` function closes the connection between the `WiFi` instance and the underlying process managing the device's connection to the network. In other words, it prevents future calls to the callback function, but it does not disconnect from the network.

```js
monitor.close();
```

### `static scan(dictionary, callback)`

The `scan` static function initiates a scan for available Wi-Fi access points. 

The dictionary parameter supports two optional properties:

| Property | Description |
| :---: | :--- |
| `hidden` | When `true`, hidden access point are included in the scan results. Defaults to `false`. |
| `channel` | The Wi-Fi channel number to scan. When this property is not present, all channels are scanned. |

The callback function is invoked once for each access point found. When the scan is complete, the callback function is invoked a final time with a `null` argument.

```js
WiFi.scan({}, item => {
	if (item)
		trace(`name: ${item.ssid}, password: ${item.authentication != "none"}, rssi: ${item.rssi}, bssid: ${(new Uint8Array(item.bssid)).join(".")}\n`);
	else
		trace("scan complete.\n");
});
```

The Wi-Fi scan runs for a fixed period of time, approximately two seconds. During that time, not all access points may be found. It may be necessary to call scan several times to create a complete list of visible access points.

> **Note**: Only one scan may be active at a time. Starting a new scan while one is still active will throw an exception.

***

### `mode` property

The `mode` property is set to 1 for station mode (e.g. device acts as Wi-Fi client), 2 for access point mode (e.g. device acts as Wi-Fi base station), and 3 for simultaneous operation of station and access point modes.

***

### `static connect(dictionary)`

The `connect` function begins the process of establishing a connection. The connection process is asynchronous and may be monitored by polling `Net.get("IP")` or by creating a new WiFi instance. The dictionary contains either `ssid` or `bssid` properties indicating the base station to connect to, and an optional `password`.

```js
WiFi.connect({ssid: "Moddable", password: "1234"});
```

> **Note**: Calling `WiFi.connect` with no parameters disconnects. However, it is recommended to use `WiFi.disconnect` insteadm

***

### `static disconnect()`

Disconnects from the current Wi-Fi base station

```js
WiFi.disconnect();
```
***

### `static accessPoint(dictionary)`

The `accessPoint` function configures the device as a Wi-Fi access point. Depending on the device, this may exit station mode.

The dictionary must include an `ssid` property, a string that gives the name of the access point.

The dictionary may optionally include the following properties:

| Property | Default Value | Description |
| :---: | :---: | :--- |
| `password` | none | A string indicating the password of the access point; if no password is provided, the access point will be open |
| `channel` | 1 | A number indicating the channel to use for the access point |
| `hidden` | `false` | A boolean indicating if the channel should be hidden |
| `interval` | 100 | A number indicating the beacon interval in milliseconds |
| `max` | 4 | A number indicating the maximum number of simultaneous connections |
| `station` | `false` | A boolean indicating if station mode should simultaneously be enabled with access point mode. |

```js
WiFi.accessPoint({
	ssid: "Moddable Zero",
	password: "12345678"
});
```

***

<a id="sntp"></a>
## class SNTP

- **Source code:** [sntp](../../modules/network/sntp)
- **Relevant Examples:** [sntp](../../examples/network/sntp), [ntpclient](../../examples/network/mdns/ntpclient)

The `SNTP` class implements an [SNTP](https://en.wikipedia.org/wiki/Network_Time_Protocol#SNTP) client ([RFC 4330](https://tools.ietf.org/html/rfc4330)) to retrieve a real time clock value.

```js
import SNTP from "sntp";
```

The SNTP client implementation fail-over mechanism allows additional servers to be queried in case of failure.

### `constructor(dictionary, callback)`

The SNTP constructor takes a dictionary of properties and a callback function to receive information about the instance status.

The dictionary must include a `host` property, a string that gives the host name or IP address of the SNTP server.

The callback receives messages and, for some messages, a data value. Positive `message` values indicate normal operation and negative `message` values indicate an error.

| `message` | Description |
| :---: | :--- |
| -1 | Unable to retrieve time. The value parameter contains a `String` with the reason for the failure. The callback function may return a `String` with the host name or IP address of another SNTP server to try; otherwise, the SNTP client closes itself and may not be used for additional requests. See the [SNTP example](https://github.com/Moddable-OpenSource/moddable/blob/public/examples/network/sntp/main.js) for an implementation of fail-over handling. |
| 1 | Time retrieved. The `value` parameter is the time in seconds since 1970, appropriate for passing to the Date constructor
| 2 | Retry. The time has not yet been retrieved and the SNTP client is making an additional request.

***

### Example: Retrieving the time
The following example retrieves the current time value from the NTP server at `pool.ntp.org`.

```js
new SNTP({host: "pool.ntp.org"}, (message, value) => {
	if (1 === message)
		trace(`time value is ${value}\n`);
});
```

The SNTP constructor requires the host name or IP address of a time server. If a host name is provided, the SNTP client first resolves that to an IP address using `Net.resolve`.

***

<a id="dns"></a>
## class DNS constants

- **Source code:** [dns](../../modules/network/dns)

The DNS module contains constants that are useful when implementing code that interacts directly with the DNS protocol. It is used by the DNS `Parser`, DNS `Serializer`, and mDNS implementation.

```js
import DNS from "dns";
```
	
- `DNS.RR` contains constants for resource record types, such as `DNS.RR.PTR`.
- `DNS.OPCODE` contains values for `DNS.OPCODE.QUERY` and `DNS.OPCODE.UPDATE`.
- `DNS.CLASS` contains values for `DNS.CLASS.IN`, `DNS.CLASS.NONE`, and `DNS.CLASS.ANY`. 
- `DNS.SECTION` contains values that include `DNS.QUESTION` and `DNS.ANSWER`.

<a id="dns-parser"></a>
## class DNS Parser

- **Source code:** [dnsparser](../../modules/network/dns)

The DNS `Parser` class extracts JavaScript objects from a binary DNS record.

```js
import Parser from "dns/parser";
```

The DNS parser class parses and returns a single resource record at a time to minimize memory use. It has parsers for the resource data of A, AAAA, PTR, SRV, TXT resource record types.

> **Note**: The DNS Parser is a low level class used to build higher level services, such as mDNS.

### `constructor(buffer)`
The DNS `Parser` constructor is initialized with an `ArrayBuffer` containing a single DNS packet.

No validation is performed by the constructor. Errors, if any, are reported when extracting resource records.

***

### `questions(index)`
Returns the question resource record corresponding to the index argument. Indices are numbered from 0. Returns `null` if index is greater than number of question records in the packet. 

***

### `answers(index)`
Returns the answer resource record corresponding to the index argument. Indices are numbered from 0. Returns `null` if index is greater than number of answer records in the packet. 

***

### `authorities(index)`
Returns the authority resource record corresponding to the index argument. Indices are numbered from 0. Returns `null` if index is greater than number of authority records in the packet.

***

### `additionals(index)`
Returns the additional resource record corresponding to the index argument. Indices are numbered from 0. Returns `null` if index is greater than number of additional records in the packet. 

***

### Example: Parsing a DNS packet
DNS packets are typically received as UDP packets. The `Socket` object provides each DNS packet in an `ArrayBuffer`. The follow example creates a DNS parser instance for an `ArrayBuffer`:

```js
let packet = new Parser(dnsPacket);
```

The `Parser` constructor does not validate the packet. If the packet is invalid, errors will be reported when extracting records from it.

***

### Example: Reading header fields

The parser instance has properties for the `id` and `flags` fields in the DNS packet:

```js
let id = packet.id;
let flags = packet.flags;
```
	
***

### Example: Determining the number of records
The parser instance has properties that provide the number of resource records in each section.

```js
let total = packet.questions + packet.answers + packet.authorities + packet.additionals;
```

***

### Example: Extracting a resource record
A JavaScript object containing a single resource record is retrieved by calling the function corresponding to the resource record's section. The following example retrieves the second question resource record (indices start at 0):

```js
let rr = packet.question(1);
```

There are also `answers`, `authorities`, and `additionals` functions.

***

<a id="dns-serializer"></a>
## class DNS Serializer

- **Source code:** [dnsserializer](../../modules/network/dns)

The DNS `Serializer` class implements a DNS record serializer.

```js
import Serializer from "dns/serializer";
```

The DNS `Serializer` class is able to serialize A, NSEC, PTR, SRV, and TXT resource record types. Clients may perform their own serialization of other resource record types and provide the result to the DNS `Serializer` class to include in the generated DNS packet.

> **Note**: The DNS `Serializer` is a low level class used to build higher level services, such as mDNS.

### `constructor(dictionary)`
The DNS `Serializer` constructor accepts a dictionary with properties to configure the DNS packet to be created. The dictionary may contain the following properties:

| Property | Default Value | Description |
| :---: | :---: | :--- |
| `opcode` | `DNS.OPCODE.QUERY` | The numeric value of the `opcode` header field 
| `query` | `true` | A boolean that indicates whether this packet contains a query or response
| `authoritative` | `false` | A boolean indicating the value of the `authoritative` bit in the header
| `id` | 0 | A numeric value for the ID field

***

### `add(section, name, type, clss, ttl, ...)`
The `add` function adds a resource record to be serialized into the DNS packet. The first five arguments to `add` are the same for all resource records.


| Argument | Description |
| :---: | :--- |
| `section` | The section to add this resource record to, e.g. `DNS.SECTION.ANSWER`.
| `name` | A `String` containing the DNS QNAME
| `type` | A `Number` representing the resource record type, e.g. `DNS.RR.A`
| `clss` | A `Number` containing the resource record class field value, typically `DNS.CLASS.IN`
| `ttl` | A `Number` containing the time-to-live value in seconds for this resource record

The optional `data` argument is used to build the resource data portion of the resource record. If not present, the resource data is empty. If it is an `ArrayBuffer`, its contents are used for the resource data. The `data` argument is interpreted these resource record types:

| Type | Description |
| :---: | :--- |
| `A` | A string containing the IP address.
| `NSEC` | A dictionary with two keys. The first is `next` containing a string with the next hostname value. The second is a Uint8Array containing the bit-mask.
| `PTR` | A string with the PTR value.
| `SRV` | A dictionary with four keys. The `priority`, `weight`, and `port` fields are numbers with the value of the corresponding field. The `target` property is a string containing the name of the target. 
| `TXT` | A dictionary of key / value pairs for the TXT record. The property name is the key. Only string values are supported at this time.

***

### `build()`
The `build` function generates a DNS packet based on the previous calls to the serializer instance. The packet is returned as an `ArrayBuffer`.

> **Note**: The current implementation does not compress QNAMES, resulting in a larger DNS packet than necessary.

***

### Example: Building a DNS query

The following example uses the DNS Serializer to create a DNS packet querying for an A record for the "example.com" domain:

```js
let serializer = new Serializer({query: true, opcode: DNS.OPCODE.QUERY});
serializer.add(DNS.SECTION.QUESTION, "example.com", DNS.RR.A, DNS.CLASS.IN);
let buffer = serializer.build();
```

The `build` function returns a DNS packet suitable for sending using the `write` function of the `Socket` class.

***

<a id="dns-server"></a>
## class DNS Server

- **Source code:** [dnsserver](../../modules/network/dns)
- **Relevant Examples:** [dnsserver](../../examples/network/dns/dnsserver)

The `DNSServer` class implements a simple DNS server.

```js
import DNSServer from "dns/server";
```

The server is indicated for use in devices in Wi-Fi access point mode that wish to act as a captive portal. The DNS server is used to direct look-ups for certain domains to an IP address, typically the device running the DNS server.

### `constructor(callback)`

The `DNSServer` constructor takes a single argument, a function to call when a look-up request is received. The callback receives two arguments. The first, `message`, is set to 1 when a look-up is performed. The second argument, `value`, is set to the name to be resolved when a look-up request is made.

```js
let server = new DNSServer((message, value) => {
	...
});
```

***

### `close()`

When the DNS server is no longer needed, call `close` to terminate it and free its resources.

```js
server.close();
```

***

### Example: Simple DNS server

The following example redirects all DNS look-ups to the IP address of the device running the server.

```js
new DNSServer((message, value) => {
	if (1 === message)
		return Net.get("IP", "ap");
})
```

> **Note:**: This example expects to be run on a Wi-Fi connection in access point mode. It passes "ap" for the interface argument to `Net.get` to retrieve the IP address for access point. 

### Example: DNS server for a single host name

The following example redirects all DNS look-ups for "example.com" to the IP address of the device running the server. All other look-ups are ignored.

```js
new DNSServer((message, value) => {
	if ((1 == message) && ("example.com" == value))
		return Net.get("IP", 'ap");
})
```

***

<a id="mdns"></a>
## class MDNS

- **Source code:** [mdns](../../modules/network/mdns)
- **Relevant Examples:** [discoverhttp](../../examples/network/mdns/discoverhttp), [httpserver](../../examples/network/mdns/httpserver), [ntpclient](../../examples/network/mdns/ntpclient), [ntpservice](../../examples/network/mdns/ntpservice), 

The `MDNS` class implements services for working with [Multicast DNS](https://tools.ietf.org/html/rfc6762) discovery and services. It includes claiming `.local` names, advertising mDNS service availability, and scanning for available mDNS services.

```js
import MDNS from "mdns";
```

### `constructor(dictionary [, callback])`

The `MDNS` constructor takes a dictionary to configure the mDNS instance and an optional `callback` to receive information about the instance status.

If the dictionary contains a `hostName` property, the MDNS instance will attempt to claim the name in the `.local` domain on the active network connection. The `hostName` is not required to monitor for available mDNS services.

The callback receives messages and, for some messages, a data value. The `message` and `value` provide information on the claiming process. Positive `message` values indicate normal operation and negative `message` values indicate an error.

| `message` | Description |
| :---: | :--- |
| 1 | **probing:** If `value` is an empty string, claiming is underway; when probing is successful, `value` contains the claimed name. |
| 2 | **conflict:** The attempt to claim the requested name discovered another device already using the name. The result of the callback function determines what happens next.<BR>- If the result is undefined, a new name is created automatically and the claiming process continues.<BR>- If a string is returned, the claiming process continues with the string used as the candidate hostname. Returning true causes the claiming process to end without having claimed a name.
| Any negative number | **error:** Claiming process terminated.

The following example shows how to claim the name "mydevice" on the local network.

```js
const mdns = new MDNS({hostName: "mydevice"});
```

The claiming process takes some time, usually under one second. Claiming the name may not succeed because the name may already be in use. An optional callback function provides status on the claim:
	
```js
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
```

***

### `monitor(serviceType, callback)`

The `monitor` function continuously scans the network for mDNS services of the type indicated by the `serviceType` parameter.

The callback function is invoked for each unique service instance found and whenever a service announces changes to its TXT resource record. The first argument to the callback is the service type, for example "\_http.\_tcp". The second is a dictionary that contains `name`, `protocol`, `port`, and `txt` properties describing the service.

The following example continuously monitors for `_http._tcp` services available on the local network:

```js
mdns.monitor("_http._tcp", (service, instance) => {
	trace(`Found ${service}: ${instance.name}\n`);
});
```

***

### `add(service)`

The `add` function registers an mDNS service description to be advertised. The service record contains the following properties:

| Property | Description |
| :---: | :--- |
| `name` | The service's name, e.g. "http"
| `protocol` | The service's protocol, e.g. "tcp" or "udp"
| `port` | The service's port
| `txt` | An optional JavaScript object with name value pairs to populate the TXT resource record of the service

`add` may only be called after the hostname claiming process has completed successfully.

The following example announces the availability of an `_http._tcp` service on port 80 of the current host.

```js
mdns.add({
	name: "http",
	protocol: "tcp",
	port: 80,
	txt: {
		url: `/index.html`,
	}
});
```

***

### `update(service)`

The `update` function tells the MDNS implementation that the contents of the TXT record have changed. This causes the new TXT record to be announced to the local network. The `service` object passed must be the same object provided to `add`.

```js
let service = mdns.services[0];
service.txt["value"] = 123;
mdns.update(service);
```
 
***

### `remove(service)` or `remove(serviceType)`

The `remove` function is used both to unregister the service and to cancel monitoring for a service type.

To unregister a service, pass the service description. This announces to the network that it is no longer available. The `service` object must be the same object provided to `add`.

```js
mdns.remove(mdns.services[0]);
```

To cancel monitoring for a service type, pass the name of the service type.

```js
mdns.remove("_http._tcp");
```

***

<a id="telnet"></a>
## class Telnet

- **Source code:** [telnet](../../modules/network/telnet)
- **Relevant Examples:** [telnet](../../examples/network/telnet)

The `Telnet` class implements a simple telnet server. The commands supported by the telnet server are determined by the `CLI` classes registered with the Console `module`.

### `constructor(dictionary)`

To start a telnet server, invoke the `Telnet` constructor:

The `Telnet` constructor takes a single argument, a dictionary. The dictionary has a single property, `port`, which indicates the port to listen on for new connections. If the `port` is not included in the dictionary, it defaults to 23.

```js
let telnet = new Telnet({port: 2300});
```

***

### `close()`

When the Telnet server is no longer needed, call `close` to terminate it and free its resources.

```js
telnet.close();
```

***

<a id="ping"></a>
## class Ping

- **Source code:** [ping](../../modules/network/ping)
- **Relevant Examples:** [ping](../../examples/network/ping)

The `Ping` class implements the ping networking utility.

```js
import Ping from "ping";
```

### `constructor(dictionary, callback)`

The `Ping` constructor takes two arguments, a dictionary and a callback function.

The dictionary must contain the following properties:

| Property | Description |
| :---: | :--- |
| `host` | The host to ping
| `id` | The identifier of the ping process; this should be unique for each `Ping` instance

The dictionary may optionally contain an `interval` parameter, which sets the interval between pings, in milliseconds. If none is specified, the default is 5000, or 5 seconds.

The user receives status information through the callback function. The callback receives messages and, for some messages, a data value and additional information in the `etc` parameter.  Positive `message` values indicate normal operation and negative `message` values indicate an error.

| `message` | Description |
| :---: | :--- |
| -1 | **error:** An error occurred and the host is no longer being pinged.
| 1 | **success:** The host responded to the echo request with an echo reply.
| 2 | **timeout:** The host did not respond.


The following example pings the server at `example.com` every 1000ms, tracing the results to the console.

```js
let ping = new Ping({host: "example.com", id: 1, interval: 1000}, (message, value, etc) => {
	if (1 == message)
		trace(`${value} bytes from ${etc.address}: icmp_seq=${etc.icmp_seq}\n`);
}
```

***

### `close()`

To stop pinging the host, call the `close` function.

```js
ping.close();
```

***

<a id="mqtt"></a>
## class MQTT

- **Source code:** [mqtt](../../modules/network/mqtt)
- **Relevant examples:** [mqttbasic](../../examples/network/mqtt/mqttbasic), [mqttsecure](../../examples/network/mqtt/mqttsecure)

The MQTT `Client` class implements a client that connects to an MQTT broker (server).

```js
import Client from "mqtt";
```

### `constructor(dictionary)`

A new MQTT `Client` is configured using a dictionary of properties. The dictionary must contain `host` and `id` properties; other properties are optional.

| Parameter | Description |
| :---: | :--- |
| `host` | The host name of the remote MQTT server |
| `port` | The remote port number. Required for connections using TLS., defaults to 1883 for direct MQTT connections and 80 for MQTT over WebSocket connections.  |
| `id` | A unique ID for this device |
| `user` | The username |
| `password` | The password as an `ArrayBuffer` |
| `will` | An object with `topic` and `message` properties to be set as the connection's Will. `message` may be a string or `ArrayBuffer`.  |
| `path` | The endpoint to connect to. If present, the MQTT client communicates established a WebSocket connecting using the `mqtt` sub-protocol. |
| `timeout` | The keep-alive timeout interval, in milliseconds. If no timeout is provided, the MQTT keep-alive feature is not used. |
| `Socket` | The socket constructor to use to create the MQTT connection. Use `SecureSocket` to establish a secure connection using TLS. |
| `secure` | Dictionary of options for a TLS connection when using `SecureSocket` |

```
let mqtt = new Client({
	host: "test.mosquitto.org",
	id: "iot_" + Net.get("MAC"),
	user: "user name",
	password: ArrayBuffer.fromString("secret")
});
```

***

### `onReady()`

The `onReady` callback is invoked when a connection is successfully established to the server. No messages may be published or subscriptions created before `onReady` is called.

```
mqtt.onReady = function () {
	trace("connection established\n");
}
```

***

### `subscribe(topic)`

To subscribe to a topic, use the `subscribe` method. Your client can subscribe to multiple clients by calling `subscribe` more than once.

```
mqtt.subscribe("test/string");
mqtt.subscribe("test/binary");
mqtt.subscribe("test/json");
```

### `unsubscribe(topic)`

Use the `unsubscribe` method to unsubscribe to a topic.


```
mqtt.unsubscribe("test/string");
```

### `onMessage(topic, data)` 

The `onMessage` callback is invoked when a message is received for any topic that your client has subscribed to. The `topic` argument is the name of the topic and the `data` argument is the complete message.

```
mqtt.onMessage = function(topic, data) {
	trace(`received message on topic "${topic}"\n`);
}
```

The `data` argument is an `ArrayBuffer`. For messages containing only UTF-8 text, you can convert it to a string using `String.fromArrayBuffer`.

```
mqtt.onMessage = function(topic, data) {
	trace(`received message on topic "${topic}"\n`);
	trace(`data: ${String.fromArrayBuffer(data)}\n`);
}
```

***

### `publish(topic, message)`

To send a message to a topic, use the `publish` method. The `message` argument may be either a string or an `ArrayBuffer`.

```
mqtt.publish("test/string", "hello");
mqtt.publish("test/binary", Uint8Array.of(1, 2, 3).buffer);
```

To publish JSON, first convert it to a string.

```
mqtt.publish("test/json", JSON.stringify({
	message: "hello",
	version: 1
}));
```

***

### `onClose()`

The `onClose` callback is invoked when the connection is lost, because of a network error or because the MQTT broker closed the connection.

```
mqtt.onClose = function() {
	trace("connection lost\n");
}
```

***
