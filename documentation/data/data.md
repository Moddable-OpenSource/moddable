# Data
Copyright 2017-2022 Moddable Tech, Inc.<BR>
Revised: September 8, 2022

## Table of Contents

* [Base64](#base64)
* [Hex](#hex)
* [CRC](#crc)
* [QRCode](#qrcode)
* [TextDecoder & Text Encoder](#text)
* [Inflate & Deflate (zlib)](#zlib)
* [URL & URLSearchParams](#url)

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

The optional separator must be a single character in the 7-bit ASCII range.

### `static toString(buffer [[, separator], hexChars]);`

The `toString` function converts a buffer to a hexadecimal encoded `String`.

```js
let buffer = Hex.toBuffer("0123456789abcdef");
let s0 = Hex.toString(buffer, ".");
// s0 is 01.23.45.67.89.AB.CD.EF
let s1 = Hex.toString(buffer);
// s1 is 0123456789ABCDEF
```

The optional separator must be a single character in the 7-bit ASCII range.

The optional `hexChars` argument may contain 16 characters to use for the hexadecimal encoding. The characters must be 7-bit ASCII:

```js
let buffer = Hex.toBuffer("0123456789abcdef");
let s0 = Hex.toString(buffer, "-", "0123456789abwxyz");
// s0 is 01-23-45-67-89-ab-wx-yz
```

<a id="crc"></a>
## class CRC8, CRC16

The `CRC8` and `CRC16` classes calculate CRC checksums on data.

```
import {CRC8} from "crc";
import {CRC16} from "crc";
```

Include the module's manifest to use it in a project:

```json
	"include": [
		"$(MODULES)/data/crc/manifest.json"
	]
```

#### `CRC8(polynomial [, initial [, reflectInput [, reflectOutput [, xorOutput]]]])`
#### `CRC16(polynomial [, initial [, reflectInput [, reflectOutput [, xorOutput]]]])`

The `CRC8` and `CRC16` functions take a number of options used to specify the CRC checksum to calculate. 

| Parameter | Default | Description |
| :---: | :---: | :--- |
| `polynomial` | (none) | Polynomial to use (required) |
| `initial` | `0` | Initial CRC accumulator value (optional) |
| `reflectInput` | `false` | If `true`, each input byte is reflected (bits used in reverse order) before being used (optional) |
| `reflectOutput` | `false` | If `true`, each output byte is reflected before being returned. The output reflection is done over the whole CRC value (optional) |
| `xorOutput` | `0` | Value to XOR to the final CRC value (optional) |

The `polynomial`, `initial` and `xorOutput` values are 8-bit integers for CRC8 and 16-bit integers for CRC16.

The [crc example](../../examples/data/crc/main.js) demonstrates the definition of the parameters for a number of common CRC checksums:

- `CRC-8` 
- `CRC-8/CDMA2000` 
- `CRC-8/DARC` 
- `CRC-8/DVB-S2` 
- `CRC-8/EBU` 
- `CRC-8/I-CODE` 
- `CRC-8/ITU` 
- `CRC-8/MAXIM` 
- `CRC-8/ROHC` 
- `CRC-8/WCDM` 
- `CRC-16/CCITT-FALSE` 
- `CRC-16/ARC` 
- `CRC-16/ARG-CCITT` 
- `CRC-16/BUYPASS` 
- `CRC-16/CDMA2000` 
- `CRC-16/DDS-110` 
- `CRC-16/DECT-R` 
- `CRC-16/DECT-X` 
- `CRC-16/DNP` 
- `CRC-16/EN-13757` 
- `CRC-16/GENIBUS` 
- `CRC-16/MAXIM` 
- `CRC-16/MCRF4XX` 
- `CRC-16/RIELLO` 
- `CRC-16/T10-DIF` 
- `CRC-16/TELEDISK` 
- `CRC-16/TMS37157` 
- `CRC-16/USB` 
- `CRC-A` 
- `CRC-16/KERMIT` 
- `CRC-16/MODBUS` 
- `CRC-16/X-25` 
- `CRC-16/XMODE` 


### `close()`

The `close` function frees resources associated with the CRC checksum calculation.

### `checksum(buffer)`

The `checksum` function applies the CRC calculation to the data provided in `buffer`. The CRC checksum is returned.

The `buffer` parameter may be a `String` or buffer.

The `checksum` function may be called multiple times. Each time it is called the CRC updated and returned. Call the `reset` function to start a new calculation.

### `reset()`

The `reset` function clears the CRC accumulator to the `initial` value.

<a id="qrcode"></a>
## class QRCode
The `QRCode` class generates QR Code data from Strings and buffers. The data may then be rendering in various ways. Extensions are provided to [Poco](https://github.com/Moddable-OpenSource/moddable/tree/public/modules/commodetto/qrcode) and [Piu](https://github.com/Moddable-OpenSource/moddable/tree/public/modules/piu/MC/qrcode) to efficiently render QR Codes. The core implementation is the QR Code Generator Library from [Project Nayuki](https://www.nayuki.io/page/qr-code-generator-library).

```js
import qrCode from "qrcode";
```

Include the module's manifest to use them in a project:

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
| `maxVersion` |  A number between 1 and 40 inclusive indicating the maximum version of the generated QR Code. The version number determines the amount of data the QR Code can contain. The implementation uses the minimum version number possible for the size of the data provided. This property is optional and defaults to 40. |
| `input` |  A `String` or buffer containing the data to encode into the QR Code. This property is required. |

The `qrCode` function throws an exception if it detects invalid parameters or that there is not enough memory to generate the QR Code.

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

The `TextDecoder` implements the [TextDecoder class](https://developer.mozilla.org/en-US/docs/Web/API/TextDecoder) as [specified by WHATWG](https://encoding.spec.whatwg.org/#interface-textdecoder). It accepts only UTF-8 input data.

The `TextEncoder` implements the [TextEncoder class](https://developer.mozilla.org/en-US/docs/Web/API/TextEncoder) as [specified by WHATWG](https://encoding.spec.whatwg.org/#interface-textencoder). The implementation includes `encodeInto()`.

<a id="zlib"></a>
## zlib: class Inflate and class Deflate

The `Inflate` and `Deflate` classes implement zlib decompression and compression. The JavaScript API follows [the API](http://nodeca.github.io/pako/) defined by the [pako](https://github.com/nodeca/pako) library.

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

The [inflate example](../../examples/data/inflate/main.js) demonstrates how to decompress data as a one-shot operation and using the `onData` callback for streaming.

> **Note**: A significant amount of memory is required for zlib decompression and especially for compression. These libraries may not work on all microcontrollers because of memory constraints.

<a id="url"></a>
## class URL and class URLSearchParams 
The `URL` and `URLSearchParams` classes provide utilities for working with URLs and their search parameters. 

```js
import URL from "url";
import {URL, URLSearchParams} from "url";
```

Include the module's manifest to use it in a project:

```json
	"include": [
		"$(MODULES)/data/url/manifest.json"
	]
```

`URL` implements the [URL class](https://developer.mozilla.org/en-US/docs/Web/API/URL) as [specified by WHATWG](https://url.spec.whatwg.org/#url-class). The implementation fully conforms to the standard with two exceptions: [Punycode](https://en.wikipedia.org/wiki/Punycode) and [IDNA](https://www.unicode.org/reports/tr46/) support are unimplemented. These are used primarily for the display and safe handling of user-entered URLs in browsers, which are not generally a concern on embedded systems. With some effort (and increase in code size), the implementation could support both.

`URLSearchParams` implements the [URLSearchParams class](https://developer.mozilla.org/en-US/docs/Web/API/URL) as [specified by WHATWG](https://url.spec.whatwg.org/#urlsearchparams).

[Tests for both](https://github.com/Moddable-OpenSource/moddable/tree/public/tests/modules/data/url) are included in the Moddable SDK. They are based on the tests used to validate these APIs in web browsers.
