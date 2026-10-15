---
name: WebSocket
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-14
---

The high-level web standard [`WebSocket()`](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket) API and the low-level ECMA-419 embedded standard [WebSocket Client](https://419.ecma-international.org/#websocket-client) APIs are both available to Embedded JavaScript developers using the Moddable SDK. In addition, the Web platform's experimental `WebSocketStream()` is available. Both `WebSocket()` and `WebSocketStream()` are implemented using WebSocket Client.

## Using WebSocket()

- [Connect to Server](./websocket-connect.md)
- [Connect Securely to Server](./websocket-use-wss.md)
- [Close Connection](./websocket-close.md)
- [Send Message](./websocket-send.md)
- [Receive Message](./websocket-receive.md)
- [Connection Information](./websocket-connection-info.md)

## Using ECMA-419 WebSocket Client

- [Connect to Server](./wsclient-connect.md)
- [Connect Securely to Server](./wsclient-use-wss.md)
- [Close Connection](./wsclient-close.md)
- [Send Message](./wsclient-send.md)
- [Receive Message](./wsclient-receive.md)
- [Control Messages](./wsclient-control.md)

## Using WebSocketStream()

- [Connect to Server](./wsstream-connect.md)
- [Connect Securely to Server](./wsstream-use-wss.md)
- [Close Connection](./wsstream-close.md)
- [Send Message](./wsstream-send.md)
- [Receive Message](./wsstream-receive.md)

## How to Choose

If `WebSocket()` works for your project, you should use it. You can always start with `WebSocket()` and switch to WebSocket Client later. With `WebSocket()` you'll usually write less code. Because `WebSocket()` is a well-known web standard, many developers are already familiar with it.

`WebSocketStream()` is built on streams, part of the modern Web platform. This simplifies composing data flows. `WebSocketStream()` also addresses a fundamental problem with `WebSocket()`, the lack of support for backpressure, which can lead to memory and buffering problems. For low throughput connections, the lack of backpressure support is not usually an issue. 

Situations where `WebSocket()` and `WebSocketStream()` might not be the best choice include when resources like RAM and ROM are very constrained, when you have large messages, and when you need precise control over the buffering of messages. 

Using WebSocket Client requires more code and a deeper understanding of the WebSocket protocol. But, it offers real advantages:

- Lower runtime overhead
- Send and receive messages of unlimited size
- Send and receive control messages
- Smaller firmware size

## Building with mcconfig

Include the `WebSocket()` manifest in your project's `manifest.json`:

	$(MODDABLE)/modules/web/websocket

Then, import the module in your JavaScript source code:

```js
import WebSocket from "WebSocket";
```

To use the WebSocket Client instead, include its manifest. Including the manifest initializes the host provider `device.network.ws`:

	$(MODDABLE)/modules/io/websocket/client

For secure connections over WSS, include the secure WebSocket Client manifest instead. It initializes the host provider `device.network.wss`:

	$(MODDABLE)/modules/io/websockets/client

To use `WebSocketStream()`, include its manifest in your project's `manifest.json`.

	$(MODDABLE)/modules/web/streams/websocket/manifest.json

Then, import the module in your JavaScript source code:

```js
import WebSocketStream from "web/websocketstream";
```

## Building with mcpack

If your project uses the `WebSocket` global, `mcpack` automatically includes the manifest and initializes the `WebSocket` global. If your project uses `WebSocket` in an import statement, its manifest is automatically included.

If your project accesses the WebSocket Client through `device.network.ws` or `device.network.wss`, `mcpack` includes the necessary manifests automatically.

If your project uses the `WebSocketStream` global, `mcpack` automatically includes the manifest and initializes the `WebSocketStream` global. If your project uses `web/websocketstream` in an import statement, its manifest is automatically included.

## Learn More

- Documentation
	- [WebSocket()](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket) on MDN
	- [WebSocketStream()](https://developer.mozilla.org/en-US/docs/Web/API/WebSocketStream) on MDN
	- [WebSocketStream Intro](https://developer.chrome.com/docs/capabilities/web-apis/websocketstream) on "Chrome for Developers"
	
- Standards
	- [WebSocket protocol RFC](https://www.rfc-editor.org/info/rfc6455/)
	- [WebSocket](https://websockets.spec.whatwg.org/) in the WHATWG WebSockets Standard
	- [WebSocket Client](https://419.ecma-international.org/#websocket-client) in ECMA-419
- Examples
	- [WebSocket() example](../../examples/io/tcp/websocket/main.js) in Moddable SDK
	- [WebSocket Client example](../../examples/io/tcp/websocketclient/main.js) in Moddable SDK
	- [WebSocketStream() example](../../examples/web/streams/websocket/main.js) in Moddable SDK
	- [Touch over WebSocketStream() example](../../examples/web/streams/touch-websocket/main.js) in Moddable SDK
- Implementations
	- [WebSocket()](../../modules/web/websocket/WebSocket.js) in Moddable SDK
	- [WebSocket Client](../../modules/io/websocket/client/websocketclient.js) in Moddable SDK
	- [WebSocketStream()](../../modules/web/streams/websocket/WebSocketStream.js) in Moddable SDK
- TypeScript Declarations
	- [WebSocket()](../../typings/web/websocket.d.ts) in Moddable SDK
	- [WebSocket Client](../../typings/embedded_network/websocket/client.d.ts) in Moddable SDK
	- [WebSocketStream()](../../typings/web/websocketstream.d.ts) in Moddable SDK
