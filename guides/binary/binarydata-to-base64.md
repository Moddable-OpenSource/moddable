---
name: Convert Binary Data to Base64
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

Use the JavaScript standard [`toBase64()` method](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Uint8Array/toBase64) of `UInt8Array` to convert binary data to a Base64 encoded string.

```js
const bytes = Uint8Array.of(0, 1, 2, 3, 4);
const base64 = bytes.toBase64();
// => "AAECAwQ="
```

---

By default `toBase64()` uses the standard Base64 alphabet. You may also use the "URL safe" alphabet for encoding.

```js
const bytes = Uint8Array.of(0xFB, 0xEF, 0xFF);
const base64Standard = bytes.toBase64();
// => "++//"

const base64URLSafe = bytes.toBase64({alphabet: "base64url"});
// => "--__"
```

---

Tp convert an `ArrayBuffer` to Base64, wrap it in a `Uint8Array`.

```js
const buffer = new ArrayBuffer(2);
const base64 = (new Uint8Array(buffer)).toBase64();
// => "AAA="
```
