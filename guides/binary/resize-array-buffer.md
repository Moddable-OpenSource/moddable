---
name: Resize an ArrayBuffer
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

An `ArrayBuffer` created with with the `maxByteLength` option may be resized.

```js
const buffer = new ArrayBuffer(0, {maxByteLength: 1024});
// => buffer.byteLength == 0

buffer.resize(48);
// => buffer.byteLength == 48
```

---

`maxByteLength` is the largest number of bytes that the `ArrayBuffer` may contain.

The XS JavaScript engine does not allocate `maxByteLength` bytes when the `ArrayBuffer` is created. It only allocates as many bytes are used.


```js
const buffer = new ArrayBuffer(64, {maxByteLength: 1024});

buffer.resize(1024);
// => ok

buffer.resize(1025);
// => throws exception
```

---

Sometimes you don't know the maximum size an `ArrayBuffer` will need, but you must provides a value for `maxBytesLength`. You can safely use a very large number because XS doesn't pre-allocate `maxByteLength` bytes.

Attempting to `resize()` beyond available memory will fail.

```js
const buffer = new ArrayBuffer(64, {maxByteLength: 1024 * 1024 * 1024});
```

---

When a resizable `ArrayBuffer` is the backing store for `Uint8Array` or any other [`TypedArray`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray), the array's `length` automatically adjusts based on the `ArrayBuffer`.

```js
const buffer = new ArrayBuffer(512, {maxByteLength: 1024});

const bytes = new Uint8Array(buffer);
const words = new Uint16Array(buffer);
// => bytes.length == 512
// => words.length = 256

buffer.resize(1024);
// => bytes.length == 1024
// => words.length = 512

buffer.resize(9);
// => bytes.length == 9
// => words.length = 4
```

---

You can determine if an `ArrayBuffer` can be resized by checking its `resizable` property. Its `maxByteLength` property indicates the largest possible size of the buffer.

```js
const resizableBuffer = new ArrayBuffer(512, {maxByteLength: 1024});
// => resizableBuffer.resizable == true
// => resizableBuffer.maxByteLength == 1024
// => resizableBuffer.byteLength == 512

const buffer = new ArrayBuffer(512);
// => buffer.resizable == false
// => buffer.maxByteLength == 512
// => buffer.byteLength == 512
```
