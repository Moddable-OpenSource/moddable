# Animated GIF for Commodetto
Copyright 2021 Moddable Tech, Inc.<BR>
Updated: April 11, 2021

## Introduction

The animated GIF image format is popular and possible to implement for microcontrollers. The Commodetto graphics library provides high quality, high performance rendering on microcontrollers for delivering products with smooth animation.

Most GIF implementations for microcontrollers decode directly to the frame buffer to minimize memory use. This works for some animated GIFs, but for others it causes flickering or artifacts. The Commodetto animated GIF decoder takes a different approach: it requires a memory buffer for the full decoded image. This makes it unsuitable when memory is tight. However, for those devices where it works it is able to deliver correct, high performance, flicker free animations. Because the output of the decoder is a Commodetto bitmap it may be composited with other graphical elements using the Poco renderer.

The Commodetto animated GIF decoder has several special decoding modes to significantly reduce the amount of memory required to decode many animated GIF sequences.

Because of the increased memory requirements, the Commodetto animated GIF decoder is not practical on all microcontrollers. It works very well on an ESP32 (with no external RAM) and Pico (RP2040) for images that are about 200 pixels on a side. 

## Examples

To use the module, first import it:

```js
import ReadGIF from "commodetto/ReadGIF";
```

### Creating an instance
The module reads the GIF data from a resource. Pass the resource to the constructor:

```js
let reader = new ReadGIF(new Resource("moddable.gif"));
```

### Memory use
ReadGIF needs memory to store the decoding state and memory to store the decoded image. If there is insufficient memory, `ReadGIF` throws an exception.

The decoding state is in the application heap. The default configuration of the application heap is too small for this, and must be increased to about 48 KB. You can do this in your project manifest:

```json
	"creation": {
		"static": 49152
	}
```

The decoded image is stored in system memory. Check the instrumentation tab in xsbug to see how much system memory is free when calling `ReadGIF`.

### Information about the file

The `width` and `height` properties of the reader instance provide the dimensions of the decoded image.

The `duration` property is the total length of the animation in milliseconds. The `frameCount` is the total number of frames in the animation.

### Decoding a frame
To decode an image, call `next`. When the end of the animated frame sequence is reached, the decoder automatically loops back to the start. 

```js
reader.next();
```
The `next` function updates the state of the reader to the next frame. The reader itself may be used as a bitmap containing the decoded image. It has other properties useful for rendering the image. The number of the frame last decoded by `next` is available from the `frameNumber` property.

> **Note**: The `ready` property of the reader instance is `true` if at least one frame of the GIF is available to draw. When downloading an image, check `reader.ready` to determine when the reader is able to return the first frame.

### Drawing a frame
Calling `next` updates the reader's bitmap with the next frame. Use Poco to draw the bitmap as usual. The following draws the image in the top left corner of the screen.

```js
import Poco from "commodetto/Poco";

poco.begin(0, 0, reader.width, reader.height);
poco.drawBitmapWithKeyColor(reader, 0, 0);
poco.end();
```

### Optimizing drawing
Often the animation does not update the entire frame, but a subset of the pixels. In this case, it is not necessary to update all pixels on the display which can be more efficient. The reader provides the update area on its `frameX`, `frameY`, `frameWidth`, and `frameHeight` properties.

The following example runs an animation continuously at 10 frames per second, using the update area to minimize drawing.

```js
Timer.repeat(() => {
	reader.next();

	poco.begin(reader.frameX, reader.frameY, reader.frameWidth, reader.frameHeight);
	poco.drawBitmapWithKeyColor(reader, 0, 0);
	poco.end();
}, 100);
```

### Animation timing
Each frame in the animated GIF has its own duration. The duration in milliseconds of decoded frame is available on the `frameDuration` property of the reader instance.

```js
let frameDuration = reader.frameDuration;
```

### Putting it together
This example shows a typical drawing loop that optimizes the drawing area and applies each frame duration.

