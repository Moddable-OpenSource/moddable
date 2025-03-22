# Streams
Revised: January 24, 2024

This directory contains an experimental implementation of [web streams](https://streams.spec.whatwg.org). It is based on the [reference implementation](https://github.com/whatwg/streams).

The purpose of this work is to evaluate the feasibility and usefulness of web streams on resource-constrained embedded devices.

Starting with the reference implementation helps to ensure that the behaviors are conformant with the web streams specification. To verify conformance, the implementation has been tested using the Web Platform Tests for web streams ([more below](#tests)).

The reference implementation has many opportunities for optimization of both memory use and performance. This is an area for future work.

<a id="modules"></a>
## `modules` directory
The `modules` directory contains the JavaScript implementation of web streams.

### streams.js

This is the web streams implementation itself, derived from the [reference implementation](https://github.com/whatwg/streams) These are some of the differences from the original reference implementation:

- Source code is in a single file rather than spread across multiple files
- Modified to eliminate dependencies on the web platform
- Changes made to improve efficiency when running under XS
- Packaged as a standard JavaScript module (ESM)

The streams module exports the following:

- AbortController
- AbortSignal
- ByteLengthQueuingStrategy
- CountQueuingStrategy
- DOMException
- ReadableByteStreamController
- ReadableStream
- ReadableStreamBYOBReader
- ReadableStreamBYOBRequest
- ReadableStreamDefaultController
- ReadableStreamDefaultReader
- TransformStream
- TransformStreamDefaultController
- WritableStream
- WritableStreamDefaultController
- WritableStreamDefaultWriter
- TextDecoderStream
- TextEncoderStream

On the web, these exported classes are global variables. To run web examples in a Moddable application, please provide the globals:

```js
import * as streams from "streams";
for (let key in streams)
	globalThis[key] = streams[key];
```

For compatibility with web platform tests, the `TextDecoderStream` and `TextEncoderStream` classes currently access the `TextDecoder` and `TextEncoder` classes as globals.

If you want to use the `TextDecoderStream` and `TextEncoderStream` classes in a Moddable application, please provide the `TextDecoder` and `TextEncoder` classes as globals:

```js
import TextDecoder from "text/decoder";
globalThis.TextDecoder = TextDecoder;
import TextEncoder from "text/encoder";
globalThis.TextEncoder = TextEncoder;
```

### iostreams.js

The IO Streams module supports ECMA-419 IO using streams. The module exports two mixins, which create `ReadableStream` and `WritableStream` subclasses based on a class that conforms to the [ECMA-419 IO class pattern](https://419.ecma-international.org/#-9-io-class-pattern). The `button` example shows how to use these mixins.

### sensorstreams.js

The IO Streams module supports ECMA-419 sensors using streams. The module exports a mixin which creates a `ReadableStream` subclass based on an a class that conforms to the [ECMA-419 Sensor class pattern](https://419.ecma-international.org/#-13-sensor-class-pattern). The `touch` example shows how to use this mixin.

<a id="examples"></a>
## `examples` directory
This directory contains several Moddable SDK example applications that use streams. The examples have all been successfully run on [Moddable Two](https://www.moddable.com/moddable-two), a development board built around the ESP32 microcontroller. They should run on other ESP32-based devices, though configuration changes may be necessary. Because of code size and RAM requirements, the examples may not fit into less capable microcontrollers.

### button

This example revisits Moddable [IO button example](https://github.com/Moddable-OpenSource/moddable/tree/public/examples/io/digital/button). It turns an LED on and off based on the state of a button.

Using to the mixins exported by `iostreams.js`, the `Digital` class becomes both a `ReadableStream` subclass for the button and a `WritableStream` subclass for the LED.

There is also a `TransformStream` to invert the value.

To build:

```shell
cd /path/to/streams/examples/button
mcconfig -d -m -p esp32/moddable_two_io
```

Note that there is no console output when the LED changes state; you must watch the LED itself to see the state changes.

### touch

Thanks to the mixin exported by `sensorstream.js`, the `Touch` class from `embedded:sensor/Touch/FT6x06` becomes a `ReadableStream` subclass.

Since `ReadableStream` provides an async iterator, the stream of points is read with a `for await` loop

To build:

```shell
cd /path/to/streams/examples/touch
mcconfig -d -m -p esp32/moddable_two_io
```

### fetch

This example implements the web standard[ `fetch()` function](https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API) to make an HTTP request, using Moddable SDK's implementation of the [ECMA-419 HTTP Client class pattern](https://419.ecma-international.org/#-20-http-client-class-pattern) and the `ReadableStream` class.

To build:

```shell
cd /path/to/streams/examples/fetch
mcconfig -d -m -p esp32/moddable_two_io ssid=<SSID> password=<PASSWORD>
```

The fetch example also runs on the simulator:

```shell
mcconfig -d -m
```

<a id="tests"></a>
## `wpt` tests directory

This directory contains web stream unit tests extracted from the [Web Platform Tests Project](https://github.com/web-platform-tests/wpt).

To run the tests, you need to build [xst](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/xs/xst.md), the XS test tool. Then:

```shell
cd /path/to/streams/wpt
./iterate.sh
```

The vast majority of tests pass - 1168 of 1203 (97.1%). Of the 35 failures, 25 are are because they use  text formats unsupported by `TextDecoder` on embedded, not because of the streams implementation itself. Setting those aside, 99.2% of the tests pass.
