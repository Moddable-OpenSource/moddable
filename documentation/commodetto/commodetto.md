# Commodetto
Copyright 2016 Moddable Tech, Inc.

Revised: October 10, 2016

Commodetto is a graphics library designed to bring modern user interface rendering to devices powered by a resource-constrained microcontroller. For many applications, Commodetto needs only a few kilobytes of RAM, including the assets for rendering the user interface.

## Overview
Commodetto consists of several parts:

* A lightweight Poco rendering engine--a display list renderer able to efficiently render a single scanline at a time, eliminating the need for a frame buffer
* Asset loaders for working with common file formats
* Pixel outputs for delivering rendered pixels to displays and files
* A JavaScript API for all features

Every part of Commodetto is a module, making it straightforward to deploy only necessary modules, add new modules, and replace existing modules. Even the rendering capabilities are a module, enabling integration of specialized rendering modules.

The Poco rendering engine contains only the most essential rendering operations: fill or blend a rectangle with a solid color, plus a small set of bitmap drawing operations, including copy, pattern fill, and alpha blend. Text is implemented externally to the rendering engine using Poco bitmap rendering operations, which enables integration of different engines for glyph generation, text layout, and text measurement.

Asset loaders prepare graphical assets, such as photos, user interface elements, and fonts, for rendering. Commodetto includes asset loaders for BMP images, JPEG photos, PNG images, NFNT fonts, and BMFont fonts. Additional asset loader modules may be added. The asset loaders enable many types of assets to be rendered directly from flash storage (for example, ROM) without having to be loaded into RAM.

Pixel outputs deliver rendered pixels to their destination. Commodetto includes modules to write to files and in-memory bitmaps. Modules may be added to send the pixels to a display over the transports supported by the host device, including SPI, I<sup>2</sup>C, serial, and memory-mapped ports.

### Native Data Types

Commodetto data types include pixel, coordinate, dimension, and bitmap. These types are described below along with the enumeration that defines pixel formats.

#### Pixel Type
To run well on resource-constrained hardware, Commodetto makes simplifying assumptions. One assumption is that only a single output pixel format is supported in a given deployment. A general-purpose graphics library needs to support many different output pixel formats to be compatible with the many different displays and file formats in use. Commodetto assumes the device it is deployed to connects to a single type of screen; this assumption reduces the code size and some runtime overhead.

Commodetto has the ability to support different pixel formats, with the choice of the pixel format to use set at compile time. Changing the output pixel format--for example, to use a different screen--simply requires recompiling Commodetto with a different output pixel format configured.

`CommodettoPixel` is the data type for pixel.

```c
typedef uint16_t CommodettoPixel;
```

<!-- jph v2 -->
The initial version of Commodetto supports 16-bit output pixels, in RGB 565 arrangement.

#### Pixel Formats
Commodetto supports multiple source pixel formats at the same time. This allows for efficient storage and rendering of different kinds of assets.

The output pixel format is always one of the supported source pixel formats. Other supported source pixel formats include 1-bit monochrome and 4-bit gray.

The `CommodettoBitmapFormat` enumeration defines the available pixel formats.

<!-- jph v2 -->
```c
typedef enum {
	kCommodettoBitmapRaw = 1,
	kCommodettoBitmapPacked,
	kCommodettoBitmapMonochrome,
	kCommodettoBitmapGray16
} CommodettoBitmapFormat;
```

* **Raw** -- Pixels are in the output pixel format. The format varies depending on how Commodetto is built.
* **Packed** -- Pixels are in the output pixel format, and packed together using an RLE algorithm for efficient storage and rendering. The encoding algorithm is described in the source code file `commodetto_RLEOut.js`.
* **Monochrome** -- Pixels are 1-bit data, packed into bytes in which the high bit of the byte is the leftmost pixel.
* **Gray16** -- Pixels are 4-bit data, where 0 represents white, 15 represents black, and the values in between are proportionally interpolated gray levels. The pixels are packed into bytes in which the high nybble is the leftmost pixel.

#### Coordinate Type

The `CommodettoCoordinate` type specifies coordinates.

```c
typedef int16_t CommodettoCoordinate;
```

The coordinate type is signed to enable applications to render objects that overlap the screen on any edge.

For devices with very small displays (128 pixels or fewer in each dimension), `CommodettoCoordinate` may be redefined as an 8-bit value--for example, `int8_t`--to reduce memory requirements. When doing this, application code must take care not to use coordinates smaller than -128 or larger than 127, or the results will be unpredictable.

#### Dimension Type

`CommodettoDimension` is the type used to specify dimensions.

```c
typedef uint16_t CommodettoDimension;
```

The dimension type is unsigned, as Commodetto assigns no meaning to a negative width or height.

