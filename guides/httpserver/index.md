---
name: HTTP Server
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-15
---

The HTTP Server is the core of an HTTP 1.1 server with a routing mechanism to bind paths to request handlers. There are built-in handlers to reply to requests with static text and binary data. You can add routes for your project's needs such as downloading and uploading files, responding with dynamically generated content, and installing firmware updates.

The HTTP Server supports request and response bodies larger than memory by processing them in fragments.

To efficiently support the many uses of an HTTP server while running in a resource constrained environment, the HTTP Server is a relatively low-level API.

When working with HTTP headers in the HTTP Server, header names use only lowercase letters. The `Headers` class automatically converts header names to lowercase.

To host WebSocket endpoints in the HTTP Server, wee the [WebSocket Server Guide](../websocket-server/index.md).

## Using ECMA-419 HTTP Server

- [Create Server](./create.md)
- [Route Request](./route.md)
- [Respond to Request](./respond.md)
- [Respond to Request with File](./respond-file.md)
- [Receive Request Body](./receive-body.md)
- [Receive Request Body to File](./receive-file.md)

## Building with mcconfig

Include the HTTP Server manifest in your project's `manifest.json`:

	$(MODDABLE)/modules/io/http/server

You can then access the server through `device.network.http.server`.

Each built-in route has its own manifest.

	$(MODDABLE)/modules/io/http/server/route/static
	$(MODDABLE)/modules/io/http/server/route/serversentevents
	$(MODDABLE)/modules/io/http/server/route/ws/handshake

## Building with mcpack

If your project accesses the HTTP Server through `device.network.http.server`, `mcpack` includes the necessary manifests automatically. If your project imports a built-in route, such as `embedded:network/http/server/route/static`, `mcpack` also includes that route's manifest.

## Learn More

- Standards
	- [HTTP Server](https://419.ecma-international.org/#http-server-class-pattern) in ECMA-419
	- [HTTP Server Connection routes](https://419.ecma-international.org/#http-server-connection-routes) in ECMA-419
	- [HTTP/1.1 specification RFC](https://www.rfc-editor.org/info/rfc9112/)
- Example
	- [HTTP Server example](../../examples/io/listener/httpserver/main.js) in Moddable SDK
- Implementation
	- [HTTP Server](../../modules/io/http/server/httpserver.js) in Moddable SDK
	- [static route](../../modules/io/http/server/route/static/static.js) in Moddable SDK
- TypeScript Declaration
	- [HTTP Server](../../typings/embedded_network/http/server.d.ts) in Moddable SDK