```js
const reader = new ReadGIF(new Resource("moddable.gif"));

Timer.repeat(id => {
	reader.next();
	Timer.schedule(id, reader.frameDuration, 1);

	poco.begin(reader.frameX, reader.frameY, reader.frameWidth, reader.frameHeight);
	poco.drawBitmapWithKeyColor(bitmap, 0, 0);
	poco.end();
}, 1);
```

### Transparency
Some animated GIF images have transparent regions which are design to allow the background show through. This is used for animations with non-rectangular shapes. The transparency is binary -- either a pixel is fully opaque or fully invisible: there is no alpha blending.

The `transparent` property of the reader instance indicates if the image has transparent pixels.

To achieve true transparency requires steps. First, the application must fill in the background pixels behind the image. The example below fills the background with red. Second, use `drawBitmapWithKeyColor` to render the image and provide it with the transparent color from the animated GIF. The `drawBitmapWithKeyColor` function is like `drawBitmap` except that it skips over any pixels that match the specified key color.

```js
const red = poco.makeColor(255, 0, 0);

poco.fillRectangle(red, 0, 0, reader.width, reader.height);
poco.drawBitmapWithKeyColor(reader, 0, 0, reader.transparentColor);
```

Note that the `fillRectangle` call draws behind the animated GIF. Your code can draw anything behind the GIF, such as a pattern or a logo or even another animation. It is safe to use `drawBitmapWithKeyColor`.

### Reducing memory use
A GIF image contains between 2 and about 16 million colors. Images with many colors must be stored at 16-bits per pixel, which uses quite a bit of memory. The Commodetto GIF decoder is also able to decode GIF images to 8-bit, 4-bit, and 1-bit pixels. The decoder automatically determines the format that uses the least memory while maintaining full color fidelity. Thee bitmap format used may always be drawn using `poco.drawBitmapWithKeyColor`. The format of the bitmap used is available from the `pixelFormat` property of the GIF Reader instance. 

Some applications may wish to force decoding to a specific format, overriding the automatic format detection. This may generate an image which does not render correctly, but should never crash. To force decoding to a particular format, pass the pixel format to the `ReadGIF` constructor as part of the optional options argument. The following example shows requesting pixels in RGB565 little-endian format:

```js
const reader = new ReadGIF(
	new Resource("moddable.gif"),
	{
		pixelFormat: Bitmap.RGB565LE
	}
);
```

For force decoding to 1, 4, and 8 bits per pixel use `Bitmap.Monochrome`, `Bitmap.Gray16`, and `Bitmap.CLUT256` respectively.

## Reference

The following table describes the properties available on the GIF reader instance. A GIF animation is draw onto a canvas. Each frame of the animation contains an image that updates the canvas. The image may update all of the canvas or only a part of it.

| Property | Description |
| ---: | :--- |
| `width` | width of the animation canvas |
| `height` | height of the animation canvas |
| `duration` | total duration of the animation in milliseconds |
| `frameBounds` | a rectangle object containing the update area of the current frame |
| `frameCount` | total number of frames in the animation |
| `frameDuration` | duration of the current frame in milliseconds |
| `frameNumber` | index number of the current frame |
| `frameX` | horizontal offset into the canvas of the area updated from the previous frame |
| `frameY` | vertical offset into the canvas of the area updated from the previous frame
| `frameWidth` | width of the area updated in the current frame |
| `frameHeight` | height of the area updated in the current frame |
| `transparent` | `true` if the canvas contains transparent background pixels |
| `transparentColor` | pixel value used to fill transparent background pixels; undefined if image does not use transparency |
| `ready` | `true` if the animated GIF reader is able to parse at least one frame; used when downloading GIF images to detect first frame is available  |


## Thank you

The core GIF decoder is [GIF Animator](https://github.com/bitbank2/AnimatedGIF) by [Larry Bank](https://github.com/bitbank2) of [BitBank Software](https://www.bitbanksoftware.com). 