`CommodettoDimension` may be redefined as `uint8_t` on devices with very small displays (256 pixels in each dimension) to reduce memory use. When doing this, application code must take care not to specify dimensions larger than 255 pixels.

#### Bitmap Type

The `CommodettoBitmap` structure defines a bitmap.

```c
typedef struct {
	CommodettoDimension		w;
	CommodettoDimension		h;
	CommodettoBitmapFormat	format;
	int8_t					havePointer;
	union {
		void				*data;
		int32_t				offset;
	} bits;
} CommodettoBitmapRecord, *CommodettoBitmap;
```

The `w` and `h` fields are the width and height of the bitmap in pixels. The coordinates of the top-left corner of a bitmap are always `{x: 0, y: 0}`, so the bitmap does not contain *x* and *y* coordinates. The `format` field indicates the pixel format of the pixels in the bitmap.

The remaining fields indicate where the pixel data is located. If the `havePointer` field is nonzero, the `data` field in the union is valid and points to the pixel data.

```c
CommodettoBitmap bitmap = ...  /* Get bitmap */;
CommodettoPixel *pixels = bitmap.bits.data;
```

If the `havePointer` field is 0, the address of `pixels` is stored external to this bitmap data structure. The `offset` field indicates where the pixel data begins, as a number of bytes from that external pixels pointer; it is commonly used to calculate the address of pixels stored in an `ArrayBuffer`.

```c
CommodettoBitmap bitmap = ...  /* Get bitmap */;
unsigned char *buffer = xsToArrayBuffer(xsArg(0));
CommodettoPixel *pixels = (CommodettoPixel *)(bitmap.bits.offset + buffer);
```

### Host Buffers

The built-in `ArrayBuffer` object is the standard way to store and manipulate binary data in ES6 JavaScript. Commodetto supports storing the pixels of assets in an `ArrayBuffer`. However, the ES6 standard effectively requires an `ArrayBuffer` to reside in RAM. Because many embedded devices have limited RAM but relatively generous ROM (flash memory), it is desirable to use assets directly from ROM without first moving them to RAM.

To support this situation, the XS6 JavaScript engine, which Commodetto is built on, adds a `HostBuffer` object. `HostBuffer` has no constructor because it can be created only by native code. Once instantiated, the `HostBuffer` JavaScript API is identical to the `ArrayBuffer` API. Because the memory referenced by a `HostBuffer` is in read-only memory, write operations should not be performed, as the results are unpredictable; depending on the target hardware, writes may crash or fail silently.

> **Note:** The Commodetto and Poco JavaScript-to-C bindings support using a `HostBuffer` anywhere an `ArrayBuffer` is allowed; however, some JavaScript-to-C bindings do not accept a `HostBuffer` in place of an `ArrayBuffer` and so throw an exception.

## Bitmap Operations

This section describes the classes used to operate on bitmaps: `Bitmap`, `PixelsOut,` and four subclasses of `PixelsOut`.

### Bitmap Class

The Commodetto `Bitmap` object contains the three pieces of information needed to render a bitmap: the bitmap dimensions, the format of the pixels in the bitmap, and a reference to the pixel data. The following example creates a `Bitmap` object with 16-bit pixels stored in an `ArrayBuffer`.

```c
import Bitmap from "commodetto/Bitmap";

let width = 40, height = 30;
let pixels = new ArrayBuffer(height * width * 2);
let bitmap = new Bitmap(width, height, Bitmap.Raw, pixels, 0);
```

The pixels for a `Bitmap` object are often stored in a `HostBuffer`, to minimize use of RAM. For example, `File.Map` creates a `HostBuffer` corresponding to the following file:

```c
let pixels = new File.Map("/k1/pixels.dat");
let bitmap = new Bitmap(width, height, Bitmap.Raw, pixels, 0);
```

#### Functions

##### `constructor(width, height, format, buffer, offset)`

The `Bitmap` constructor takes the following arguments:

* `width` -- number indicating width of bitmap in pixels
* `height` -- number indicating height of bitmap in pixels
* `format` -- type of pixels in bitmap, taken from `Bitmap` static properties (for example, `Bitmap.Raw` or `Bitmap.RLE`)
* `buffer` -- `ArrayBuffer` or `HostBuffer` containing pixel data for bitmap
* `offset` -- number indicating offset in bytes from start of `buffer` to pixel data

All properties of a bitmap are fixed at the time it is constructed and cannot be changed after that.

#### Properties

##### `width`

Returns the width in pixels of the bitmap. This property is read-only.

##### `height`

Returns the height in pixels of the bitmap. This property is read-only.

##### `format`

Returns the pixel format of pixels of the bitmap. This property is read-only.


