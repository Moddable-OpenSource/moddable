---
name: Write File
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

To write a file, first open it in a write mode such as "r+", then call `write()`. Pass a [Byte Buffer](https://419.ecma-international.org/#byte-buffer) to write as the first argument. The position to write to the file must be passed as the second argument as there is no current position.

```js
const help = device.files.openFile({
	path: "docs/help.txt",
	mode: "r+"
});
const bytes = Uint8Array.of(0, 1, 2, 3);
help.write(bytes, 0);
help.close();
```

---

To write a string to a file, convert the string to a buffer. This example uses `ArrayBuffer.fromString()` or you can use `TextEncoder`.

```js
const help = device.files.openFile({
	path: "docs/help.txt",
	mode: "r+"
});
const buffer = ArrayBuffer.fromString("hello");
help.write(buffer, 0);
help.close();
```

---

To copy a file, use `read()` and `write()` together.

```js
let position = 0;
const help = device.files.openFile({
	path: "docs/help.txt",
	mode: "r"
});
const copy = device.files.openFile({
	path: "docs/help-copy.txt",
	mode: "w"
});
do {
	const buffer = help.read(512, position);
	if (!buffer?.byteLength)
		break;
	copy.write(buffer, position);
	position += buffer.byteLength;
} while (true);
copy.close();
help.close();
```
