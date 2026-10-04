---
name: Make Request using fetch()
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

Making an HTTP request using `fetch()` is straightforward. The result is traced as text to the console.

```js
import { fetch } from "fetch";

const response = await fetch("http://httpbin.org/encoding/utf8");
trace(`${response.url} ${response.status} ${response.statusText}\n`);
const text = await response.text();
trace(text, "\n");
```

---

You can retrieve the response as binary data in an `ArrayBuffer`.

```js
import { fetch } from "fetch";

const response = await fetch("http://httpbin.org/image/png");
trace(`${response.url} ${response.status} ${response.statusText}\n`);
const buffer = await response.arrayBuffer();
trace((new Uint8Array(buffer)).toHex(), "\n");
```

---

You can retrieve the response as parsed JSON.

```js
import { fetch } from "fetch";

const response = await fetch("http://httpbin.org/json");
trace(`${response.url} ${response.status} ${response.statusText}\n`);
const json = await response.json();
trace(JSON.stringify(json), "\n");
```

---

The HTTP request method defaults to `"GET"`. Set the request method using the `method` option.

```js
import { fetch } from "fetch";

const response = await fetch("http://httpbin.org/json", {method: "DELETE"});
trace(`${response.url} ${response.status} ${response.statusText}\n`);
```

---

Use the stream-enabled version of `fetch()` to stream the response body.

This example pipes the response body stream through a `TextDecoderStream` to convert the binary data delivered by `fetch()` to strings.

```js
import { fetch } from "web/fetch/streams";
import TextDecoderStream from "web/textdecoderstream";

const url = "http://httpbin.org/encoding/utf8";
const response = await fetch(url);
const transformedBody = response.body.pipeThrough(
	new TextDecoderStream());

const reader = transformedBody.getReader();
while (true) {
	const {done, value} = await reader.read();
	if (value)
		trace(value);
	if (done) {
		trace("\n");
		break;
	}
}
```
