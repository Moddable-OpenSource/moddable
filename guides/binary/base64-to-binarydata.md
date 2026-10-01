---
name: Convert Base64 to Binary Data
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

Use the JavaScript standard [`Uint8Array.fromBase64()` method](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Uint8Array/fromBase64) to convert a Base64 encoded string to a `Uint8Array`.

```js
const string = "AAECAwQ=";
const bytes = Uint8Array.fromBase64(string);
// => Uint8Array of 0, 1, 2, 3, 4
```

---

By default `Uint8Array.fromBase64()` uses the standard Base64 alphabet. You may also use the "URL safe" alphabet for decoding.

```js
const base64Standard = Uint8Array.fromBase64("++//");
// Uint8Array of 0xFB, 0xEF, 0xFF

const base64URLSafe = Uint8Array.fromBase64("--__", {alphabet: "base64url"});
// Uint8Array of 0xFB, 0xEF, 0xFF
```
