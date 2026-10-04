---
name: Read File
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

To read a file, first open it in read mode such as "r", then call `read()`. Pass the number of bytes to read as the first argument. The position to read from the file must be passed as the second argument as there is no current position.

The return value is an `ArrayBuffer`. If fewer than 50 bytes are available to read, the returned buffer contains all available bytes.

```js
const help = device.files.openFile({
	path: "docs/help.txt",
	mode: "r"
});
const buffer = help.read(50, 0);
trace(`${(new Uint8Array(buffer)).toHex()}\n`);
help.close();
```

---

To read a file into a memory buffer, use `status()` to determine the file size.

```js
const help = device.files.openFile({
	path: "docs/help.txt",
	mode: "r"
});
const size = help.status().size;
const buffer = help.read(size, 0);
trace(`${(new Uint8Array(buffer)).toHex()}\n`);
help.close();
```

---

To read a file into a string, first read it into a memory buffer, then convert it to a string. This example uses `String.fromArrayBuffer()` or you can use `TextDecoder`.

```js
const help = device.files.openFile({
	path: "docs/help.txt",
	mode: "r"
});
const size = help.status().size;
const buffer = help.read(size, 0);
trace(`${String.fromArrayBuffer(buffer)}\n`);
help.close();
```

Instead of each read creating a new memory buffer, you can read into an existing buffer. This can reduce the load on the garbage collector and execute faster. To read into a memory buffer, pass a [Byte Buffer](https://419.ecma-international.org/#byte-buffer) to `read()`. The return value is the number of bytes read.

This example reads a file 512 bytes at a time and traces them to the console. When there is no more to read, `read()` returns a falsy value.

```js
let position = 0;
const bytes = new Uint8Array(512);
const help = device.files.openFile({
	path: "docs/help.txt",
	mode: "r"
});
do {
	const count = help.read(bytes, position);
	if (!count)
		break;
	trace(`${(new Uint8Array(
		bytes.buffer, 0, count)).toHex()}\n`);
	position += count;
} while (true);
help.close();
```
