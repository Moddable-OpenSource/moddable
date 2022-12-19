# Commodetto
Copyright 2016-2021 Moddable Tech, Inc.<BR>
Revised: October 26, 2021

## About This Document

Commodetto is a graphics library designed to bring modern user interface rendering to devices powered by a resource-constrained microcontroller. For many applications, Commodetto needs only a few kilobytes of RAM, including the assets for rendering the user interface.

This document provides a high-level overview of the parts of Commodetto, information about asset format requirements for assets included in Commodetto applications, and details on the objects that define the Commodetto JavaScript API.

## Table of Contents

* [Overview of Commodetto](#overview)
	* [Native Data Types](#native-data-types)
	* [Host Buffers](#host-buffers)
* [Bitmap Operations](#bitmap-operations)
	* [Bitmap Class](#bitmap-class)
	* [PixelsOut Class](#pixelsout-class)
	* [SPIOut Class](#"spiout-class)
	* [BufferOut Class](#bufferout-class)
	* [BMPOut Class](#bmpout-class)
	* [RLE4Out Class](#rle4out-class)
	* [ColorCellOut Class](#colorcellout-class)
* [Asset Parsing](#asset-parsing)
	* [BMP](#bmp)
	* [JPEG](#jpeg)
	* [PNG](#png)
	* [BMFont](#bmfont)
* [Rendering](#rendering)	
	* [Render class](#render-class)
* [Pixel format conversion](#pixel-format-conversion)
	* [Convert Class](#convert-class)
* [Odds and Ends](#odds-and-ends)

<a id="overview"></a>
## Overview of Commodetto

Commodetto consists of several parts:

* A lightweight [Poco rendering engine](./poco.md)--a display list renderer able to efficiently render a single scanline at a time, eliminating the need for a frame buffer
* Asset loaders for working with common file formats
* Pixel outputs for delivering rendered pixels to displays and files
* A JavaScript API for all features

Every part of Commodetto is a module, making it straightforward to deploy only necessary modules, add new modules, and replace existing modules. Even the rendering capabilities are a module, enabling integration of specialized rendering modules.

The **Poco rendering engine** contains only the most essential rendering operations: fill or blend a rectangle with a solid color, plus a small set of bitmap drawing operations, including copy, pattern fill, and alpha blend. Text is implemented externally to the rendering engine using Poco bitmap rendering operations, which enables integration of different engines for glyph generation, text layout, and text measurement.

**Asset loaders** prepare graphical assets, such as photos, user interface elements, and fonts, for rendering. Commodetto includes asset loaders for BMP images, JPEG photos, PNG images, BMFont fonts. Additional asset loader modules may be added. The asset loaders enable many types of assets to be rendered directly from flash storage (for example, ROM) without having to be loaded into RAM.

**Pixel outputs** deliver rendered pixels to their destination. Commodetto includes modules to write to files and in-memory bitmaps. Modules may be added to send the pixels to a display over the transports supported by the host device, including SPI, I<sup>2</sup>C, serial, and memory-mapped ports.

<a id="native-data-types"></a>
### Native Data Types

Commodetto data types include pixel, coordinate, dimension, and bitmap. These types are described below along with the enumeration that defines pixel formats.

#### Pixel Type

To run well on resource-constrained hardware, Commodetto makes simplifying assumptions. One assumption is that only a single output pixel format is supported in a given deployment. A general-purpose graphics library needs to support many different output pixel formats to be compatible with the many different displays and file formats in use. Commodetto assumes the device it is deployed to connects to a single type of screen; this assumption reduces the code size and some runtime overhead.

Commodetto has the ability to support different pixel formats, with the choice of the pixel format to use set at compile time. Changing the output pixel format--for example, to use a different screen--simply requires recompiling Commodetto with a different output pixel format configured.

The pixel format is determined at build time by the value of the C #define `kCommodettoBitmapFormat`.  The following pixel format specifiers are supported for the rendering destination: `kCommodettoBitmapRGB565LE`, `kCommodettoBitmapRGB332`, `kCommodettoBitmapGray256`, `kCommodettoBitmapGray16`, and `kCommodettoBitmapCLUT16`.

`CommodettoPixel` is the data type for pixel. The definition of the type depends on the value of `kCommodettoBitmapFormat`. For 16-bit pixels, it is `uint16_t`, for 8-bit and 4-bit pixels it is `uint8_t`.

```c
typedef uint16_t CommodettoPixel;
```

***

#### Pixel Formats
Commodetto supports multiple source pixel formats at the same time. This allows for efficient storage and rendering of different kinds of assets.

The output pixel format is always one of the supported source pixel formats. In addition, source pixel formats of 1-bit monochrome and 4-bit gray are always supported.

The `CommodettoBitmapFormat` enumeration defines the pixel formats. A subset of pixel formats are supported in each deployment.

```c
typedef enum {
	kCommodettoBitmapDefault = 1,
	kCommodettoBitmapMonochrome = 3,
	kCommodettoBitmapGray16,
	kCommodettoBitmapGray256,
	kCommodettoBitmapRGB332,
	kCommodettoBitmapRGB565LE,
	kCommodettoBitmapRGB565BE,
	kCommodettoBitmap24RGB,
	kCommodettoBitmap32RGBA,
	kCommodettoBitmapCLUT16,

	kCommodettoBitmapPacked = 0x80
} CommodettoBitmapFormat;
```

| Format | Description |
| :---: | :--- |
| kCommodettoBitmapDefault | The output pixel format. The format varies depending on the value of `kCommodettoBitmapFormat` when Commodetto is built.
|  kCommodettoBitmapMonochrome | Pixels are 1-bit data, packed into bytes in which the high bit of the byte is the leftmost pixel.
| kCommodettoBitmapGray16 | Pixels are 4-bit data, where 0 represents white, 15 represents black, and the values in between are proportionally interpolated gray levels. The pixels are packed into bytes in which the high nybble is the leftmost pixel.
| kCommodettoBitmapGray16 \| kCommodettoBitmapPacked | Pixels are the same as in `kCommodettoBitmapGray16`, and compressed using a weighted RLE algorithm.
| kCommodettoBitmapGray256 | Pixels are 8-bit data, where 0 represents white, 255 represents black, and the values in between are proportionally interpolated gray levels. 
| kCommodettoBitmapRGB332 | Pixels are 8-bit data, with packed RGB values. The high three bits are red, followed by three bits of green, and two bits of blue.
| kCommodettoBitmapRGB565LE | Pixels are 16-bit data, with packed RGB values. The high five are red, followed by six bits of green, and five bits of blue. The 16-bit value is stored in little-endian byte order.
| kCommodettoBitmapRGB565BE | Pixels are 16-bit data, with packed RGB values. The high five are red, followed by six bits of green, and five bits of blue. The 16-bit value is stored in big-endian byte order.
| kCommodettoBitmap24RGB | Pixels are three 8-bit values, in the order of red, green, blue.
| kCommodettoBitmap24RGBA | Pixels are four 8-bit values, in the order of red, green, blue, alpha.

***

#### Coordinate Type

The `CommodettoCoordinate` type specifies coordinates.

```c
typedef int16_t CommodettoCoordinate;
```

The coordinate type is signed to enable applications to render objects that overlap the screen on any edge.

For devices with very small displays (128 pixels or fewer in each dimension), `CommodettoCoordinate` may be redefined as an 8-bit value--for example, `int8_t`--to reduce memory requirements. When doing this, application code must take care not to use coordinates smaller than -128 or larger than 127, or the results will be unpredictable.

***

#### Dimension Type

`CommodettoDimension` is the type used to specify dimensions.

```c
typedef uint16_t CommodettoDimension;
```

The dimension type is unsigned, as Commodetto assigns no meaning to a negative width or height.

`CommodettoDimension` may be redefined as `uint8_t` on devices with very small displays (256 pixels in each dimension) to reduce memory use. When doing this, application code must take care not to specify dimensions larger than 255 pixels.

***

#### Bitmap Type

The `CommodettoBitmap` structure defines a bitmap.

```c
typedef struct {
	CommodettoDimension	w;
	CommodettoDimension	h;
	CommodettoBitmapFormat	format;
	int8_t			havePointer;
	union {
		void		*data;
		int32_t		offset;
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

***

<a id="host-buffers"></a>
### Host Buffers

The built-in `ArrayBuffer` object is the standard way to store and manipulate binary data in JavaScript. Commodetto supports storing the pixels of assets in an `ArrayBuffer`. However, the JavaScript standard effectively requires an `ArrayBuffer` to reside in RAM. Because many embedded devices have limited RAM but relatively generous ROM (flash memory), it is desirable to use assets directly from ROM without first moving them to RAM.

To support this situation, the XS JavaScript engine, which Commodetto is built on, adds a `HostBuffer` object. `HostBuffer` has no constructor because it can be created only by native code. Once instantiated, the `HostBuffer` JavaScript API is identical to the `ArrayBuffer` API. Because the memory referenced by a `HostBuffer` is in read-only memory, write operations should not be performed, as the results are unpredictable; depending on the target hardware, writes may crash or fail silently.

In the Moddable SDK, the `Resource` constructor is a way to create a `HostBuffer` for a block of data in ROM.

> **Note:** The Commodetto and Poco JavaScript-to-C bindings support using a `HostBuffer` anywhere an `ArrayBuffer` is allowed; however, some JavaScript-to-C bindings do not accept a `HostBuffer` in place of an `ArrayBuffer` and so throw an exception.

***

<a id="bitmap-operations"></a>
## Bitmap Operations

This section describes the classes used to operate on bitmaps: `Bitmap`, `PixelsOut`, and four subclasses of `PixelsOut`.

<a id="bitmap-class"></a>
### Bitmap Class

The Commodetto `Bitmap` object contains the three pieces of information needed to render a bitmap: the bitmap dimensions, the format of the pixels in the bitmap, and a reference to the pixel data. The following example creates a `Bitmap` object with 16-bit pixels stored in an `ArrayBuffer`.

```js
import Bitmap from "commodetto/Bitmap";

let width = 40, height = 30;
let pixels = new ArrayBuffer(height * width * 2);
let bitmap = new Bitmap(width, height, Bitmap.RGB565LE, pixels, 0);
```

The pixels for a `Bitmap` object are often stored in a `HostBuffer`, to minimize use of RAM. For example, `Resource` creates a `HostBuffer` corresponding to the following resource:

```js
let pixels = new Resource("pixels.dat");
let bitmap = new Bitmap(width, height, Bitmap.RGB565LE, pixels, 0);
```

#### Functions

##### `constructor(width, height, format, buffer, offset)`

The `Bitmap` constructor takes the following arguments:

| Argument | Description |
| :---: | :--- |
| `width` | number indicating width of bitmap in pixels
| `height` | number indicating height of bitmap in pixels
| `format` | type of pixels in bitmap, taken from `Bitmap` static properties (for example, `Bitmap.RGB565LE` or `Bitmap.Gray16`)
| `buffer` | `ArrayBuffer` or `HostBuffer` containing pixel data for bitmap
| `offset` | number indicating offset in bytes from start of `buffer` to pixel data

All properties of a bitmap are fixed at the time it is constructed and cannot be changed after that.

***

#### Properties

| Name | Type | Read-only | Description |
| :---: | :---: | :---: | :--- |
| `width` | `Number` | ✓ | The width in pixels of the bitmap.
| `height` | `Number` | ✓ | The height in pixels of the bitmap.
| `format` | `Number` | ✓ | Returns the pixel format of pixels of the bitmap.

***

#### Static Functions

#### `depth(pixelFormat)`

Returns the number of bits per pixel for the specified pixel format.

```js
Bitmap.depth(Bitmap.RGB565LE);		// returns 16
Bitmap.depth(Bitmap.Monochrome);	// returns 1
Bitmap.depth(Bitmap.Gray256);		// returns 8
```

***

#### Static Properties

| Name | Description |
| :---: | :--- |
| `Bitmap.Default` | A constant corresponding to the value of `kCommodettoBitmapDefault`
| `Bitmap.RLE` |A constant corresponding to the value of `kCommodettoBitmapPacked`
| `Bitmap.Monochrome` | A constant corresponding to the value of `kCommodettoBitmapMonochrome`
| `Bitmap.Gray16` | A constant corresponding to the value of `kCommodettoBitmapGray16`
| `Bitmap.Gray256` | A constant corresponding to the value of `kCommodettoBitmapGray256`
| `Bitmap.RGB332` | A constant corresponding to the value of `kCommodettoBitmapRGB332`
| `Bitmap.RGB565LE` | A constant corresponding to the value of `kCommodettoBitmapRGB565LE`
| `Bitmap.RGB565BE` | A constant corresponding to the value of `kCommodettoBitmapRGB565BE`
| `Bitmap.RGB24` | A constant corresponding to the value of `kCommodettoBitmapRGB24`
| `Bitmap.RGBA32` | A constant corresponding to the value of `kCommodettoBitmapRGBA32`
| `Bitmap.CLUT16` | A constant corresponding to the value of `kCommodettoBitmapCLUT16`

***

<a id="pixelsout-class"></a>
### PixelsOut Class

The `PixelsOut` class is an abstract base class used to output pixels. It is overridden by these other classes to output to different destinations:

* `SPIOut` -- sends pixels to LCD display connected over SPI
* `BufferOut` -- sends pixels to offscreen memory buffer
* `BMPOut` -- writes pixels to BMP file
* `RLE4Out` -- compresses pixels to Commodetto 4-bit gray RLE format
* `ColorCellOut` -- compresses pixels to Moddable variation of ColorCell

A `PixelsOut` instance is initialized with a dictionary of values. The dictionary provides an arbitrary number of name-value pairs to the constructor. Different `PixelsOut` subclasses require different initialization parameters. For example:

```javascript
let display = new SPIOut({width: 320, height: 240,
		pixelFormat: Bitmap.RGB565BE, orientation: 90, dataPin: 30});
	
let offScreen = new BufferOut({width: 40, height: 40,
		pixelFormat: Bitmap.RGB565LE});

let bmpOut = new BMPOut({width: 240, height: 240,
		pixelFormat: Bitmap.RGB565LE, path: "/test.bmp"});
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

| Name | Description |
| :---: | :--- |
| `width` | number specifying width of output in pixels
| `height` | number specifying height of output in pixels
| `pixelFormat` | string specifying format of output pixels (for example, `Bitmap.RGB565BE` for 16-bit 565 little-endian pixels and `Bitmap.RGB565BE` for 16-bit 565 big-endian pixels)

Subclasses define additional properties in their dictionary as needed.

```javascript
import PixelsOut from "commodetto/PixelsOut";

let out = new PixelsOut({width: 120, height: 160, pixelFormat: Bitmap.RGB565BE});
```

> **Note:** The `width`, `height`, and `pixelFormat` properties of a `PixelsOut` object are fixed at the time the object is created and cannot be changed. In general, all properties included in the dictionary provided to the constructor should be considered read-only to keep the implementation of the `PixelsOut` subclasses simple. However, subclasses of `PixelsOut` may choose to provide ways to modify some properties.

***

##### `begin(x, y, width, height)`

The `begin` function starts the delivery of a frame to the output. The `x` and `y` arguments indicate the top-left corner of the frame; the `width` and `height` arguments, the size of the frame. The area contained by the arguments to the `begin` function must fit entirely within the dimensions provided to the constructor in its dictionary.

***

##### `end()`

The `end` function indicates that delivery of the current frame to the output is complete. Calling `end` is required for proper output of the frame.

***

<a id="PixelsOut-continue"></a>
##### `continue(x, y, width, height)`

The `continue` function is a way to draw to more than one area of a single frame. Calling `continue` is similar to calling `end` following by `begin`. That is, the following

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

***

##### `send(pixels, offset = 0, count = pixels.byteLength - offset)`

The `send` function transmits pixels to the output. The `pixels` argument is an `ArrayBuffer` or `HostBuffer` containing the pixels. The `offset` argument is the offset (in bytes) at which the pixels begin in the buffer, and `count` is the number of bytes to transmit from the buffer.

```javascript
let pixels = new ArrayBuffer(40);
out.send(pixels);		// Send all pixels in buffer
out.send(pixels, 30);	// Send last 5 pixels in buffer
out.send(pixels, 2, 2)	// Send 2nd pixel in buffer
```

>**Note:** The `offset` and `count` arguments are in units of bytes, not pixels.

***

##### `adaptInvalid(r)`

The `adaptInvalid` function takes a rectangle argument specifying an area inside the `PixelsOut` area and modifies the rectangle as needed to make it a valid update area for the PixelsOut. The `adaptInvalid` function is necessary because some display controllers are only able to update the display in certain quanta. For example, one display requires that the x coordinate and width are even numbers, another display can only update complete horizontal scan lines, and another updates the entire display even if only a single pixel changed.

	let r = renderer.rectangle(x, y, w, h);
	pixelsOut.adaptInvalid(r);
	pixelsOut.begin(r.x, r.y, r.w, r.h);
	...

***

##### `pixelsToBytes(count)`

The `pixelsToBytes` function calculates the number of bytes required to store the number of pixels specified by the `count` argument. The following allocates an `ArrayBuffer` corresponding to a single scanline of pixels: 

```javascript
let buffer = new ArrayBuffer(out.pixelsToBytes(width));
```

***

#### Properties

| Name | Type | Read-only | Description |
| :---: | :---: | :---: | :--- |
| `width` | `Number` | ✓ | The width of the `PixelsOut` instance in pixels.
|`height` | `Number` | ✓ | The height of the `PixelsOut` instance in pixels.
| `pixelFormat` | `Number` | ✓ | The format of the pixels of the `PixelsOut` instance.
`async` | `Boolean` | ✓ | Returns true if the PixelsOut supports asynchronous rendering. This means  pixels passed to `send` are not copied, and must remain unchanged through the completion of the next call to `send` or `end`.
| `c_dispatch` |  | ✓ | Optionally returns a pointer to a `HostBuffer` that contains a native `PixelsOutDispatchRecord`. Returns undefined if no native dispatch table is available. The native `PixelsOutDispatchRecord` allows native code to call the `PixelsOut`'s `begin`, `send`, `continue`, `end`, and `adaptInvalid` functions directly, without going through the XS virtual machine. The native dispatch table is strictly an optimization and provides only functionality in the JavaScript API.
| `clut` |  |  | (Not documented yet)

***

<a id="spiout-class"></a>
### SPIOut Class

The `SPIOut` subclass of `PixelsOut` is a placeholder for an implementation of a `PixelsOut` instance that sends pixels to a display connected over SPI.

```javascript
class SPIOut extends PixelsOut
```

***

<a id="bufferout-class"></a>
### BufferOut Class

`BufferOut` is a subclass of `PixelsOut` that implements an offscreen in-memory bitmap. 

```javascript
class BufferOut extends PixelsOut;
```

Because memory tends to be a scarce resource on target devices, applications should use `BufferOut` sparingly.

To create a `BufferOut` instance:

```javascript
import BufferOut from "commodetto/BufferOut";

let offscreen = new BufferOut({width: 40, height: 30, pixelFormat: Bitmap.RGB565LE});
```

The `BufferOut` constructor allocates the buffer for the bitmap pixels. Once the offscreen bitmap has been created, pixels are delivered to it using the `send` function, as described for the `PixelsOut` class. The pixels in the `BufferOut` instance are accessed through the instance's bitmap.

```javascript
let bitmap = offscreen.bitmap;
```

`BufferOut` implements the optional `continue` function of `PixelsOut`.

#### Properties

| Name | Type | Read-only | Description |
| :---: | :---: | :---: | :--- |
| `bitmap ` | `Bitmap` | ✓ | Returns a `Bitmap` instance to access the pixels of the `BufferOut` instance.

***

<a id="bmpout-class"></a>
### BMPOut Class

`BMPOut` is a subclass of `PixelsOut` that receives pixels and writes them in a BMP file in the following formats: `Bitmap.Gray16`, `Bitmap.Gray256`, `Bitmap.RGB565LE`, `Bitmap.RGB332`, and `Bitmap.CLUT16`. The `BMPOut` implementation writes pixels to the file incrementally, so it can create files larger than available free memory.

`BMPOut` adds the `path` property to the constructor's dictionary; `path` is a string containing the full path to the output BMP file.

> **Note:** The width multiplied by the bit-depth of the BMP file must be a multiple of 32.

***

<a id="rle4out-class"></a>
### RLE4Out Class

`RLE4Out` is a subclass of `PixelsOut` that receives pixels in `Bitmap.Gray16` format and packs them into an RLE-encoded bitmap.

```javascript
class RLE4Out extends PixelsOut;
```

RLE-encoded bitmaps are smaller, reducing the amount of ROM required for assets. Additionally, for many common bitmaps uses, RLE-encoded bitmaps draw more quickly.

The `RLE4Out` class may be used on the device. More commonly, however, a bitmap is RLE-encoded and written to storage at build time; this avoids the time and memory required to encode the image on the device, while saving storage space.

The `rle4encode` tool in the Moddable SDK compresses 4-bit grayscale BMP files using the `RLE4Out` class. The `compressbmf` tool applies the RLE4Out class to each individual glyph in a BMF font.

The `RLE4Out` class does not implement the optional `continue` function.

#### Properties

| Name | Type | Read-only | Description |
| :---: | :---: | :---: | :--- |
| `bitmap ` | `Bitmap` | ✓ | Returns a `Bitmap` instance to access the pixels of the `RLE4Out` instance.

***

<a id="colorcellout-class"></a>
### ColorCellOut Class

`ColorCellOut` is a subclass of `PixelsOut` that receives pixels in `Bitmap.RGB565LE` format and compresses them using a modified version of the [ColorCell](https://en.wikipedia.org/wiki/Color_Cell_Compression) image compression algorithm. ColorCell was widely used in the early 90's, primarily in the [Apple Video](https://en.wikipedia.org/wiki/Apple_Video) algorithm.

The Moddable SDK's `image2cs` tool uses the `ColorCellOut` class to compress JPEG, PNG, and GIF images and image sequences.

The `send` function of the ColorCellOut class requires that the number of scan lines provided on each call be a multiple of 4. This allows the ColorCellOut implementation to encode complete rows of the image on each call to `send`.

The `begin` function takes an optional 5th parameter, a dictionary of compression options for the frame. The dictionary contains an optional `quality` property with values from 0 to 1, with 0 being the lowest quality and 1 the highest.

#### Properties

| Name | Type | Read-only | Description |
| :---: | :---: | :---: | :--- |
| `bitmap ` | `Bitmap` | ✓ | Returns a `Bitmap` instance to access the pixels of the `ColorCellOut ` instance.

***

<a id="asset-parsing"></a>
## Asset Parsing

Building a user interface for a display requires visual assets--icons, bitmaps, photos, fonts, and so on. There are many commonly used file formats for storing these assets, some of which work well on constrained devices. Commodetto includes functions to use several common asset file formats directly; however, Commodetto does not support all features of these file formats. Graphic designers creating assets for use with Commodetto need to be aware of the asset format requirements.

> **Note:** For optimal render performance and minimum storage size, it is beneficial to convert assets to the preferred format of the target device at build time.

<a id="bmp"></a>
### BMP

The [BMP](https://msdn.microsoft.com/en-us/library/dd183391.aspx) file format was created by Microsoft for use on Windows. It is a flexible container for uncompressed pixels of various formats. The format is unambiguously documented and well supported by graphic tools. BMP is the preferred format for uncompressed bitmaps in Commodetto. The Commodetto BMP file parser supports the following variants of BMP:

* **16-bit 565 little-endian pixels** -- The image width must be a multiple of 2. When using Photoshop to save the BMP file, select **Advanced Mode** in the BMP dialog and check **R5 G6 B5**.
* **16-bit 555 little-endian pixels** -- The image width must be a multiple of 2. This format is the most common 16-bit format for BMP files; however, it is not recommended, as the pixels must be converted to 565, which takes time and requires memory to store the converted pixels.
* **8-bit gray** -- The image width must be a multiple of 4 and must have a gray palette.
* **8-bit color** -- The image width must be a multiple of 4 and must have an RGB 332 palette.
* **4-bit gray** -- The image width must be a multiple of 8 and must have a gray palette. When working in Photoshop, set **Image Mode** to **Grayscale** before saving the image in BMP format.
* **4-bit color** -- The image width must be a multiple of 8.
* **1-bit black and white** -- The image width must be a multiple of 32.

By default, BMP files are stored with the bottom line of the bitmap first in the file--for example, bottom-to-top order. Commodetto requires bitmaps to be stored top to bottom. When saving a BMP file, select the option to store it in top-to-bottom order. In Photoshop, check **Flip row order** to store the BMP in top-to-bottom order.

The `parseBMP` function creates a bitmap from an `ArrayBuffer` or `HostBuffer` containing BMP data. It performs validation to confirm that the file format is supported, and throws an exception if it detects an unsupported variant of BMP.

```javascript
import parseBMP from "commodetto/parseBMP";

let bmpData = new Resource("image.bmp");
let bitmap = parseBMP(bmpData)
trace(`Bitmap width ${bitmap.width}, height ${bitmap.height}\n`);
```

***

<a id="jpeg"></a>
### JPEG

The JPEG file format is the most common format for storing photos. Many resource-constrained devices have the performance to decompress JPEG images, though not all have the memory to store the result. Commodetto provides a way to render a JPEG image to an output, even if the decompressed JPEG image cannot fit into memory.

The JPEG decoder in Commodetto supports a subset of the JPEG specification. YUV encoding with H1V1 and H2V2 are supported. Grayscale JPEG images are supported, and there are no restrictions on the width and height of the JPEG image. Progressive JPEG images are not supported.

#### Decompressing entire image
To decompress a complete JPEG data to an offscreen bitmap, use the `loadJPEG` function.

```javascript
import loadJPEG from "commodetto/loadJPEG";

let jpegData = new Resource("image.jpg");
let bitmap = loadJPEG(jpegData);
```

#### Decoding block by block
The JPEG decoder also implements a block-based decode mode to return a single block of decompressed data at a time.

```javascript
import JPEG from "commodetto/readJPEG";

let jpegData = new Resource("image.jpg");
let decoder = new JPEG(jpegData);

while (decoder.ready) {
	let block = decoder.read();
	trace(`block x: ${block.x}, y: ${block.y},
			width: ${block.width}, height: ${block.height}\n`);
}
```

Each block returned is a bitmap. The `width` and `height` fields of the bitmap indicate the dimensions of the block. The width and height can change from block to block. The `x` and `y` properties indicate the placement of the block relative to the top-left corner of the full JPEG image. Blocks are returned in a left-to-right, top-to-bottom order.

The same bitmap object is used for all blocks, so the contents of the block change after each call to `read`. This means an application cannot collect all the blocks into an array for later rendering. To do that, the application must copy the data from each block.

Using a renderer, it is straightforward to incrementally send a JPEG image to a display block-by-block as it is decoded, eliminating the need to copy the data of each block. The Poco renderer documentation includes an example of this technique.

#### Streaming decode

The preceding sections explain how to decode a JPEG image when the full compressed JPEG image is available. The JPEG decoder can also be used to decode a streaming JPEG image, that is decode JPEG data as it arrives over the network. This approach has the advantage of eliminating the need to store the entire compressed JPEG image in memory. Instead, network buffers are pushed to the JPEG decoder as they arrive, and `read` is used to retrieve as many decoded blocks as possible from the available buffers.

To use the JPEG decoder in streaming mode, call the constructor with no arguments:

```javascript
let decoder = new JPEG();
```

As data arrives, push `ArrayBuffer` containing the data to the decoder:

```javascript
decoder.push(buffer);
```

After the final buffer arrives, call `push` with parameters to indicate the end of the data stream. Failure to do this may result in blocks missing from the bottom of the image:

```javascript
decoder.push();
```

The JPEG decoder `ready` property indicates blocks are available to be read. The first time the `ready` property returns true also indicates that the JPEG header has been successfully parsed and the JPEG `width` and `height` properties have been initialized.

```javascript
decoder.push(buffer);
while (decoder.ready) {
	let block = decoder.read();
	// render block
}
```

> **Note:** Commodetto uses the excellent public domain [picojpeg](https://code.google.com/archive/p/picojpeg/) decoder, which is optimized to minimize memory use. Some quality and performance are sacrificed, but the results are generally quite good. Small changes have been made to picojpeg to eliminate compiler warnings; those changes are included in the Moddable SDK source code distribution.

***

<a id="png"></a>
### PNG

The PNG image format is commonly used for the assets of user interface elements such as buttons and sliders. Because the PNG file format is heavily compressed, PNG images must be decompressed to a `BufferOut` instance for use. Also because of the compression used in PNG, a significant amount of memory is required for decompressing the image. Nonetheless, because PNG is so common in user interface work, Commodetto implements a PNG module for use on devices and scenarios where it is practical.

The PNG decoder in Commodetto supports most variations of the PNG file format, with two exceptions: 

- Interlaced images are not supported, as interlacing is incompatible with progressive decoding.

- Images with 16-bit channels are unsupported because the high resolution exceeds the image quality capabilities of target devices.

To decompress PNG data to an offscreen bitmap, use the static `decompress` function.

```javascript
import PNG from "commodetto/readPNG";

let pngData = new Resource("image.png");
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
let pngData = new Resource("image_with_alpha.png");
let bitmap = PNG.decompress(pngData, true);
let alpha = bitmap.mask;
```

The PNG decoder also implements a progressive decoding mode to return a single scanline of decompressed data at a time. The scanline is raw data from the PNG decoder, with no pixel transformations applied. The following example converts a 24-bit (RGB) or 32-bit (RGBA) PNG image to a 16-bit BMP.

```js
let png = new PNG(new Resource("image.png"));
let width = png.width, height = png.height;
trace(`width ${width}, height ${height}, channels ${png.channels}, depth ${png.depth}, bpp ${png.channels * png.depth}\n`);

if ((8 != png.depth) || ((3 != png.channels) && (4 != png.channels)))
	throw new Error("unsupported PNG variant");

let bmp = new BMPOut({width: width, height: height, pixelFormat: Bitmap.RGB565LE, path: "image.bmp"});
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
```

In addition to the `width` and `height` properties, the PNG instance contains `depth` and `channels` properties based on the content of the PNG image.

The PNG decoder uses up to 45 KB of memory while decoding an image. This amount of memory may not be available or practical on all target devices. The memory requirement is primarily due to the zlib compression algorithm used in PNG images.

> **Note:** Commodetto uses the public domain [miniz](https://github.com/richgel999/miniz/) library to decompress the [zlib](http://www.zlib.net/manual.html) data contained in PNG images. The PNG parsing is partially based on the Apache-licensed [`FskPNGDecodeExtension.c`](https://github.com/Kinoma/kinomajs/blob/master/extensions/FskPNGDecode/sources/FskPNGDecodeExtension.c) from KinomaJS, with significant simplifications.

***

<a id="bmfont"></a>
### BMFont

[BMFont](http://www.angelcode.com/products/bmfont/) is a format for storing bitmap fonts. It is widely used to embed distinctive fonts in games in a format that is efficiently rendered using OpenGL. BMFont is well designed and straightforward to support. Commodetto uses BMFont to store both anti-aliased and multicolor fonts. In addition, BMFont has good tool support--in particular the excellent [Glyph Designer](https://71squared.com/glyphdesigner), which converts macOS fonts to a Commodetto-compatible BMFont.  For Windows and Linux users, the command line [bmfont](https://github.com/vladimirgamalyan/fontbm) has been used  successfully. 

BMFont stores a font's metrics data separately from the font's glyph atlas (bitmap). This means that loading a BMFont requires two steps: loading the metrics and loading the glyph atlas. BMFont allows the metrics data to be stored in a variety of formats, including text, XML, and binary. Commodetto supports the binary format for metrics.

The `parseBMF` function prepares the BMFont binary metrics file for use with a `Render` object.

```javascript
import parseBMF from "commodetto/parseBMF";
import parseBMP from "commodetto/parseBMP";

let palatino36 = parseBMF(new Resource("palatino36.fnt"));
palatino36.bitmap = parseBMP(new Resource("palatino36.bmp");
```

After the metrics are prepared with `parseBMF`, the glyph atlas is prepared using `parseBMP` and is attached to the metrics as the `bitmap` property.

 BMFont files with discontiguous ranges of characters are supported. Commodetto may be configured to use the kerning data that may be  prseent in a BMFont, though it is disabled by default. 

For anti-aliased text, the BMP file containing the glyph atlas bitmap must be in 4-bit gray format. For multicolor text, the bitmap must be in `Bitmap.default` format (e.g. the pixel format Commodetto is configured to render to).

Commodetto extends the BMFont format with RLE compressed glyphs. Glyphs are individually compressed using the `RLE4Out` class. The Moddable SDK tool `compressbmf` performs the compression.  The tool also appends the compressed glyphs to the `.fnt` metrics file, storing a single font's metrics and glyph data in a single file.

***

<a id="rendering"></a>
## Rendering

Commodetto is designed to support multiple rendering engines. The initial engine is [Poco](./poco.md), a small bitmap-based scanline renderer. A renderer knows how to draw pixels and relies on a `PixelsOut` instance to output those pixels, whether to a display, an offscreen buffer, or a file.

When a renderer is created, it is bound to an output. For example, to render to a BMP file:

```javascript
import BMPOut from "commodetto/BMPOut";
import Poco from "commodetto/Poco";

let bmpOut = new BMPOut({width: decoded.width, height: decoded.height,
		pixelFormat: Bitmap.RGB565LE, path: "allegra64.bmp"});
let render = new Poco(bmpOut);
```

To render to a display, use `SPIOut` in place of `BMPOut`, as follows:

```javascript
import SPIOut from "commodetto/SPIOut";
import Poco from "commodetto/Poco";

let display = new SPIOut({width: 320, height: 240,
		pixelFormat: Bitmap.RGB565LE, dataPin: 30});
let render = new Poco(display);
```

The Poco renderer documentation describes its rendering operations with examples of common uses.

***

<a id="render-class"></a>
### Render class

The `Render` class is a abstract base class used to generate pixels. It is overridden by specific rendering engines, such as Poco. The `Render` class has only four functions, which manage the rendering process but do no rendering themselves. The specific rendering operations available are defined by subclasses of `Render`.

The following example shows using the Poco renderer with `SPIOut` to render a screen consisting of a white background with a 10-pixel red square at location `{x: 5, y: 5}`.

```javascript
let display = new SPIOut({width: 320, height: 240,
		pixelFormat: Bitmap.RGB565LE, dataPin: 30});
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

***

##### `begin(x, y, width, height)`

The `begin` function starts the rendering of a frame. The area to be updated is specified with the `x` and `y` coordinates and `width` and `height` dimensions. The area must be fully contained within the bounds of the `PixelsOut` instance bound to the renderer.

```javascript
render.begin(x, y, width, height);
```

All drawing is clipped to the updated area defined by `begin`.

Calling `begin` with no arguments is equivalent to calling it with `x` and `y` equal to 0 and `width` and `height` equal to the `width` and `height` values of the `render` instance. That is, the following

```javascript
render.begin()
```
	
is equivalent to this:

```javascript
render.begin(0, 0, render.width, render.height);
```

If `width` and `height` are omitted, the update area is the rectangle defined by the `x` and `y` coordinates passed to `begin`. The following bottom-right corner of the `render` bounds

```javascript
render.begin(x, y);
```

is equivalent to this:

```javascript
render.begin(x, y, render.width - x, render.height - y);
```

***

##### `end()`

The `end` function completes the rendering of a frame. All pending rendering operations are completed by this function.

> **Note:** For a display list renderer, such as Poco, all rendering occurs during the execution of `end`. Consequently, the display is not updated immediately following drawing calls.

***

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

***

##### `adaptInvalid(r)`

The renderer's `adaptInvalid` function is used to adjust the update area defined by the `r` argument to the update area constraints of the `PixelsOut` used for rendering. The renderer's implementation of `adaptInvalid` invokes the `PixelsOut`'s `adaptInvalid` function, taking renderer features into consideration, most notably rotation.

```javascript
let r = renderer.rectangle(x, y, w, h);
renderer.adaptInvalid(r);
renderer.begin(r.x, r.y, r.w, r.h);
...
```

Applications written to work with a single display controller should be able to safely ignore `adaptInvalid` by applying its constraints themselves. Code written to work with more than a single display controller will likely need to use `adaptInvalid` for reliable updates to subsections of the display.

The Piu user interface framework calls `adaptInvalid` when necessary, so application scripts don't need to call it directly.

***

<a id="pixel-format-conversion"></a>
## Pixel format conversion

Commodetto provides a pixel format conversion capability intended primarily for use in tools that process images running on a computer. They are used, for example, by the `image2cs` and `png2bmp` tools in the Moddable SDK. The converters are small and efficient, and so may also be used in deployments to microcontrollers.

<a id="convert-class"></a>
### Convert class

The `Convert` class converts between two pixel formats supported by Commodetto. The conversions are implemented in native code and so run more quickly than they would if written in JavaScript.

```js
import Convert from "commodetto/Convert";
```

The `Convert` class operates on an array of pixels. The concepts of Bitmap and row bytes (or stride) are not part of the API. Instead, they must be handled by the code that calls the `Convert` class.

#### `constructor(src, dst)`

The constructor accepts two pixel formats, the source and destination formats.

The following instantiates a converter to convert pixels from RGB565LE to Gray256.

```js
let converter = new Convert(Bitmap.RGB565LE, Bitmap.Gray256);
```

***

#### `process(src, dst)`
#### `process(src, srcOffset, srcLength, dst, src, dstOffset, dstLength)`

The `process` function performs a pixel conversion. The `src` argument is the input pixels in the format specified in the constructor. The input and output pixels are stored in a buffer -- `ArrayBuffer`, `TypedArray`, `DataView`, or `HostBuffer`.

There are two ways to call `process`. The first passes only the input and output buffers. The second passes offsets and lengths within the input and output buffers to use. Note that `process` always respects the view of the input and output buffers and applies passed offset and lengths to the view.

```js
converter.process(inputPixels, outputPixels);
```

The caller of process is responsible for allocating a large enough output buffer. The `Bitmap.depth` function is useful for this calculation.

```js
let outputPixelFormat = Bitmap.Gray256;
let inputPixelCount = 240;
let outputBufferSize = ((Bitmap.depth(outputPixelFormat) * inputPixelCount) + 7) >> 3;
let outputPixels = new ArrayBuffer(outputBufferSize);
```

***

<a id="odds-and-ends"></a>
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

Commodetto runs on any target hardware that supports the XS JavaScript engine. Commodetto is written in ANSI C, with only a handful of calls to external functions (`memcpy`, `malloc`, and `free`). The core Poco renderer allocates no memory.

***

### About the Name "Commodetto"

The word *commodetto* is a term used in music meaning "leisurely." The use of the name Commodetto here is taken from a set of piano variations by Beethoven, specifically the third variation of WoO 66. A sample of the variation is [available for listening](http://www.prestoclassical.co.uk/r/Warner%2BClassics/5857612). The feeling of the variation is light and leisurely, though there is nothing simple or trivial about the composition or the performance.
