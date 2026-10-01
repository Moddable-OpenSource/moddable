---
name: Compress Binary Data – Streaming
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

The `Deflate` module provides [zlib](https://www.zlib.net) compression using a subset of the [Pako](https://github.com/nodeca/pako/blob/master/README.md) API.

---

Stream multiple uncompressed input data buffers to `Deflate` by setting the second argument to `push()` to `true` on only the last buffer.

```js
const deflator = new Deflate({});

const data = Uint8Array.of(0, 1, 2, 3, 4, 5, 6, 7);
for (let i = 0; i < 128; i++)
	deflator.push(data, 127 === i);
// => deflator.result is a Uint8Array with compressed data

deflator.close();
```

---

Stream the output of `Deflate` by implementing `onData()` and `onEnd()` callbacks. Streaming output is useful when the compressed data is larger than available memory.

```js
const deflator = new Deflate({});

deflator.onData = buffer => {
	// => buffer is an ArrayBuffer of compressed data
};

deflator.onEnd = error => {
	if (error) {
		// compression failed. handle error.
	}
	else {
		// compression successfully completed.
	}
};

const data = Uint8Array.of(0, 1, 2, 3, 4, 5, 6, 7);
for (let i = 0; i < 128; i++)
	deflator.push(data, 127 === i);

deflator.close();
```