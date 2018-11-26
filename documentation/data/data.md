# Data

Copyright 2017 Moddable Tech, Inc.<BR>
Revised: November 21, 2018

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## Table of Contents

* [Base64](#base64)
* [Hex](#hex)

<a id="base64"></a>
## class Base64

The `Base64` class provides static functions to encode and decode using the Base64 algorithm defined in [RFC 4648](https://tools.ietf.org/html/rfc4648).

```js
import Base64 from "base64";
```

### `static decode(str)`

The `decode` function takes a `String` encoded in Base64 and returns an `ArrayBuffer` with the decoded bytes.

```js
trace(Base64.decode("aGVsbG8sIHdvcmxk") + "\n");
// output: "hello, world"
```

### `static encode(data)`

The `encode` function takes a `String` or `ArrayBuffer` and returns an `ArrayBuffer` containing the data with Base64 encoding applied.

```js
trace(Base64.encode("hello, world") + "\n");
// output: "aGVsbG8sIHdvcmxk"
```

> **Note**: When a string is provided, its contents are treated as UTF-8 encoded characters when performing Base64 encoding.

<a id="hex"></a>
## class Hex

The `Hex` class provides static functions to convert between an `ArrayBuffer` and hexadecimal encoded `String` values.

```js
import Hex from "hex";
```

### `static toBuffer(string [, separator])`

The `toBuffer` function converts a [hexadecimal](https://en.wikipedia.org/wiki/Hexadecimal) encoded `String`, with optional separator, to an `ArrayBuffer`.

```js
let b0 = Hex.toBuffer("0123456789abcdef");
let b1 = Hex.toBuffer("01:23:45:67:89:AB:CD:EF", ":");
```

The hexadecimal digits may be uppercase or lowercase. If the optional separator argument is provided, it must appear between each pair of hexadecimal digits.

### `static toString(buffer [, separator]);`

The `toString` function converts an `ArrayBuffer` to a hexadecimal encoded `String`.

```js
let buffer = Hex.toBuffer("0123456789abcdef");
let s0 = Hex.toString(buffer, ".");
// s0 is 01.23.45.67.89.AB.CD.EF
let s1 = Hex.toString(buffer);
// s1 is 0123456789ABCDEF
```