<!-- jph v2 -->
#### Static Properties

##### `Bitmap.Raw`

A constant corresponding to the value of `kCommodettoBitmapRaw`

##### `Bitmap.RLE`

A constant corresponding to the value of `kCommodettoBitmapPacked`

##### `Bitmap.Monochrome`

A constant corresponding to the value of `kCommodettoBitmapMonochrome`

##### `Bitmap.Gray`

A constant corresponding to the value of `kCommodettoBitmapGray16`

### PixelsOut Class

The `PixelsOut` class is an absract base class used to output pixels. It is overridden by these other classes to output to different destinations:

* `SPIOut` -- sends pixels to LCD display connected over SPI
* `BufferOut` -- sends pixels to offscreen memory buffer
* `BMPOut` -- writes pixels to BMP file
* `RLEOut` -- compresses pixels to Commodetto RLE format

A `PixelsOut` instance is initialized with a dictionary of values. The dictionary provides an arbitrary number of name-value pairs to the constructor. Different `PixelsOut` subclasses require different initialization parameters. For example:

```javascript
let display = new SPIOut({width: 320, height: 240,
		pixelFormat: "rgb565be", orientation: 90, dataPin: 30});
	
let offScreen = new BufferOut({width: 40, height: 40,
		pixelFormat: "rgb565le"});

let bmpOut = new BMPOut({width: 240, height: 240,
		pixelFormat: "rgb565le", path: "/k1/test.bmp"});
```

Developers building applications using Commodetto do not need to use the majority of the `PixelsOut` API directly. An application uses the constructor of a subclass of `PixelsOut` and then immediately binds it to a `Renderer` instance that calls the `PixelsOut` API as needed. The application interacts exclusively with the `Renderer` instance. The remaining information in this `PixelsOut` section is intended for developers implementing their own `PixelsOut` subclasses.

Once constructed, a `PixelsOut` instance can receive pixels. Three function calls are used to send pixels to the output: `begin`, `send`, and `end`. The following example shows how to output black pixels to a portion of a `PixelsOut` instance.

```javascript
let x = 10, y = 20;
let width = 40, height = 50;
display.begin(x, y, width, height);
	
let scanLine = new ArrayBuffer(display.pixelsToBytes(width));
for (let line = 0; line < height; line++)
	display.send(scanLine);
	
display.end();
```

The `begin` call indicates the area of the `PixelsOut` bitmap to update. Following that are one or more calls to `send` that contain the pixels. In the example above, `send` is called 50 times, each time with a buffer of 40 black (all 0) pixels. The number of bytes of data passed by the combined calls to `send` must be exactly the number of bytes needed to fill the area specified in the call to `begin`. Once all data has been provided, call `end`. 

The `PixelsOut` API does not define when the data is transmitted to the output. Different `PixelsOut` subclasses implement it differently: some transmit the data immediately and synchronously, some use asynchronous transfers, and others buffer the data to display only when `end` is called.

The following example shows the definition of the `PixelsOut` class (with the function bodies omitted).

```javascript
class PixelsOut {
	constructor(dictionary) {}
	begin(x, y, width, height) {}
	end() {}
	continue(x, y, width, height) {}

	send(pixels, offset = 0, count = pixels.byteLength - offset) {}

	get width() {}
	get height() {}

	get pixelFormat() {}
	pixelsToBytes(count) {}
}
```

#### Functions

##### `constructor(dictionary)`

The `PixelsOut` constructor takes a single argument: a dictionary of property names and values. The `PixelsOut` base class defines three properties that are defined for all subclasses of `PixelsOut`: 

<!-- jph v2 -->
* `width` -- number specifying width of output in pixels
* `height` -- number specifying height of output in pixels
* `pixelFormat` -- string specifying format of output pixels (for example, `"rgb565le"` for 16-bit 565 little-endian pixels and `"rgb565be"` for 16-bit 565 big-endian pixels)

Subclasses define additional properties in their dictionary as needed.

```javascript
import PixelsOut from "commodetto/PixelsOut";

let out = new PixelsOut({width: 120, height: 160, pixelFormat: "rgb565be"});
```

> **Note:** The `width`, `height`, and `pixelFormat` properties of a `PixelsOut` object are fixed at the time the object is created and cannot be changed. In general, all properties included in the dictionary provided to the constructor should be considered read-only to keep the implementation of the `PixelsOut` subclasses simple. However, subclasses of `PixelsOut` may choose to provide ways to modify some properties.

##### `begin(x, y, width, height)`

The `begin` function starts the delivery of a frame to the output. The `x` and `y` arguments indicate the top-left corner of the frame; the `width` and `height` arguments, the size of the frame. The area contained by the arguments to the `begin` function must fit entirely within the dimensions provided to the constructor in its dictionary.

