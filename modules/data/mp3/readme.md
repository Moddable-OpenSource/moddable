# MP3 Decoder
Updated: May 11, 2023

A JavaScript class for decoding MP3 frames.

## Reference
The `constructor` takes no arguments.

The `close` method frees all resources. As the MP3 decoder does reserve some memory, it is important to close it when done to release that memory.

The `decode` method decodes a single MP3 frame. The output is always mono (this is easy to change if necessary). There is also a static `scan` method which looks through a buffer to find the start of an MP3 frame and extract information from the header such as bit rate, sample rate, and channel count.

## Acknowledgements
The MP3 decoder is libmad, specifically the fork by Earle F. Philhower, III that he optimized for ESP8266. While not intended for use on ESP8266 in the Moddable SDK, its memory optimizations are always welcome. The tag used is 12131e9 from the [ESP8266Audio repository](https://github.com/earlephilhower/ESP8266Audio). There have been no changes to the sources beyond a few adjustments for the Moddable SDK build environment. Thank you to Earle for making this excellent work available.


## Licensing
The libmad MP3 decoder uses the GPL license. Please consider this if incorporating this module into commercial projects.
