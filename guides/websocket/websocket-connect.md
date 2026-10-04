---
name: Connect to Server using WebSocket()
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

To connect to a WebSocket server, pass the "ws:" URL to the `WebSocket` constructor.

```js
import WebSocket from "WebSocket";

const ws = new WebSocket("ws://example.com/ws");
```

---

To request a subprotocol be used, pass a list of acceptable subprotocols.

```js
import WebSocket from "WebSocket";

const ws = new WebSocket(
		"ws://example.com/ws",
		["mqtt", "soap"],
);
```

---

The Moddable SDK extends `WebSocket` to accept an options object as the sole constructor argument. The options include HTTP headers and keep-alive that are unavailable from the standard `WebSocket` constructor. Enabling keep-alive sends periodic `ping` control messages.

```js
import WebSocket from "WebSocket";

const ws = new WebSocket({
	url: "ws://example.com/ws",
	protocol: ["mqtt", "soap"],
	keepalive: 10_000,
	headers: new Map([
		["custom-header", "secret"]
	])
});
```

---

The WebSocket constructor begins the process of establishing a connection. The connection handshake occurs asynchronously. Listen for the `"open"` event to know when the connection has been established.

```js
import WebSocket from "WebSocket";

const ws = new WebSocket("ws://example.com/ws");
ws.addEventListener("open", event => {
	trace(`connected\n`);
});
```
