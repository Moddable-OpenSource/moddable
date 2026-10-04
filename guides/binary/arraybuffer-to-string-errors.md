---
name: Handle Errors Converting ArrayBuffer to String
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

If the binary data being converted to a String is not valid UTF-8 data, `String.fromArrayBuffer()` throws an exception you catch with a `try`-`catch` block. 

```js
// invalid UTF-8 sequence
const bytes = Uint8Array.of(0xE2, 0x28, 0xA1);
try {
	let s = String.fromArrayBuffer(bytes.buffer);
}
catch {
	// invalid UTF-8 data
}
```

---

If you don't want an exception thrown on invalid UTF-8 data, use `TextDecoder`. It replaces invalid data with the [Unicode Replacement Character](https://en.wikipedia.org/wiki/Specials_%28Unicode_block%29#Replacement_character) (`U+FFFD`) .

```js
// invalid UTF-8 sequence
const bytes = Uint8Array.of(0xE2, 0x28, 0xA1);
const decoder = new TextDecoder;
const string = decoder.decode(bytes.buffer);
// => "\uFFFD"
```
---

If you want the `TextDecoder` to reject invalid input, set the `fatal` option when calling the constructor.

```js
// invalid UTF-8 sequence
const bytes = Uint8Array.of(0xE2, 0x28, 0xA1);
const decoder = new TextDecoder("utf-8", {fatal: true});
try {
	let s = decoder.decode(bytes.buffer);
}
catch {
	// invalid UTF-8 data
}
```