##### `end()`

The `end` function indicates that delivery of the current frame to the output is complete. Calling `end` is required for proper output of the frame.

<a id="PixelsOut-continue"></a>
##### `continue(x, y, width, height)`

The `continue` function is a way to draw to more than one area of a single frame. In concept, calling `continue` is equivalent to calling `end` following by `begin`. That is, the following

```javascript
out.continue(10, 20, 30, 40);
```

is almost the same as this:

```javascript
out.end();
out.begin(10, 20, 30, 40);
```
	
The difference is that when `continue` is used, all output pixels are part of the same frame, whereas with `end`/`begin` pixels transmitted before `end` are part of one frame and pixels transmitted after it are part of the following frame.

For some outputs--for example, `BufferOut`--there is no difference. For others--for example, a hardware-accelerated renderer--the results are visually different.

> **Note:** Not all `PixelsOut` subclasses implement `continue`. If it is not supported, an exception is thrown. The subclass descriptions that follow indicate if `continue` is supported.

##### `send(pixels, offset = 0, count = pixels.byteLength - offset)`

The `send` function transmits pixels to the output. The `pixels` argument is an `ArrayBuffer` or `HostBuffer` containing the pixels. The `offset` argument is the offset (in bytes) at which the pixels begin in the buffer, and `count` is the number of bytes to transmit from the buffer.

```javascript
let pixels = new ArrayBuffer(40);
out.send(pixels);		// Send all pixels in buffer
out.send(pixels, 30);	// Send last 5 pixels in buffer
out.send(pixels, 2, 2)	// Send 2nd pixel in buffer
```

>**Note:** The `offset` and `count` arguments are in units of bytes, not pixels.

##### `pixelsToBytes(count)`

The `pixelsToBytes` function calculates the number of bytes required to store the number of pixels specified by the `count` argument. The following allocates an `ArrayBuffer` corresponding to a single scanline of pixels: 

```javascript
let buffer = new ArrayBuffer(out.pixelsToBytes(width));
```

#### Properties

##### `width`

Returns the width of the `PixelsOut` instance in pixels. This property is read-only.

##### `height`

Returns the height of the `PixelsOut` instance in pixels. This property is read-only.

##### `pixelFormat`

Returns the format of the pixels of the `PixelsOut` instance. This property is read-only.

### SPIOut Class

The `SPIOut` subclass of `PixelsOut` is a placeholder for an implementation of a `PixelsOut` instance that sends pixels to a display connected over SPI.

```javascript
class SPIOut extends PixelsOut;
```

### BufferOut Class

<!-- jph v2 -->
`BufferOut` is a subclass of `PixelsOut` that implements an offscreen in-memory bitmap. It supports `"rgb565le"` and `"rgb565be"` 16-bit pixels, `"g4"` 4-bit pixels, and `"m1"` 1-bit pixels.

```javascript
class BufferOut extends PixelsOut;
```

Because memory tends to be a scarce resource on target devices, applications should use `BufferOut` sparingly.

To create a `BufferOut` instance:

```javascript
import BufferOut from "commodetto/BufferOut";

let offscreen = new BufferOut({width: 40, height: 30, pixelFormat: "rgb565le"});
```

The `BufferOut` constructor allocates the buffer for the bitmap pixels. Once the offscreen bitmap has been created, pixels are delivered to it using the `send` function, as described for the `PixelsOut` class. The pixels in the `BufferOut` instance are accessed through the instance's bitmap.

```javascript
let bitmap = offscreen.bitmap;
```

`BufferOut` implements the optional `continue` function of `PixelsOut`.

#### Properties

##### `bitmap`

Returns a `Bitmap` instance to access the pixels of the `BufferOut` instance. This property is read-only.

### BMPOut Class

`BMPOut` is a subclass of `PixelsOut` that receives pixels and writes them in a BMP file with 16-bit color `"rgb565le"` pixels or 4-bit gray `"g4"` pixels. The `BMPOut` implementation writes pixels to the file incrementally, so it can create files larger than available free memory.

`BMPOut` adds the `path` property to the constructor's dictionary; `path` is a string containing the full path to the output BMP file.

> **Note:** The width of the BMP file must be a multiple of 2 for 16-bit pixels and a multiple of 8 for 4-bit pixels.

### RLEOut Class

`RLEOut` is a subclass of `PixelsOut` that receives pixels and packs them into an RLE-encoded bitmap.

```javascript
class RLEOut extends PixelsOut;
```

There are several benefits to storing a bitmap in the RLE format:

* For many common bitmap uses, RLE-encoded bitmaps draw more quickly.
* RLE-encoded bitmaps are smaller.
* RLE-encoded bitmaps can include a 1-bit mask to draw nonrectangular images, including images with holes.

