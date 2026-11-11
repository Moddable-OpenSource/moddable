---
name: Binary Data
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-11-11
---

These guides cover working with binary data in Embedded JavaScript — the standard [`ArrayBuffer`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer) and [`TypedArray`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/TypedArray) objects, conversions to and from strings, Base64, and hexadecimal, checksums, and compression.

Unlike most other guides, binary data is not a single API. Many operations use features built into the XS JavaScript engine, including web standard APIs and Moddable SDK extensions such as `ArrayBuffer.fromString()`. Others are provided by Moddable SDK modules that you add to your project only when needed. The [Building](#building) section lists the modules used by these guides.

## Strings

- [Convert String to ArrayBuffer](./string-to-arraybuffer.md)
- [Convert ArrayBuffer to String](./arraybuffer-to-string.md)
- [Convert ArrayBuffers to String](./arraybuffers-to-string.md)
- [Handle Errors Converting ArrayBuffer to String](./arraybuffer-to-string-errors.md)

## ArrayBuffers

- [Immutable ArrayBuffers](./read-only-arraybuffer.md)
- [Resize an ArrayBuffer](./resize-array-buffer.md)
- [Combine ArrayBuffers](./combining-arraybuffers.md)

## Base64 and Hexadecimal

- [Convert Base64 to Binary Data](./base64-to-binarydata.md)
- [Convert Binary Data to Base64](./binarydata-to-base64.md)
- [Convert Binary Data to Hex](./binarydata-to-hex.md)
- [Convert Hex to Binary Data](./hex-to-binarydata.md)

## Checksums

- [Calculate CRC for Binary Data](./calculate-crc-for-binarydata.md)

## Compression

- [Compress Binary Data – One Buffer](./compress-binarydata-onebuffer.md)
- [Compress Binary Data – Streaming](./compress-binarydata-streaming.md)
- [Decompress Binary Data – One Buffer](./decompress-binarydata-onebuffer.md)
- [Decompress Binary Data – Streaming](./decompress-binarydata-streaming.md)

## Building

Many operations in these guides are built into the XS JavaScript engine and require no manifest changes. These include converting between strings and `ArrayBuffer` with `ArrayBuffer.fromString()` and `String.fromArrayBuffer()`, working with immutable and resizable `ArrayBuffer`s, combining buffers with `concat()`, and Base64 and hexadecimal encoding with the `Uint8Array` methods `toBase64()`, `fromBase64()`, `toHex()`, and `fromHex()`.

Other operations are provided by Moddable SDK modules. 

| Feature | Import | Manifest |
|---------|--------|----------|
| `TextEncoder` | `text/encoder` | `$(MODDABLE)/modules/data/text/encoder` |
| `TextDecoder` | `text/decoder` | `$(MODDABLE)/modules/data/text/decoder` |
| CRC (`CRC8`, `CRC16`) | `crc` | `$(MODDABLE)/modules/data/crc` |
| Compression (`Deflate`) | `deflate` | `$(MODDABLE)/modules/data/zlib/manifest_deflate.json` |
| Decompression (`Inflate`) | `inflate` | `$(MODDABLE)/modules/data/zlib/manifest_inflate.json` |

When building with `mcconfig`, add the manifests needed to your project.

When building with `mcpack`, manifests are automatically included for any of these modules used in an `import` statement. Further, the `TextEncoder` and `TextDecoder` globals are initialized if your project uses them. 

## Learn More

- Documentation
	- [TextEncoder](https://developer.mozilla.org/en-US/docs/Web/API/TextEncoder) and [TextDecoder](https://developer.mozilla.org/en-US/docs/Web/API/TextDecoder) on MDN
	- [Uint8Array.fromBase64()](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Uint8Array/fromBase64) and [Uint8Array.fromHex()](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Uint8Array/fromHex) on MDN
	- [class CRC8, CRC16](../../documentation/data/data.md#crc) in Moddable SDK
	- [zlib: class Inflate and class Deflate](../../documentation/data/data.md#zlib) in Moddable SDK
- Standards
	- [TextEncoder](https://encoding.spec.whatwg.org/#interface-textencoder) and [TextDecoder](https://encoding.spec.whatwg.org/#interface-textdecoder) in the WHATWG Encoding Standard
	- [Uint8Array to and from Base64 and Hex](https://tc39.es/proposal-arraybuffer-base64/) in TC39
- TypeScript Declarations
	- [TextEncoder](../../typings/text/encoder.d.ts) in Moddable SDK
	- [TextDecoder](../../typings/text/decoder.d.ts) in Moddable SDK
	- [CRC8, CRC16](../../typings/crc.d.ts) in Moddable SDK
	- [Deflate](../../typings/deflate.d.ts) in Moddable SDK
	- [Inflate](../../typings/inflate.d.ts) in Moddable SDK
