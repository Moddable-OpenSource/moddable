---
name: Immutable ArrayBuffers
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

An immutable `ArrayBuffer` is a read-only instance of an `ArrayBuffer`. You create an immutable `ArrayBuffer` from a regular read/write `ArrayBuffer`.

The `transferToImmutable()` method moves the buffer to a new read-only `ArrayBuffer`. The original `ArrayBuffer` can no longer be used.

```js
const readWriteBuffer = new ArrayBuffer(64);
const readOnlyBuffer = readWriteBuffer.transferToImmutable();
```

---

Attempting to modify an immutable `ArrayBuffer` throws an exception.

```js
const readOnlyBuffer = (new ArrayBuffer(64)).transferToImmutable();
const bytes = new Uint8Array(readOnlyBuffer);

let byte = bytes[0];
// => byte == 0

bytes[0] = 1;
// => exception thrown
```

---

Your code can check if an `ArrayBuffer` is immutable using the `immutable` property.

```js
const readOnlyBuffer = (new ArrayBuffer(64)).transferToImmutable();
// => readOnlyBuffer.immutable => true

const readWriteBuffer = new ArrayBuffer(64);
// => readWriteBuffer.immutable => false
```

---

You can create an immutable `ArrayBuffer` from just part of an `ArrayBuffer` using `sliceToImmutable()`. Unlike `transferToImmutable()`, `sliceToImmutable()` does not modify the original `ArrayBuffer` so it may still be used. Any changes to the original `ArrayBuffer` are not reflected in the immutable copy created by `sliceToImmutable()`.

```js
const readWriteBuffer = new ArrayBuffer(1024);
const readWriteBytes = new Uint8Array(readWriteBuffer);
readWriteBytes[100] = 1;

const readOnlyBuffer = readWriteBuffer.sliceToImmutable(100, 150);
// => readOnlyBuffer.byteLength == 50

const readOnlyBytes = new Uint8Array(readOnlyBuffer);
// => readOnlyBytes[0] == 1

readWriteBytes[100] = 2;
// => readOnlyBytes[0] == 1;
```
