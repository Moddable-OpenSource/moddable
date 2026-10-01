---
name: Send Message using WebSocket()
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

Use `send()` to transmit messages as a string.

```js
import WebSocket from "WebSocket";

const ws = new WebSocket("ws://example.com/ws");
ws.addEventListener("open", event => {
	ws.send("This is a message.");
	ws.send("This is another message. ❤️");
});
```

---

To transmit binary messages with `send()` pass an `ArrayBuffer`, `DataView`, or `TypedArray`.

```js
import WebSocket from "WebSocket";

const ws = new WebSocket("ws://example.com/ws");
ws.addEventListener("open", event => {
	ws.send(new ArrayBuffer(20));
	ws.send(Uint8Array.of(1, 2, 3));
	ws.send(new DataView(new ArrayBuffer(10)));
});
```

---

If a message cannot be transmitted immediately, `send()` queues it to send as soon as possible. Check [`bufferedAmount`](./websocket-connection-info.md) for the number of bytes queued to send.