`RLEOut` extends the constructor's dictionary with the optional `key` property, which contains a pixel value to use as a key color when encoding the image. All pixels transmitted to `RLEOut` that match the value of the key color pixel are excluded from the RLE-encoded image, making them invisible. This key color capability is effectively a 1-bit mask.

The `RLEOut` class may be used on the device. More commonly, however, a bitmap is RLE-encoded and written to storage at build time; this avoids the time and memory required to encode the image on the device, while saving storage space.

#### Static Functions

##### `encode(source, key)`

The `encode` function converts a raw bitmap, specified by the `source` argument, into an RLE-encoded bitmap. The optional `key` argument is the pixel value to use as the key color for generating the 1-bit mask.

```javascript
import RLEOut from "commodetto/RLEOut";

let uncompressed = ...  // bitmap of format Bitmap.Raw
let compressed = RLEOut.encode(uncompressed);
let compressedWithBlackKey = RLEOut.encode(uncompressed, 0);
```

#### Properties

##### `bitmap`

Returns a `Bitmap` instance to access the pixels of the `RLEOut` instance. This property is read-only.

## Asset Parsing

Building a user interface for a display requires visual assets--icons, bitmaps, photos, fonts, and so on. There are many commonly used file formats for storing these assets, some of which work well on constrained devices. Commodetto includes functions to use several common asset file formats directly; however, Commodetto does not support all features of these file formats. Graphic designers creating assets for use with Commodetto need to be aware of the asset format requirements.

> **Note:** For optimal render performance and minimum storage size, it is beneficial to convert assets to the preferred format of the target device at build time. Tools to perform the conversion are out of scope of this release of Commodetto.

### BMP

