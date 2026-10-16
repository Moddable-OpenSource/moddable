---
name: Key-Value Storage
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-08-04
---

Key-value storage lets a project store small pieces of data, such as settings and state, that persist across restarts. The Moddable SDK provides two APIs: the high-level web standard [Web Storage](https://developer.mozilla.org/en-US/docs/Web/API/Web_Storage_API) API (`localStorage`), and the low-level ECMA-419 embedded standard [Key-Value store](https://419.ecma-international.org/#storage-keyvalue). Both organize data into named domains, each holding a set of key-value pairs. In fact, Web Storage is implemented using the Key-Value store.

Key-Value store replaces the Moddable SDK's Preferences module. The names are quite different, so this is easy to miss.

## Using Web Storage

- [Read and Write Values](./ws-read-write.md)
- [Delete Keys](./ws-delete.md)
- [Enumerate Keys](./ws-enumerate.md)

## Using ECMA-419 Key-Value Store

- [Read and Write Values](./kvp-read-write.md)
- [Change Data Formats](./kvp-data-formats.md)
- [Delete Keys](./kvp-delete.md)
- [Enumerate Keys](./kvp-enumerate.md)

## How to Choose

If Web Storage works for your project, you should use it. Because `localStorage` is a well-known web standard, many developers are already familiar with it, and it requires the least code. Web Storage values are always strings, so binary and structured data must be encoded, for example as Base64 or JSON.

The Key-Value store is a better choice when you work directly with binary data. It reads and writes [Byte Buffers](https://419.ecma-international.org/#byte-buffer) by default and supports several numeric and string [data formats](./kvp-data-formats.md). It also has lower runtime overhead and lets you iterate a domain's keys directly.

## Setting Up Web Storage

To access key-value pairs using Web Storage, a Web Storage instance must first be bound to a Key-Value store domain. Because `localStorage` is commonly used on the Web, this example binds the domain "local" to the global variable `localStorage`. You can create several instances of Web Storage, for example, a `sessionStorage` for Web compatibility.

```js
import WebStorage from "webstorage";

const kvp = device.keyValue.open({
	path: "local"
});
globalThis.localStorage = new WebStorage(kvp);
```

When building with `mcpack` or developing for Alloy on Pebble OS, this step is unnecessary as both create `localStorage` for you.

## Building with mcconfig

Include the Web Storage manifest in your project's `manifest.json`. It includes the Key-Value store, so no other manifest is needed:

	$(MODDABLE)/modules/web/webstorage

Then, import the module in your JavaScript source code:

```js
import WebStorage from "webstorage";
```

To use the Key-Value store directly, include its manifest:

	$(MODDABLE)/modules/io/storage/manifest.json

Then, import the module in your JavaScript source code:

```js
import keyValue from "embedded:storage/key-value";
```

## Building with mcpack

When building with `mcpack`, the Web Storage manifest is included automatically when your project uses the `localStorage` global. If your project uses `webstorage` with an import statement, it's manifest is automatically included.

To use the Key-Value store directly, import the module in your JavaScript source code. `mcpack` automatically includes the manifest.

```js
import keyValue from "embedded:storage/key-value";
```

## Learn More

- Documentation
	- [Web Storage API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Storage_API) on MDN
	- [class Preference](../../documentation/files/files.md#preference) in Moddable SDK — the module replaced by the Key-Value store
- Standards
	- [Web storage](https://html.spec.whatwg.org/multipage/webstorage.html) in the WHATWG HTML Standard
	- [Key-Value store](https://419.ecma-international.org/#storage-keyvalue) in ECMA-419
- Example
	- [Web Storage example](../../examples/io/storage/webstorage/main.js) in Moddable SDK
- Implementations
	- [Web Storage](../../modules/web/webstorage/webstorage.js) in Moddable SDK
	- [Key-Value store](../../modules/io/storage/) in Moddable SDK
- TypeScript Declarations
	- [Web Storage](../../typings/web/webstorage.d.ts) in Moddable SDK
	- [Key-Value store](../../typings/embedded/storage/key-value.d.ts) in Moddable SDK
