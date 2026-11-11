---
name: Streams
SPDX-FileCopyrightText: Copyright (c) 2026 Moddable Tech, Inc.
updated: 2026-11-11
---

The Moddable SDK provides a full implementation of WHATWG Streams as widely used on the Web platform. Streams are optimized for use on embedded devices. The implementation is deeply integrated with the XS JavaScript engine to minimize its code footprint and memory use, and to optimize performance. Developers can work with streams in exactly the same way as they do on the web because the implementation conforms to the relevant Web Platform Tests at a level comparable to web browsers.

## When to Use Streams
Web Streams are a powerful and convenient tool to organize complex data flows. That power requires overhead. Consequently, streams may not be the right tool where the highest possible performance or smallest footprint are a priority. If you are already familiar with streams, it can be quick to do a test to evaluate whether the implementation meets your needs.

## Where to Use Streams
Streams are a fundamental building block for many Web platform APIs. The Moddable SDK implements several of these Web APIs.

- [WebSocketStream](https://developer.mozilla.org/en-US/docs/Web/API/WebSocketStream) – An experiemntal stream-based WebSocket client that addresses shortcominngs in the standard `WebSocket`. See the [WebSocket Guide](../websocket/index.md) for more information.
- [fetch](https://developer.mozilla.org/en-US/docs/Web/API/Window/fetch) – Streams can be used for both the Request body and Response body to conveniently support bodies larger than available memory. As an added bonus, streams allows fetch() to support compressed response bodies. See the [HTTP Guide](../http/index.md) for more information. 
- [Web Serial](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API) – Access serial (UART) ports using readable and writable streams. Available on devices and the desktop simulator.

Of course, you can use streams in your own projects in many ways. The streams examples can give you ideas, secifically for embedded devices. Start with the [readme](../../examples/web/streams/readme.md). Then, check out the [button stream](../../examples/web/streams/button/main.js) and [touch stream](../../examples/web/streams/touch/main.js) examples.

## Building with mcconfig
The stream classes are implemented across several modules.

Include the core streams manifest in your project's `manifest.json`:

	$(MODDABLE)/modules/web/streams/all

Then, import the required constructors in your JavaScript source code:

```js
import {
	ReadableStream,
	ReadableStreamDefaultReader,
	ReadableStreamBYOBReader,
	ReadableStreamDefaultController,
	ReadableByteStreamController,
	ReadableStreamBYOBRequest,
	WritableStream,
	WritableStreamDefaultWriter,
	WritableStreamDefaultController,
	TransformStream,
	TransformStreamDefaultController,
	ByteLengthQueuingStrategy,
	CountQueuingStrategy
} from "web/streams";
```

To use the `DecompressionStream`, `TextDecoderStream`, and `TextEncoderStream` constructors, include their manifests in your project's `manifest.json`:

	$(MODDABLE)/modules/web/streams/decompression
	$(MODDABLE)/modules/web/streams/text/decoder
	$(MODDABLE)/modules/web/streams/text/encoder

Then, import the required constructors in your JavaScript source code:

```js
import DecompressionStream from "web/decompressionstream";
import TextDecoderStream from "web/textdecoderstream";
import TextEncoderStream from "web/textencoderstream";
```

## Building with mcpack
If your project uses stream constructors, `mcpack` automatically includes the corresponding manifests and initializes the globals.

The stream constructors are: `ReadableStream`, `ReadableStreamDefaultReader`, `ReadableStreamBYOBReader`, `ReadableStreamDefaultController`, `ReadableByteStreamController`, `ReadableStreamBYOBRequest`, `WritableStream`, `WritableStreamDefaultWriter`, `WritableStreamDefaultController`, `TransformStream`, `TransformStreamDefaultController`, `ByteLengthQueuingStrategy`, `CountQueuingStrategy`, `DecompressionStream`, `TextDecoderStream`, and `TextEncoderStream`.

## Learn More

- Documentation
	- [Streams API](https://developer.mozilla.org/en-US/docs/Web/API/Streams_API) on MDN
	- [Using readable streams](https://developer.mozilla.org/en-US/docs/Web/API/Streams_API/Using_readable_streams) on MDN
	- [Using readable byte streams](https://developer.mozilla.org/en-US/docs/Web/API/Streams_API/Using_readable_byte_streams) on MDN
	- [Using writable streams](https://developer.mozilla.org/en-US/docs/Web/API/Streams_API/Using_writable_streams) on MDN
	- [DecompressionStream](https://developer.mozilla.org/en-US/docs/Web/API/DecompressionStream) on MDN
	- [TextDecoderStream](https://developer.mozilla.org/en-US/docs/Web/API/TextDecoderStream) and [TextEncoderStream](https://developer.mozilla.org/en-US/docs/Web/API/TextEncoderStream) on MDN
- Standards
	- [WHATWG Streams Standard](https://streams.spec.whatwg.org/)
	- [ReadableStream](https://streams.spec.whatwg.org/#rs-class), [WritableStream](https://streams.spec.whatwg.org/#ws-class), and [TransformStream](https://streams.spec.whatwg.org/#ts-class) in the WHATWG Streams Standard
	- [DecompressionStream](https://compression.spec.whatwg.org/#decompression-stream) in the WHATWG Compression Standard
	- [TextDecoderStream](https://encoding.spec.whatwg.org/#interface-textdecoderstream) and [TextEncoderStream](https://encoding.spec.whatwg.org/#interface-textencoderstream) in the WHATWG Encoding Standard
- Examples
	- [Streams examples](../../examples/web/streams/readme.md) in Moddable SDK
	- [fetch example](../../examples/web/streams/fetch/main.js) in Moddable SDK
	- [decompress example](../../examples/web/streams/decompress/main.js) in Moddable SDK — `DecompressionStream` and `TextDecoderStream`
	- [button example](../../examples/web/streams/button/main.js) in Moddable SDK — ECMA-419 IO as readable and writable streams
	- [touch example](../../examples/web/streams/touch/main.js) in Moddable SDK — an ECMA-419 sensor as a readable stream
- Implementation
	- [Streams](../../modules/web/streams/all/) in Moddable SDK
	- [DecompressionStream](../../modules/web/streams/decompression/DecompressionStream.js) in Moddable SDK
	- [TextDecoderStream](../../modules/web/streams/text/decoder/TextDecoderStream.js) and [TextEncoderStream](../../modules/web/streams/text/encoder/TextEncoderStream.js) in Moddable SDK
- TypeScript Declarations
	- [Streams](../../typings/web/streams.d.ts) in Moddable SDK
	- [DecompressionStream](../../typings/web/decompressionstream.d.ts) in Moddable SDK
	- [TextDecoderStream](../../typings/web/textdecoderstream.d.ts) and [TextEncoderStream](../../typings/web/textencoderstream.d.ts) in Moddable SDK
