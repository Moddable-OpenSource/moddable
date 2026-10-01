---
name: Get Microseconds
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

Some platforms support retrieving the system elapsed time with microsecond precision. `Time.microseconds` is similar to `Time.ticks` but with units of microseconds instead of milliseconds

Only use microseconds when the precision is necessary as microseconds are more expensive to retrieve than milliseconds.

```js
import Time from "time";

trace(`Time.microseconds ${Time.microseconds}\n`);
```
---

To see if `Time.microseconds` is supported on your device, check for the presence of the `microseconds` property:

```js
if ("microseconds" in Time)
	trace("microseconds available\n");
else
	trace("microseconds unavailable\n");
```
