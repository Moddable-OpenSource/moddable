# Moddable SDK - Commodetto Examples

These examples demonstrate how to use features of [Commodetto](../../documentation/commodetto/commodetto.md), a bitmap graphics library that provides a 2D graphics API. Commodetto includes the lightweight [Poco rendering engine](../../documentation/commodetto/poco.md), a display list renderer able to efficiently render a single scanline at a time, saving considerable memory by eliminating the need for a frame buffer. 

Most of the examples are designed for a QVGA (320x240) screen, but many work on a variety of screen sizes. All of the examples in this folder run on the desktop simulator with the exception of the `mini-drag` example.

This document provides a brief description of each example and a preview of each app running on the desktop simulator. If you are looking for an example that demonstrates how to use a specific feature, see the list below. Keep in mind that this list provides only a few recommendations and is not a complete list of examples that use each feature.

- **Images:** <a href="#image-frames">`image-frames`</a>, <a href="#docs">`docs`</a>, <a href="#rotated">`rotated`</a>, <a href="#sprite">`sprite`</a>
- **Text:** <a href="#text">`text`</a>, <a href="#text-ticker">`text-ticker`</a>, <a href="#cfe8x8">`cfe8x8`</a>, <a href="#cfeNFNT">`cfeNFNT`</a>
- **Animation:** <a href="#docs">`docs`</a>, <a href="#progress">`progress`</a>, <a href="#text-ticker">`text-ticker`</a>
- **Touch input:** <a href="#mini-drag">`mini-drag`</a>
- **Networking:** <a href="##jpeghttp-and-jpegstream">`jpeghttp`</a>, <a href="##jpeghttp-and-jpegstream">`jpegstream`</a>

***

### `cfe8x8`

<img src="https://www.moddable.com/assets/commodetto-gifs/cfe8x8.gif" width=180>

The `cfe8x8` example demonstrates how to use a simple embedded 8 x 8 bitmap font. This is the simplest example of implementing a new Commodetto Font Engine.

***

### `cfeNFNT`

<img src="https://www.moddable.com/assets/commodetto-gifs/cfeNFNT.gif" width=180>

The `cfeNFNT` example renders text using an NFNT font resource, the bitmap font format of the original Macintosh. Includes an Commodetto Font Engine. for NFNT.

***

### `clip`

![](https://www.moddable.com/assets/commodetto-gifs/clipped.png)

The `clip` example shows how to use the drawing clip. The clip stack is maintained by the Poco rendering engine.

***

### `clock`

![](https://www.moddable.com/assets/commodetto-gifs/clock.gif)

The `clock` example shows a simple on screen clock with a blinking colon. It demonstrates how to center text to build an application that works on any screen size.

***

### `docs`

![](https://www.moddable.com/assets/commodetto-gifs/docs.gif)

The `docs` example includes all of the examples in the [Poco documentation](../../documentation/commodetto/poco.md).

***

### `image-frames`

![](https://www.moddable.com/assets/commodetto-gifs/image-frames.gif)

The `image-frames` example shows an animated flag. It demonstrates how to render a gif.

***

### `jpeghttp` and `jpegstream`

![](https://www.moddable.com/assets/commodetto-gifs/jpeghttp.gif) ![](https://www.moddable.com/assets/commodetto-gifs/jpegstream.gif)

The `jpeghttp` and `jpegstream` examples fetch images from moddable.com and display them on screen. These examples run on the ESP8266, which doesn't have enough memory to hold the compressed JPEG images. Async/await is used to decode and render the JPEG images as they arrive, allowing the application to overcome the RAM limits.

***

### `mini-drag`

The `mini-drag` example is great for testing touch on displays. You can move the object by touching it and dragging across the screen.

***

### `origin`

![](https://www.moddable.com/assets/commodetto-gifs/origin.png)

The `origin` example demonstrates the use of the Poco drawing origin stack.

***

### `progress`

![](https://www.moddable.com/assets/commodetto-gifs/progress.gif)

The `progress` example displays high frame rate animations of progress bars and a spinner, all of which are useful for loading screens.

***

### `rotated`

![](https://www.moddable.com/assets/commodetto-gifs/rotated.png)

The `rotated` example demonstrates how to define the rotation of the screen. This example rotates 90 degrees; 0, 180, and 270 degree rotation is also supported. 

***

### `sprite`

![](https://www.moddable.com/assets/commodetto-gifs/sprite.gif)

The `sprite` example displays a simple spinning animation, useful for loading screens. It demonstrates how to cycle through different portions of a single image to create an animated sprite.

***

### `text-ticker`

<img src="https://www.moddable.com/assets/commodetto-gifs/text-ticker.gif" width=180>

The `text-ticker` example displays the message "Greetings from Moddable..." animated in a loop.

***

### `text`

<img src="https://www.moddable.com/assets/commodetto-gifs/text.png" width=180>

The `text` example demonstrates different truncation and alignment options for rendering text.

***