The [BMP](https://msdn.microsoft.com/en-us/library/dd183391.aspx) file format was created by Microsoft for use on Windows. It is a flexible container for uncompressed pixels of various formats. The format is unambiguously documented and well supported by graphic tools. BMP is the preferred format for uncompressed bitmaps in Commodetto. The Commodetto BMP file parser supports the following variants of BMP:

* **16-bit 565 little-endian pixels** -- The image width must be a multiple of 2. When using Photoshop to save the BMP file, select **Advanced Mode** in the BMP dialog and check **R5 G6 B5**.
* **16-bit 555 little-endian pixels** -- The image width must be a multiple of 2. This format is the most common 16-bit format for BMP files; however, it is not recommended, as the pixels must be converted to 565, which takes time and requires memory to store the converted pixels.
* **4-bit gray** -- The image width must be a multiple of 8 and must have a gray palette. When working in Photoshop, this means setting **Image Mode** to **Grayscale** before saving the image in BMP format.
* **1-bit black and white** -- The image width must be a multiple of 32.

By default, BMP files are stored with the bottom line of the bitmap first in the file--for example, bottom-to-top order. Commodetto requires bitmaps to be stored top to bottom. When saving a BMP file, select the option to store it in top-to-bottom order. In Photoshop, check **Flip row order** to store the BMP in top-to-bottom order.

The `parseBMP` function creates a bitmap from an `ArrayBuffer` or `HostBuffer` containing BMP data. It performs validation to confirm that the file format is supported, and throws an exception if it detects an unsupported variant of BMP.

```javascript
import parseBMP from "commodetto/parseBMP";

let bmpData = new File.Map("/k1/image.bmp");
let bitmap = parseBMP(bmpData)
console.log(`Bitmap width ${bitmap.width}, height ${bitmap.height}\n`);
```

### JPEG

The JPEG file format is the most common format for storing photos. Many resource-constrained devices have the performance to decompress JPEG images, though not all have the memory to store the result. Commodetto provides a way to render a JPEG image to an output, even if the decompressed JPEG image cannot fit into memory.

The JPEG decoder used in Commodetto supports a subset of the JPEG specification. YUV encoding with H1V1 is supported, with plans to support H2V2 in a future release. Grayscale JPEG images are supported, and there are no restrictions on the width and height of the JPEG image. Progressive JPEG images are not supported.

To decompress JPEG data to an offscreen bitmap, use the static `decompress` function.

```javascript
import JPEG from "commodetto/readJPEG";

let jpegData = File.Map("/k1/image.jpg");
let bitmap = JPEG.decompress(jpegData);
```

The JPEG decoder also implements a block-based decode mode to return a single block of decompressed data at a time.

```javascript
import JPEG from "commodetto/readJPEG";

let jpegData = File.Map("/k1/image.jpg");
let decoder = new JPEG(jpegData);
	
while (true) {
	let block = decoder.read();
	if (!block) break;		// all blocks decoded

	console.log(`block x: ${block.x}, y: ${block.y},
			width: ${block.width}, height: ${block.height}\n`);
}
```

Each block returned is a bitmap. The `width` and `height` fields of the bitmap indicate the dimensions of the block. The width and height can change from block to block. The `x` and `y` properties indicate the placement of the block relative to the top-left corner of the full JPEG image. Blocks are returned in a left-to-right, top-to-bottom order.

The same bitmap object is used for all blocks, so the contents of the block change after each call to `read`. This means an application cannot collect all the blocks into an array for later rendering. To do that, the application needs to make a copy of the data from each block.

Using a renderer, it is straightforward to incrementally send a JPEG image to a display block-by-block as it is decoded, eliminating the need to copy the data of each block. The Poco renderer documentation includes an example of this technique.

> **Note:** Commodetto uses the excellent public domain [picojpeg](https://code.google.com/archive/p/picojpeg/) decoder, which is optimized to minimize memory use. Some quality and performance are sacrificed, but the results are generally quite good. Small changes have been made to picojpeg to eliminate compiler warnings; those changes are part of the Commodetto source code distribution.

### PNG

The PNG image format is commonly used for the assets of user interface elements such as buttons and sliders. Because the PNG file format is heavily compressed, PNG images must be decompressed to a `BufferOut` instance for use. Also because of the compression used in PNG, a significant amount of memory is required for decompressing the image. Nonetheless, because PNG is so common in user interface work, Commodetto implements a PNG module for use on devices and scenarios where it is practical.

The PNG decoder in Commodetto supports most variations of the PNG file format, with two exceptions: 

- Interlaced images are not supported, as interlacing is incompatible with progressive decoding.

- Images with 16-bit channels are unsupported because the high resolution exceeds the image quality capabilities of target devices.

To decompress PNG data to an offscreen bitmap, use the static `decompress` function.

```javascript
import PNG from "commodetto/readPNG";

let pngData = File.Map("/k1/image.png");
let bitmap = PNG.decompress(pngData);
```

The PNG `decompress` function supports the following PNG variants:

- 3 channels, 8 bits per channel (24-bit RGB)
- 4 channels, 8 bits per channel (32-bit RGBA)
- 1 channel, 8 bits per channel (8-bit gray)
- 1 channel, 8 bits per channel with palette (8-bit indexed RGB)
- 1 channel, 1 bit per channel (1-bit monochrome)

For images that contain an alpha channel, pass `true` for the optional second argument to `decompress`. The alpha channel is returned as a 4-bit gray bitmap in the `mask` property of the returned bitmap image.

```javascript
let pngData = File.Map("/k1/image_with_alpha.png");
let bitmap = PNG.decompress(pngData, true);
let alpha = bitmap.mask;
```

The PNG decoder also implements a progressive decoding mode to return a single scanline of decompressed data at a time. The scanline is raw data from the PNG decoder, with no pixel transformations applied. The following example converts a 24-bit (RGB) or 32-bit (RGBA) PNG image to a 16-bit BMP.

	let png = new PNG(new File.Map("/k1/image.png"));
	let width = png.width, height = png.height;
	trace(`width ${width}, height ${height}, channels ${png.channels}, depth ${png.depth}, bpp ${png.channels * png.depth}\n`);

	if ((8 != png.depth) || ((3 != png.channels) && (4 != png.channels)))
		throw new Error("unsupported PNG variant");

	let bmp = new BMPOut({width: width, height: height, pixelFormat: "rgb565le", path: "/k1/image.bmp"});
	bmp.begin(0, 0, width, height);

	let scan16 = new Uint16Array(width);
	for (let i = 0; i < height; i++) {
		let scanLine = png.read();
		for (let j = 0, offset = 0; j < width; j++, offset += png.channels) {
			let r = scanLine[offset    ];
			let g = scanLine[offset + 1];
			let b = scanLine[offset + 2];
			scan16[j] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
		}
		bmp.send(scan16.buffer);
	}
	bmp.end();

In addition to the `width` and `height` properties, the PNG instance contains `depth` and `channels` properties based on the content of the PNG image.

The PNG decoder uses approximately 45 KB of memory while decoding an image. This amount of memory may not be available or practical on all target devices. The memory requirement is primarily due to the zlib compression algorithm used in PNG images.

> **Note:** Commodetto uses the public domain [miniz](https://github.com/richgel999/miniz/) library to decompress the [zlib](http://www.zlib.net/manual.html) data contained in PNG images. The PNG parsing is partially based on the Apache-licensed [`FskPNGDecodeExtension.c`](https://github.com/Kinoma/kinomajs/blob/master/extensions/FskPNGDecode/sources/FskPNGDecodeExtension.c) from KinomaJS, with significant simplifications.

### NFNT

[NFNT](http://mirror.informatimago.com/next/developer.apple.com/documentation/mac/Text/Text-250.html#MARKER-9-414) is the bitmap font format used in the original Macintosh computer. (It is a small update to the Macintosh's original FONT resource.) NFNT fonts are 1-bit glyphs with compact metrics tables, making them easy to fit into the most constrained of devices. An NFNT font may be as small as 3 KB. The design of many NFNT fonts is beautiful, with carefully hand-tuned pixels.

Using NFNT in Commodetto requires that the NFNT resource be extracted to a file. Several tools do this extraction, including [rezycle](https://itunes.apple.com/us/app/rezycle/id485082834?mt=12), which is available in the Mac App Store.

The `parseNFNT` function prepares an NFNT resource for measuring and drawing text with a `Render` object.

```javascript
import parseNFNT from "commodetto/parseNFNT";

let chicagoResource = File.Map("/k1/chicago_12.nfnt");
let chicagoFont = parseNFNT(chicagoResource);
```

The font object returned by `parseNFNT` contains the information required by a `Render` object, including the monochrome bitmap containing the glyphs. The glyph bitmap may be accessed directly, as follows:

```javascript
let chicagoBitmap = chicagoFont.bitmap;
```

### BMFont

[BMFont](http://www.angelcode.com/products/bmfont/) is a format for storing bitmap fonts. It is widely used to embed distinctive fonts in games in a format that is efficiently rendered using OpenGL. BMFont is well designed and straightforward to support. Commodetto uses BMFont to store both anti-aliased and multicolor fonts, capabilities unavailable using NFNT. In addition, BMFont has good tool support--in particular the excellent [Glyph Designer](https://71squared.com/glyphdesigner), which converts macOS fonts to a Commodetto-compatible BMFont.

BMFont stores a font's metrics data separately from the font's glyph atlas (bitmap). This means that loading a BMFont requires two steps: loading the metrics and loading the glyph atlas. BMFont allows the metrics data to be stored in a variety of formats, including text, XML, and binary. Commodetto supports the binary format for metrics.

The `parseBMF` function prepares the BMFont binary metrics file for use with a `Render` object.

```javascript
import parseBMF from "commodetto/parseBMF";
import parseBMP from "commodetto/parseBMP";

let palatino36 = parseBMF(File.Map("/k1/palatino36.fnt"));
palatino36.bitmap = parseBMP(File.Map("/k1/palatino36.bmp");
```

After the metrics are prepared with `parseBMF`, the glyph atlas is prepared using `parseBMP` and is attached to the metrics as the `bitmap` property.

The `BMFont` format allows multiple discontiguous ranges of characters, but `parseBMF` requires that the characters be a single contiguous range.

<!-- jph v2 -->
For anti-aliased text, the BMP file containing the glyph atlas bitmap must be in 4-bit gray format. For multicolor text, the bitmap must be in raw (16-bit) format.

## Rendering

Commodetto is designed to support multiple rendering engines. The initial engine is Poco, a small bitmap-based scanline renderer. A renderer knows how to draw pixels and relies on a `PixelsOut` instance to output those pixels, whether to a display, an offscreen buffer, or a file.

When a renderer is created, it is bound to an output. For example, to render to a BMP file:

```javascript
import BMPOut from "commodetto/BMPOut";
import Poco from "commodetto/Poco";

let bmpOut = new BMPOut({width: decoded.width, height: decoded.height,
		pixelFormat: "rgb565le", path: "/k1/allegra64.bmp"});
let render = new Poco(bmpOut);
```

To render to a display, use `SPIOut` in place of `BMPOut`, as follows:

```javascript
import SPIOut from "commodetto/SPIOut";
import Poco from "commodetto/Poco";

let display = new SPIOut({width: 320, height: 240,
		pixelFormat: "rgb565be", orientation: 90, dataPin: 30});
let render = new Poco(display);
```

The Poco renderer documentation describes its rendering operations with examples of common uses.

### Render Class

The `Render` class is a abstract base class used to generate pixels. It is overridden by specific rendering engines, such as Poco. The `Render` class has only four functions, which manage the rendering process but do no rendering themselves. The specific rendering operations available are defined by subclasses of `Render`.

The following example shows using the Poco renderer with `SPIOut` to render a screen consisting of a white background with a 10-pixel red square at location `{x: 5, y: 5}`.

```javascript
let display = new SPIOut({width: 320, height: 240,
		pixelFormat: "rgb565le", orientation: 90, dataPin: 30});
let render = new Poco(display);
	
let white = poco.makeColor(255, 255, 255);
let red = poco.makeColor(255, 0, 0);
	
render.begin();
render.fillRectangle(white, 0, 0, display.width, display.height);
render.fillRectangle(red, 5, 5, 10, 10);
render.end();
```

#### Functions

##### `constructor(pixelsOut, dictionary)`

The `Render` constructor takes two arguments: the `PixelsOut` instance the `Render` object sends pixels to for output, and a dictionary to configure rendering options. All dictionary properties are defined by the subclasses of `Render`.

##### `begin(x, y, width, height)`

The `begin` function starts the rendering of a frame. The area to be updated is specified with the `x` and `y` coordinates and `width` and `height` dimensions. The area must be fully contained within the bounds of the `PixelsOut` instance bound to the renderer.

```javascript
render.begin(x, y, width, height);
```

All drawing is clipped to the updated area defined by `begin`.

Calling `begin` with no arguments is equivalent to calling it with `x` and `y` equal to 0 and `width` and `height` equal to the `width` and `height` values of the `PixelsOut` instance. That is, the following

```javascript
render.begin()
```
	
is equivalent to this:

```javascript
render.begin(0, 0, pixelsOut.width, pixelsOut.height);
```

If `width` and `height` are omitted, the update area is the rectangle defined by the `x` and `y` coordinates passed to `begin`. The following bottom-right corner of the `pixelsOut` bounds

```javascript
render.begin(x, y);
```

is equivalent to this:

```javascript
render.begin(x, y, pixelsOut.width - x, pixelsOut.height - y);
```

##### `end()`

The `end` function completes the rendering of a frame. All pending rendering operations are completed by this function.

> **Note:** For a display list renderer, such as Poco, all rendering occurs during the execution of `end`.

##### `continue(x, y, width, height)`

The `continue` function is used when there are multiple update areas in a single frame. The arguments to `continue` behave in the same manner as the arguments to `begin`. The `continue` function is nearly equivalent to the following sequence:

```javascript
render.end();
render.begin(x, y, width, height);
```

The difference is for displays with a buffer--for example, when Commodetto is running on a display with page flipping hardware or on a computer simulator. In such cases the output frame is displayed only when `end` is called. With `continue`, intermediate results remain hidden offscreen until the full frame is rendered.

The following fragment shows three separate update areas for a single frame.

```javascript
render.begin(10, 10, 20, 20);
// Draw
...
render.continue(200, 100, 40, 40);
// Draw more
...
render.continue(300, 0, 20, 20);
// Draw even more
...
render.end();
```

## Odds and Ends

### Target Hardware

The primary design constraint for Commodetto is to render a modern user interface on a resource-constrained microcontroller. The target devices have the following broad characteristics:

- ARM Cortex M3/M4 CPU
- Single core
- 80 to 200 MHZ CPU speed
- 128 to 512 KB of RAM
- 1 to 4 MB of flash storage
- No graphics rendering hardware

If a more capable processor is available, Commodetto gets better. More memory allows for more offscreen bitmaps and more complex scenes. More CPU speed allows for greater use of computationally demanding graphics operations. More flash storage allows for more assets to be stored. Hardware graphics acceleration allows for faster rendering of more complex screens.

Commodetto runs on any target hardware that supports the XS6 JavaScript engine. Commodetto is written in ANSI C, with only a handful of calls to external functions (`memcpy`, `malloc`, and `free`). The core Poco renderer allocates no memory.

### XS6

The JavaScript API in Commodetto is implemented for use with the [XS6](https://github.com/Kinoma/kinomajs/tree/master/xs6) JavaScript engine. XS6 is optimized for use on embedded platforms while maintaining [compatibility](http://kangax.github.io/compat-table/es6/) with the full ES6 JavaScript language.

### License

Commodetto is provided under the [Mozilla Public License 2.0](https://www.mozilla.org/en-US/MPL/2.0/) (MPL). The MPL is a copyleft license, which guarantees that users of products that incorporate Commodetto have access to the version of the Commodetto source code used in that product. The MPL is an [OSI-approved](https://opensource.org/licenses) open source license compatible with major open source licenses, including the GPL and Apache 2.0.

The MPL grants broad rights to developers incorporating Commodetto into their products. In exchange for those rights, developers have the obligation to acknowledge their use of Commodetto and to share the source code of any changes they make to Commodetto, as well other obligations described in the MPL.

### About the Name "Commodetto"

The word *commodetto* is a term used in music meaning "leisurely." The use of the name Commodetto here is taken from a set of piano variations by Beethoven, specifically the third variation of WoO 66. A sample of the variation is [available for listening](http://www.prestoclassical.co.uk/r/Warner%2BClassics/5857612). The feeling of the variation is light and leisurely, though there is nothing simple or trivial about the composition or the performance.
