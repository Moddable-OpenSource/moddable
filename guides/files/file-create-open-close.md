---
name: Create, Open, and Close File
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

To create a new file, call `openFile()` with the mode set to "w+". 

```js
const help = device.files.openFile({
	path: "docs/help.txt",
	mode: "w+"
});
```

---

To open an existing file, call `openFile()` with the mode set to "r+" for read-write, and "r" for read-only.

```js
const help = device.files.openFile({
	path: "docs/help.txt",
	mode: "r+"
});
```

---

To close an open file, call `close()`.

```js
const help = device.files.openFile({
	path: "docs/help.txt",
	mode: "r+"
});
help.close();
```

---

To check if a file already exists, call `status()`. This example only creates the file if it does not already exist.

```js
const status = device.files.status("docs/help.txt");
if (!status.isFile()) {
	const help = device.files.openFile({
		path: "docs/help.txt",
		mode: "w+"
	});
	help.close();
}
```

---

The file instance may be used with Explicit Resource Management. The `using` here simplifies the previous example by eliminating the explicit `close()` of `help`.

```js
const status = device.files.status("docs/help.txt");
if (!status.isFile()) {
	using help = device.files.openFile({
		path: "docs/help.txt",
		mode: "w+"
	});
}
```
