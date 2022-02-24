# Data

Copyright 2017-2022 Moddable Tech, Inc.<BR>
Revised: February 23, 2022

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## Table of Contents

* [Base64](#base64)
* [Hex](#hex)
* [QRCode](#qrcode)
* [Text](#text)
* [zlib](#zlib)

<a id="base64"></a>
## class Base64

The `Base64` class provides static functions to encode and decode using the Base64 algorithm defined in [RFC 4648](https://tools.ietf.org/html/rfc4648).

```js
import Base64 from "base64";
```

Include the module's manifest to use it in a project:

```json
	"include": [
		"$(MODULES)/data/base64/manifest.json"
	]
```

### `static decode(str)`

The `decode` function takes a `String` encoded in Base64 and returns an `ArrayBuffer` with the decoded bytes.

```js
trace(Base64.decode("aGVsbG8sIHdvcmxk") + "\n");
// output: "hello, world"
```

### `static encode(data)`

The `encode` function takes a `String` or buffer and returns an `ArrayBuffer` containing the data with Base64 encoding applied.

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

Include the module's manifest to use it in a project:

```json
	"include": [
		"$(MODULES)/data/hex/manifest.json"
	]
```

### `static toBuffer(string [, separator])`

The `toBuffer` function converts a [hexadecimal](https://en.wikipedia.org/wiki/Hexadecimal) encoded `String`, with optional separator, to an `ArrayBuffer`.

```js
let b0 = Hex.toBuffer("0123456789abcdef");
let b1 = Hex.toBuffer("01:23:45:67:89:AB:CD:EF", ":");
```

The hexadecimal digits may be uppercase or lowercase. If the optional separator argument is provided, it must appear between each pair of hexadecimal digits.

### `static toString(buffer [, separator]);`

The `toString` function converts a buffer to a hexadecimal encoded `String`.

```js
let buffer = Hex.toBuffer("0123456789abcdef");
let s0 = Hex.toString(buffer, ".");
// s0 is 01.23.45.67.89.AB.CD.EF
let s1 = Hex.toString(buffer);
// s1 is 0123456789ABCDEF
```

<a id="qrcode"></a>
## class QRCode
The `QRCode` class generates QR Code data from Strings and buffers. The data may then be rendering in various ways. Extensions are provided to [Poco](https://github.com/Moddable-OpenSource/moddable/tree/public/modules/commodetto/qrcode) and [Piu](https://github.com/Moddable-OpenSource/moddable/tree/public/modules/piu/MC/qrcode) to efficiently render QR Codes. The core implementation is the QR Code Generator Library from [Project Nayuki](https://www.nayuki.io/page/qr-code-generator-library).

```js
import qrCode from "qrcode";
```

Include the modules' manifest to use them in a project:

```json
	"include": [
		"$(MODULES)/data/qrcode/manifest.json"
	]
```

For additional details see the article [QR Code module for the Moddable SDK](https://blog.moddable.com/blog/qrcode/).

### `qrCode(options)`

The `qrCode` function accepts an options object that describes the QR Code to generate. It returns an `ArrayBuffer` where each byte is 0 or 1 for a white or black pixel. The returned buffer has a `size` property that indicate the number of cells in one dimension of the generated QR Code.

The following properties are supported in the options object:

| Property | Description |
| :---: | :--- |
| `maxVersion` |  A number between 1 and 40 inclusive indicating the maximum version of the generated QR Code. The version number determines the amount of data the QR Code can contain. This property is optional and defaults to 40. |
| `input` |  A `String` or buffer containing the data to encode into the QR Code. This property is required. |

```js
const code = qrCode({input: "Welcome to Moddable", maxVersion: 4});

// trace QR Code to console
code = new Uint8Array(code);
for (let y = 0; y < = code.size; y++) {
    for (let x = 0; x < = code.size; x++)
        trace(code[(y * code.size) + x] ? "X" : ".", "\n");
    trace("\n");
}
```

<a id="text"></a>
## class TextDecoder and class TextEncoder

The `TextDecoder` and `TextEncoder` classes implement conversion between JavaScript strings and memory buffers containing UTF-8 data.

```js
import TextDecoder from "text/decoder";
import TextEncoder from "text/encoder";
```

Include the modules' manifest to use them in a project:

```json
	"include": [
		"$(MODULES)/data/text/decoder/manifest.json",
		"$(MODULES)/data/text/encoder/manifest.json"
	]
```

The `TextDecoder` implements the [TextDecoder class](https://developer.mozilla.org/en-US/docs/Web/API/TextDecoder) as [specified by WhatWG](https://encoding.spec.whatwg.org/#interface-textdecoder). It accepts only UTF-8 input data.

The `TextEncoder` implements the [TextEncoder class](https://developer.mozilla.org/en-US/docs/Web/API/TextEncoder) as [specified by WhatWG](https://encoding.spec.whatwg.org/#interface-textencoder). The implementation includes `encodeInto()`.

<a id="zlib"></a>
## zlib: class Inflate and class Deflate

The `Inflate` and `Deflate` classes implement zlib decompression and compression. The JavaScript API follows the API defined by the [pako](https://github.com/nodeca/pako) library.

```js
import Inflate from "inflate";
import Deflate from "deflate";
```

Include the modules' manifest to use them in a project:

```json
	"include": [
		"$(MODULES)/data/zlib/manifest_deflate.json",
		"$(MODULES)/data/zlib/manifest_inflate.json"
	]
```

> **Note**: A significant amount of memory is required for zlib decompresssion and especially for compression. These libraries may not work on all microcontrollers because of memory constraints.
