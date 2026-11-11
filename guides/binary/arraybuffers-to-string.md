---
name: Convert ArrayBuffers to String
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-11-11
---

The `TextDecoder` handles multi-byte UTF-8 sequences split across buffers using the `stream` option of its `decode()` method.

The `stream` option is useful for text received in fragments, such as serial data from a UART or the body of an HTTP response.

```js
// UTF-8 values for "forêt"
const bytes = Uint8Array.of(102, 111, 114, 195, 170, 116);
const decoder = new TextDecoder;

// first part, splitting the "ê"
let string = decoder.decode(bytes.buffer.slice(0, 4), {stream: true});
// => "for"

// second part, splitting the "ê"
string += decoder.decode(bytes.buffer.slice(4));
// => "forêt"
```

---

The `decode()` method accepts any Byte Buffer. The above example can be rewritten using `subarray()` instead of `slice()` to eliminate a copy of the data.

```js
// UTF-8 values for "forêt"
const bytes = Uint8Array.of(102, 111, 114, 195, 170, 116);
const decoder = new TextDecoder;

 // first part, splitting the "ê"
let string = decoder.decode(bytes.subarray(0, 4), {stream: true});
// => "for"

 // second part, splitting the "ê"
string += decoder.decode(bytes.subarray(4));
// => "forêt"
```

---

To use `TextDecoder` you need to import its module in your module...

```js
import TextDecoder from "text/decoder"; 
```

---

...and to include its manifest in your manifest.

```json
{
	"include": [
		"$(MODDABLE)/modules/data/text/decoder"
	]
}
```

