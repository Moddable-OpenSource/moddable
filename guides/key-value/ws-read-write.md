---
name: Read and Write Values using Web Storage
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

Reading and writing values with Web Storage requires a Web Storage instance, such as `localStorage`. See [Setting Up Web Storage](./index.md#setting-up-web-storage) to create one.

Once you have a `localStorage` instance, read and write values using `getItem()` and `setItem()`. Keys and values are always strings. 

Note that domain and key names are case-sensitive and may contain any Unicode character. Most host implementations limit the length of domain and key names. For portability, limit them to 31 bytes.

While the Web supports very long values, embedded implementations limit the length of the value strings and this limit varies by host. For portability, limit them to 1024 bytes.

```js
localStorage.setItem("hello", "world");
trace(`${localStorage.getItem("hello")}\n`);
```

---

Web Storage values are always strings. If you need to store binary data, use the Base64 support provided by `Uint8Array`.

```js
const bytes = Uint8Array.of(1, 2, 3, 4);
localStorage.setItem("binary", bytes.toBase64());
const value = localStorage.getItem("binary");
trace(`${Uint8Array.fromBase64(value)}\n`);
```

---

If you need to store structured data, JSON is a convenient option. Because the size of stored values is limited on embedded platforms, keep the JSON data relatively small.

```js
const object = {one: 1, two: "two", and: {three: 3}};
localStorage.setItem("json", JSON.stringify(object));
const value = localStorage.getItem("json");
const json = JSON.parse(value);
trace(`${JSON.stringify(json)}\n`);
```

---

If the requested key is not present in the domain, `getItem()` returns `null` rather than throwing an exception.

```js
if (null === localStorage.getItem("once"))
	localStorage.setItem("once", "true");
```
