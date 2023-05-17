# MP3 Decoder
Updated: May 15, 2023

A JavaScript class for decoding MP3 frames.

## Reference
```js
import MP3 from "mp3/decode";
```

The `constructor` takes no arguments.

```js
const mp3 = new MP3;
```

The `close` method frees all resources. As the MP3 decoder does reserve some memory, it is important to close it when done to release that memory.

```js
mp3.close();
```

The `decode` method decodes a single MP3 frame. The output is always mono (this is easy to change if necessary).

```js
mp3.decode(inputFrame, outputBuffer);
```

The `decode` method returns the number of bytes used or 0 if a frame could not be decoded. The number of samples decoded is set on the output buffer as the `samples` property. While `samples` is typically 1152 for MP3, other smaller values can also be present in the bitstream.

There is a static `scan` method which looks through a buffer to find the start of an MP3 frame and extract information from the header such as bit rate, sample rate, and channel count.

```js
let found = MP3.scan(buffer, start, end[, info]);
```

The return value is `undefined` if no frame could be found, otherwise it is an object. The object has `position` and `length` properties indicating the offset and size of the frame in the input buffer. The `length` value is an estimate and should always be at least as large as the frame. To determine the actual end of the frame, use the return value of the  `decode` method.

## Acknowledgements
The MP3 decoder is libmad, specifically the fork by Earle F. Philhower, III that he optimized for ESP8266. While not intended for use on ESP8266 in the Moddable SDK, its memory optimizations are always welcome. The tag used is 12131e9 from the [ESP8266Audio repository](https://github.com/earlephilhower/ESP8266Audio). There have been no changes to the sources beyond a few adjustments for the Moddable SDK build environment. Thank you to Earle for making this excellent work available.


## Licensing
The libmad MP3 decoder uses the GPL license. Please consider this if incorporating this module into commercial projects.
