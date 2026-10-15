---
name: HTTP
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-14
---

The high-level web standard [`fetch()`](https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API) API and the low-level ECMA-419 embedded standard [HTTP Client](https://419.ecma-international.org/#http-client-class-pattern) APIs are both available to Embedded JavaScript developers using the Moddable SDK. In fact, `fetch()` is implemented using the HTTP Client.

> **Note**: There are two versions of `fetch()`. The default is the smaller of the two, without support for streams.

## Using fetch()

- [Make Request](./fetch-making-http-request.md)
- [Make Secure Request](./fetch-using-https.md)
- [Send Request Headers](./fetch-sending-request-headers.md)
- [Receive Response Headers](./fetch-receiving-response-headers.md)
- [Send Request Body](./fetch-sending-request-body.md)

## Using ECMA-419 HTTP Client

- [Make Request](./httpclient-making-http-request.md)
- [Make Secure Request](./httpclient-using-https.md)
- [Send Request Headers](./httpclient-sending-request-headers.md)
- [Receive Response Headers](./httpclient-receiving-response-headers.md)
- [Send Request Body](./httpclient-sending-request-body.md)

## How to Choose

If `fetch()` works for your project, you should use it. You can always start with `fetch()` and switch to HTTP Client later. With `fetch()` you'll usually write less code. Because `fetch()` is a well-known web standard, many developers are already familiar with it.

Situations where `fetch()` might not be the best choice include when resources like RAM and ROM are very constrained and when you need precise control over the buffering of request and response bodies.

Using HTTP Client requires more code and a deeper understanding of the HTTP protocol. But, it offers real advantages:

- Lower runtime overhead
- Stream request body
- Stream response body
- Ignore unneeded response headers
- Smaller firmware size

## Building with mcconfig

Include the `fetch()` manifest in your project's `manifest.json`:

	$(MODDABLE)/modules/web/fetch

Then, import the module in your JavaScript source code:

```js
import { fetch } from "fetch";
```

To use the stream-enabled version of `fetch()`, include its manifest in your project's `manifest.json`:

	$(MODDABLE)/examples/web/streams/fetch/manifest_fetch.json

Then, import the module in your JavaScript source code:

```js
import { fetch } from "web/fetch/streams";
```

To use the HTTP Client instead, include its manifest. Including the manifest initializes the host provider `device.network.http.client`:

	$(MODDABLE)/modules/io/http/client

For secure requests over HTTPS, include the HTTPS Client manifest instead. It initializes the host provider `device.network.https.client`:

	$(MODDABLE)/modules/io/https/client

## Building with mcpack

If your project uses the `fetch` global, `mcpack` automatically includes the manifest and initializes the `fetch` global. If your project uses `fetch` in an import statement, its manifest is automatically included.

To use the stream-enabled version of `fetch()`, include its manifest as described above under `Building with mcconfig`

If your project accesses the HTTP Client through `device.network.http.client` or `device.network.https.client`, `mcpack` includes the necessary manifests automatically.

## Learn More

- Documentation
	- [fetch()](https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API) on MDN
- Standards
	- [fetch()](https://fetch.spec.whatwg.org/) in the WHATWG Fetch Standard
	- [HTTP Client](https://419.ecma-international.org/#http-client-class-pattern) in ECMA-419
	- [HTTP/1.1 specification RFC](https://www.rfc-editor.org/info/rfc9110/)
- Examples
	- [fetch() example](../../examples/io/tcp/fetch/main.js) in Moddable SDK
	- [HTTP Client example](../../examples/io/tcp/httpclient/main.js) in Moddable SDK
- Implementations
	- [fetch()](../../modules/web/fetch/fetch.js) in Moddable SDK
	- [HTTP Client](../../modules/io/http/client/httpclient.js) in Moddable SDK
- TypeScript Declarations
	- [fetch()](../../typings/web/fetch.d.ts) in Moddable SDK
	- [HTTP Client](../../typings/embedded_network/http/client.d.ts) in Moddable SDK
