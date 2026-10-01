---
name: Close Connection using WebSocket()
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

Use the `close()` method to disconnect from the WebSocket server.

```js
import WebSocket from "WebSocket";

const ws = new WebSocket("ws://example.com/ws");
ws.close();
```

---

Indicate the reason for closing by passing a [status code](https://www.rfc-editor.org/info/rfc6455/#section-7.4.1) and string explaining the reason. The string must be no longer than 123 bytes when encoded as UTF-8.

```js
import WebSocket from "WebSocket";

const ws = new WebSocket("ws://example.com/ws");
ws.close(3000, "going away");
```

---

When the server closes the connection, the client receives a `"close"` event. The event includes the [status code](https://www.rfc-editor.org/info/rfc6455/#section-7.4.1) and reason string.

```js
import WebSocket from "WebSocket";

const ws = new WebSocket("ws://example.com/ws");
ws.addEventListener("close", event => {
	trace(`remote close\n`);
	trace(`  status code ${event.code}\n`);
	trace(`  reason ${event.reason}\n`);
});
```
