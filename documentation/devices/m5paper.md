# Moddable SDK support for M5 Paper
Updated November 2, 2021

## Setup, build, run

There are several example applications in the Moddable SDK that show how to take make best use of the M5 Paper and its See the [ePaper blog](https://blog.moddable.com/blog/epaper#examples) post for details.

Just `cd` to the directory of the example and build as usual:

```
mcconfig -d -m -p esp32/m5paper
```

## macOS

The USB driver situation for M5Paper on macOS is a little tricky:

- Run at least macOS Big Sur
- Install the driver referenced in this [issue](https://github.com/Xinyuan-LilyGO/LilyGo-T-Call-SIM800/issues/139#issuecomment-904390716)

## Port Status

The following are implemented and working:

- EPD display driver
- GT911 touch driver
- SHT30 temperature/humidity sensor
- A / B / C buttons 
- RTC

> *Note*: The I2C address of the GT911 touch controller floats. The implementation tries both addresses 0x14 and 0x5D. This is handled in host provider's Touch constructor -- not in driver and not in user script If 0x14 fails, an exception is thrown before it retries at 0x5D. If you encounter this, just hit Go in xsbug.

## Display Driver

The display driver is a [Poco `PixelsOut`](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/commodetto/commodetto.md#pixelsout-class) implementation. This allows it to use both the [Poco](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/commodetto/poco.md) graphics APIs and[ Piu](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md) user interface framework from the Moddable SDK.

While many existing Poco and Piu examples run with the EPD, most are not practical. Because they were designed for a small color LCD with a high refresh rate, their appearance on a big gray display with a low refresh rate is often silly. We need some examples designed for this display.

The display driver is written entirely in JavaScript. It uses [Ecma-419 IO](https://419.ecma-international.org/#-9-io-class-pattern) APIs for all hardware access. Performance is excellent, often faster than the EPD class built into the native M5Paper library. One reason for this is that Poco can render directly to 4-bit gray pixels, eliminating the need for pixel format conversion. Another reason is that the SPI transfers to the display controller bulk transfer of thousands of pixels at a time, rather than four at a time. This reduces the number of bits transferred by over half.

Memory use is also quite low. There is no frame buffer in ESP32 memory: rendered pixels are sent directly to the display from a 16 line render buffer (about 8 KB).

Using the `continue` feature of Poco, it is possible to update several areas of the screen while only refreshing the EPD panel once. This allows for very efficient updates -- the least possible amount of memory is transferred and only one long panel flash occurs. The Piu balls example is a good way to see this in action - only the ball images (not the empty space around them) are transferred to the display and only the rectangle that encloses the four balls flashes on the display panel.

The rotation feature of the display controller is supported, allowing no-overhead rotation at 0, 90, 180, and 270 rotations.

### Update Modes
The display controller supports several different [update modes](https://github.com/phoddie/m5paper/blob/f0b79e0a0579c0dbdb1bb4445dc6acf501403681/targets/m5paper/it8951.js#L82-L93). The optimal mode depends on the content being drawn. The mode may be changed on each frame. The default mode is `GLD16`. To change the mode:

```js
screen.config({updateMode: "A2"});
```

### Image Filters
The display driver supports several different [pixel filters](https://github.com/phoddie/m5paper/blob/4110701c8084c07d7f777a44e17e970ffd18f729/targets/m5paper/it8951.js#L342-L349). These filter adjust the luminance of the pixels. The are useful for optimizing image and applying special effects. The default filter is "none". The filter may be changed on each frame. To change the filter:

```js
screen.config({filter: "negative"});
```

The filters are a `Uint8Array` of 16 values. To set your own filter, instead of using one of the built-in filters:

```js
let filter = new Uint8Array(16);
// code here to initialize filter
screen.config({filter});
```
