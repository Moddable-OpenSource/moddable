# Moddable SDK - TCP IO Examples

Copyright 2022 Moddable Tech, Inc.<BR>
Revised: March 29, 2022

The examples in this directory use the [TCP socket](https://419.ecma-international.org/#-10-io-classes-tcp-socket) defined by Ecma-419.

- [simplehttpget](./simplehttpget) – A bare bones example of making an HTTP request using TCP socket directly. This is intended as an example of using the TCP socket IO, not as a good solution for HTTP requests.
- [httpclient](./httpclient) – An HTTP client API built on TCP socket. The `HTTPRequest` class implements most of the [draft spec](https://github.com/EcmaTC53/spec/blob/master/docs/proposals/Network%20Classes.md#http-request-class) for version 2 of Ecma-419.
- [fetch](./fetch) – An implementation of the standard HTML5 `fetch` API built on the proposed Ecma-419 `HTTPRequest`. Implements most of `fetch` with the exception of streams (under investigation).
- [websocketclient](./websocketclient) – A WebSocket client API built on TCP socket. The `WebSocketClient` class implements most of the [draft spec](https://github.com/EcmaTC53/spec/blob/master/docs/proposals/Network%20Classes.md#websocket-client-class) for version 2 of Ecma-419.
- [websocket](./websocket) – An implementation of the standard HTML5 `WebSocket` API built on the proposed Ecma-419 `WebSocketClient `.

The `HTTPRequest ` and `WebSocketClient` classes are designed to use runtime resources efficiently while supporting all the fundamental capabilities of the underlying protocol. They give the most control and the lightest footprint, but as lower-level APIs they are less convenient to use.

The `fetch` and `WebSocket` classes provide embedded developers access to familiar, standard APIs from the web platform. They are generally more convenient to use and use more resources (RAM, code size, CPU). 

The choice of which API to use for a project depends on the requirements of that project. For many situations, the standard `fetch` and `WebSocket` APIs work well. 
