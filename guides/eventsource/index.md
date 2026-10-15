---
name: EventSource
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-14
---

The web standard [`EventSource`](https://developer.mozilla.org/en-US/docs/Web/API/EventSource) API receives [Server-Sent Events](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events) over an HTTP or HTTPS connection. Because it is the same API available in web browsers, it is familiar to many developers. In the Moddable SDK, `EventSource` is implemented using the HTTP Client.

The Moddable SDK implementation extends the standard API to support setting the HTTP request method, request headers, and request body.

## Using EventSource()

- [Connect](./connect.md)
- [Connect Securely](./connect-securely.md)
- [Close](./close.md)
- [Receive](./receive.md)
- [Connection Information](./connection-info.md)

## Building with mcconfig

Include the `EventSource` manifest in your project's `manifest.json`:

	$(MODDABLE)/modules/web/eventsource

Then, import the module in your JavaScript source code:

```js
import EventSource from "eventsource";
```

## Building with mcpack

If your project uses the `EventSource` global, `mcpack` automatically includes the manifest and initializes the `EventSource` global. If your project uses `eventsource` in an import statement, its manifest is automatically included.

## Learn More

- Documentation
	- [EventSource](https://developer.mozilla.org/en-US/docs/Web/API/EventSource) on MDN
- Standard
	- [Server-Sent Events](https://html.spec.whatwg.org/multipage/server-sent-events.html) in the WHATWG HTML Standard
- Example
	- [EventSource example](../../examples/io/tcp/eventsource/main.js) in Moddable SDK
- Implementation
	- [EventSource](../../modules/web/eventsource/eventsource.js) in Moddable SDK
- TypeScript Declaration
	- [EventSource](../../typings/web/eventsource.d.ts) in Moddable SDK
