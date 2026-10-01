---
name: Operate on Bits
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-10-01
---

Embedded JavaScript developers often need to operate on the bits of an integer value. JavaScript supports this, even though it doesn't have an integer data type. The XS JavaScript engine is optimized for integer operations. With a little care, you can get the fastest possible bit operations and avoid some subtle bugs.

---

When OR-ing values together always use the OR operator `|` and never `+`. The OR operator is guaranteed to perform an integer operation while the addition operator can operate on many kinds of values including strings, floating point values, and `BigInt`.

```js
/* BEFORE */
let flags = Options.cache + Options.wrap;

let flags = Options.cache;
flags += Options.wrap;

/* AFTER */
let flags = Options.cache | Options.wrap;

let flags = Options.cache;
flags |= Options.wrap;
```

---

When shifting values, use the shift operators instead of multiplying or dividing by powers of two. The shift operators ensure the values are 32-bit integers, which is what you expect for bit operations. Multiplying and dividing by powers of two are slower and may give unexpected results.

```js
let value = 0x8000_0000;
let multiplied = value * 2;
// => multiplied is 4294967296

let value = 0x8000_0000;
let shifted = value << 1;
// => shifted is 0

let value = 1;
value /= 2;
// => value is 0.5

let value = 1;
value >>= 1;
// => value is 0
```
