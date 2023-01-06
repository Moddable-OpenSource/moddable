# Moddable SDK - TCP IO Examples

Copyright 2022 Moddable Tech, Inc.<BR>
Revised: December 21, 2022

The examples in this directory use the [TCP socket](https://419.ecma-international.org/#-10-io-classes-tcp-socket) defined by Ecma-419.

- HTTP
	- [simplehttpget](./simplehttpget) – A bare bones example of making an HTTP request using TCP socket directly. This is intended as an example of using the TCP socket IO, not as a good solution for HTTP requests.
	- [httpclient](./httpclient) – An HTTP client API built on TCP socket. The `HTTPRequest` class implements most of the [draft spec](https://github.com/EcmaTC53/spec/blob/master/docs/proposals/Network%20Classes.md#http-request-class) for the 2nd Edition of Ecma-419.
	- [fetch](./fetch) – An implementation of the standard HTML5 `fetch` [API](https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API) built on the proposed 2nd Edition Ecma-419 `HTTPRequest` API. Implements most of `fetch` with the exception of streams (under investigation). Updated 12/2022 to also make HTTPS requests.
- HTTPS
	- [httpsclient](./httpsclient) – Combines `TLS` TCP socket with `httpclient` to make several secure requests to google.com.
- WebSocket
	- [websocketclient](./websocketclient) – A WebSocket client API built on TCP socket. The `WebSocketClient` class implements most of the [draft spec](https://github.com/EcmaTC53/spec/blob/master/docs/proposals/Network%20Classes.md#websocket-client-class) for 2nd Edition of Ecma-419.
	- [websocket](./websocket) – An implementation of the standard HTML `WebSocket` [API](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket) built on the proposed 2nd Edition Ecma-419 `WebSocketClient` API.
- MQTT
	- [mqttclient](./mqttclient) – An MQTT client API built on TCP socket. The `MQTTClient` class implements most of the [draft spec](https://github.com/EcmaTC53/spec/blob/master/docs/proposals/Network%20Classes.md#network-mqtt-client) for the 2nd Edition of Ecma-419.
	- [MQTT.js](./mqtt) – An implementation of the widely used [MQTT.js package](https://www.npmjs.com/package/mqtt) for Node.js and browsers built on the proposed 2nd Edition Ecma-419 `MQTTClient` API.
- Server Sent Events
	- [EventSource](./eventsource) – An implementation of the standard HTML `EventSource` [API](https://developer.mozilla.org/en-US/docs/Web/API/EventSource) for receiving [Server Sent Events](https://html.spec.whatwg.org/multipage/server-sent-events.html#parsing-an-event-stream). Build on the HTTP client API.

The `HTTPRequest`, `WebSocketClient`, and `MQTTClient` classes are designed to use runtime resources efficiently while supporting all the fundamental capabilities of the underlying protocol. They give the most control and the lightest footprint, but as lower-level APIs they are less convenient to use.

The `fetch`, `WebSocket`, and `mqtt` classes provide embedded developers access to familiar, standard APIs from the web platform. They are generally more convenient to use and use more resources (RAM, code size, CPU). 

The choice of which API to use for a project depends on the requirements of that project. For many situations, the standard `fetch`, `WebSocket`, and `mqtt` APIs work well. 
