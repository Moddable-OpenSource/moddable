---
name: WebSocket Server
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-14
---

The WebSocket Server provides comprehensive support for the WebSocket protocol by building on the HTTP Server and WebSocket client capabilities. A WebSocket connection begins with an HTTP connection before transitioning to the WebSocket protocol. This phase is implemented in the HTTP server using the WebSocket Handshake route. Once the handshake is complete, the connection is detached from the HTTP server and attached to a WebSocket client. While this may seem counter-intuitive, once the connection has been established, WebSocket is effectively a peer-to-peer protocol so the same API can be used by both endpoints.

This guide covers the process of creating a WebSocket server endpoint, accepting connections, and attaching them to a WebSocket client. For deeper information see the [HTTP Server Guide](../httpserver/index.md) and [WebSocket Guide](../websocket/index.md).

## Using ECMA-419 WebSocket Handshake Route

- [Create WebSocket Server Endpoint](./create.md)
- [Accept WebSocket Request](./accept.md)

## Building with mcconfig

The WebSocket Handshake Route is included by the HTTP Server manifest. Add it to your project's manifest:

	$(MODDABLE)/modules/io/http/server

To attach accepted connections to a WebSocket client, add the WebSocket manifest:

	$(MODDABLE)/modules/web/websocket

This provides both the Web platform's `WebSocket()` and ECMA-419 WebSocket Client.

## Building with mcpack

If your project accesses the HTTP Server through `device.network.http.server`, `mcpack` includes the WebSocket Handshake route automatically.

## Learn More

- Standards
	- [WebSocket Handshake route](https://419.ecma-international.org/#http-server-connection-routes-websocket-handshake-route) in ECMA-419
- Examples
	- [HTTP Server example (includes WebSocket endpoint)](../../examples/io/listener/httpserver/main.js) in Moddable SDK
	- [Captive Portal (includes WebSocket endpoint)](../../examples/io/wifiaccesspoint/captiveportal/captiveportal.js) in Moddable SDK
- Implementation
	- [WebSocket Handshake route](../../modules/io/http/server/options/websocket.js) in Moddable SDK
- TypeScript Declaration
	- [HTTP Server](../../typings/embedded_network/http/server.d.ts) in Moddable SDK
	- [WebSocket Client](../../typings/embedded_network/websocket/client.d.ts) in Moddable SDK
