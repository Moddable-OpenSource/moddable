# Piu Extensions
Copyright 2026 Moddable Tech, Inc.<BR>
Revised: October 12, 2026

## About This Document

Piu is a user interface framework designed to run efficiently on microcontrollers. The core of Piu is described in the [Piu JavaScript Reference](./piu.md). This document describes `content` objects that extend Piu's core: animated GIF images, pixel buffers, QR Codes, rounded rectangles, outlines, and vector images. There are also behaviors to provide mobile-style scrolling.

Extensions are not part of the core to keep projects small. Projects add extensions by including their manifest. Each section below gives the manifest to include.

## Table of Contents

  * [Using Extensions](#using-extensions)
  * [Piu Extensions Object Reference](#piu-extensions-object-reference)
 	 * [GIFImage Object](#gifimage-object)
 	 * [ImageBuffer Object](#imagebuffer-object)
 	 * [Kinetic Scroller Behaviors](#kinetic-scroller-behaviors)
 	 * [QRCode Object](#qrcode-object)
 	 * [RoundRect Object](#roundrect-object)
 	 * [Shape Object](#shape-object)
 	 * [SVGImage Object](#svgimage-object)

## Using Extensions

First, the project manifest must includes the extension's manifest, here `qrcode`:

```json
"include": [
	"$(MODDABLE)/examples/manifest_base.json",
	"$(MODDABLE)/examples/manifest_piu.json",
	"$(MODDABLE)/modules/piu/MC/qrcode"
],
```

Second, the application imports the extension's module. Importing the module adds the extension's constructor to the globals, like the core constructors.

```javascript
import {} from "piu/MC";
import {} from "piu/QRCode";
```

Once imported, the extension's constructors are used can be called with `new`, called without `new` in the `contents` of a `container` object, and they provide a `template` function.

| Object | Manifest | Module |
| --- | --- | :--- |
| `gifImage` | `$(MODDABLE)/modules/piu/MC/gif` | `piu/GIFImage`
| `imageBuffer` | `$(MODDABLE)/modules/piu/MC/imageBuffer` | `piu/ImageBuffer`
| kinetic scroller | `$(MODDABLE)/modules/piu/MC/kineticScroller` | `piu/KineticScroller`
| `qrCode` | `$(MODDABLE)/modules/piu/MC/qrcode` | `piu/QRCode`
| `roundRect` | `$(MODDABLE)/modules/piu/MC/outline/manifestRoundRect.json` | `piu/RoundRect`
| `shape` | `$(MODDABLE)/modules/piu/MC/outline` | `piu/shape`
| `svgImage` | `$(MODDABLE)/modules/piu/MC/outline/manifestSVGImage.json` | `piu/SVGImage`

> The `image` object, which displays images compressed with the ColorCell algorithm, is also an extension. Because it was originally part of the core, it is described in the [Image Object](./piu.md#image-object) section of the Piu JavaScript Reference.

## Piu Extensions Object Reference

This section provides details on the objects that extend the Piu API. The information presented for each object is the same as in the [Piu Object Reference](./piu.md#piu-object-reference), plus:

- **Manifest**: The manifest to include to add the object to an application
- **Module**: The module to import to define the constructor of the object

### GIFImage Object

- **Source code:** [`piuGIFImage.c`][0]
- **Relevant Examples:** [gifimage][20], [first-run-moddable-four][10], [giphy][11]
- **Manifest:** `$(MODDABLE)/modules/piu/MC/gif`
- **Module:** `piu/GIFImage`

The `gifImage` object is a `content` object that displays animated GIF images.

The `gifImage` object decodes the GIF file at runtime, unlike the `image` object which converts GIF files to ColorCell when building. Consequently the GIF image may be a resource of the application or a buffer that the application gets at runtime, for example by downloading it. Note that runtime decoding of GIF images generally requires more CPU power and memory.

```json
"include": [
	"$(MODDABLE)/examples/manifest_base.json",
	"$(MODDABLE)/examples/manifest_piu.json",
	"$(MODDABLE)/modules/piu/MC/gif"
],
```

GIF files that are resources of the application must be defined in the `data` of your manifest so the build includes them unchanged. Scripts reference them by name, including the `.gif` extension, as shown below.

```json
"data":{
	"*": [
		"$(MODDABLE)/examples/assets/images/anemometer"
	]
},
```

The `gifImage` object opens the GIF image when it is bound to the displayed containment hierarchy and closes the GIF image when it is unbound. While it is bound, the `gifImage` object uses memory to hold the decoded frame.

When the GIF image is opened, the `gifImage` object sets its duration to the duration of the GIF image and its time to `0`. The frame displayed is the frame at the time of the `gifImage` object. Use the `start` and `stop` functions of the `content` object to play and pause the animation.

Transparent pixels of the GIF image are not drawn, provided the transparent color of the GIF image is also its background color. The GIF format has no partial transparency: each pixel is either transparent or opaque.

The measured width and measured height of a `gifImage` object are the width and height of its GIF image. The GIF image is clipped by the bounds of the `gifImage` object.

#### Constructor Description

##### `GIFImage([behaviorData, dictionary])`

| Argument | Type | Description |
| --- | --- | :--- |
| `behaviorData` | `*` | A parameter that is passed into the `onCreate` function of this GIF image's `behavior`. This may be any type of object, including `null` or a dictionary with arbitrary parameters.
| `dictionary` | `object` | An object with properties to initialize the result. Only parameters specified in the [Dictionary](#gifimage-dictionary) section below will have an effect; other parameters will be ignored.

Returns a `gifImage` instance, an object that inherits from `GIFImage.prototype`

```javascript
class GIFImageBehavior extends Behavior {
	onDisplaying(image) {
		image.start();
	}
}
let sampleGIFImage = new GIFImage(null, {
	path: "anemometer.gif", loop: true, Behavior: GIFImageBehavior
});
application.add(sampleGIFImage);
```

> This example uses `anemometer.gif` from [the example images directory](../../examples/assets/images)

##### `GIFImage.template(anonymous)`

| Arguments | Type | Description
| --- | --- | :--- |
| `anonymous` | `function` | A function that returns an object with properties to initialize the instances that the result creates

Returns a constructor, a function that creates instances of `GIFImage.prototype`. The `prototype` property of the result is `GIFImage.prototype`. The result also provides a `template` function.

```javascript
class SplashBehavior extends Behavior {
	onDisplaying(image) {
		image.start();
	}
	onFinished(image) {
		trace("Splash finished\n");
	}
}
let SplashImage = GIFImage.template($ => ({
	left: 0, top: 0, path: $, Behavior: SplashBehavior
}));
application.add(new SplashImage("splash.gif"));
```

<a id="gifimage-dictionary"></a>
#### Dictionary

Same as for `content` object (see [Dictionary](./piu.md#content-dictionary) in the section [Content Object](./piu.md#content-object)), plus:

| Parameter | Type | Description |
| --- | --- | :--- |
| `buffer` | `object` | A buffer that contains the GIF image, for example an `ArrayBuffer` instance. Ignored if the `path` parameter is present.
| `color` | `color` | If present, the GIF image is decoded as a gray image and drawn with this color, the gray levels being used as the alpha channel. The `flip` parameter is ignored. See [Color](./piu.md#color) for the values.
| `flip` | `string` | Flips the GIF image: `"h"` horizontally, `"v"` vertically, or `"hv"` both horizontally and vertically.
| `path` | `string` | The name of the resource that contains the GIF image.

#### Prototype Description

Prototype inherits from `Content.prototype`.

The `gifImage` object has no properties or functions of its own. The animation is controlled with the `duration`, `fraction`, `loop`, and `time` properties and the `start` and `stop` functions of the `content` object.

#### Events

Same as for `content` object (see [Events](./piu.md#content-events) in the section [Content Object](./piu.md#content-object)). The `onFinished` event is triggered when the animation reaches the end of the GIF image, unless the `loop` property is `true`.

### ImageBuffer Object

- **Source code:** [`piuImageBuffer.c`][1]
- **Relevant Examples:** [imagebuffer][22], [jpeghttp][26], [camera-test][12], [camera-balls][13]
- **Manifest:** `$(MODDABLE)/modules/piu/MC/imageBuffer`
- **Module:** `piu/ImageBuffer`

The `imageBuffer` object is a `content` object that displays a buffer of uncompressed pixels. It is useful for displaying images that are generated at runtime, such as the frames captured by a camera, the pixels rendered offscreen by Commodetto, or JPEG and PNG images decoded at runtime.

```json
"include": [
	"$(MODDABLE)/examples/manifest_base.json",
	"$(MODDABLE)/examples/manifest_piu.json",
	"$(MODDABLE)/modules/piu/MC/imageBuffer"
],
```

The `imageBuffer` object does not copy the pixels. It keeps a reference to the buffer and reads the pixels from the buffer each time it is drawn. By default, an `imageBuffer` object has no buffer and displays nothing. Set the `buffer` property to display pixels.

The width, height, and pixel format of the image are defined when the `imageBuffer` object is created and cannot be changed. The buffer must contain at least `imageWidth` times `imageHeight` pixels in that pixel format.

The measured width and measured height of an `imageBuffer` object are the width and height of its image. The image is clipped by the bounds of the `imageBuffer` object.

#### Constructor Description

##### `ImageBuffer([behaviorData, dictionary])`

| Argument | Type | Description |
| --- | --- | :--- |
| `behaviorData` | `*` | A parameter that is passed into the `onCreate` function of this image buffer's `behavior`. This may be any type of object, including `null` or a dictionary with arbitrary parameters.
| `dictionary` | `object` | An object with properties to initialize the result. Only parameters specified in the [Dictionary](#imagebuffer-dictionary) section below will have an effect; other parameters will be ignored.

Returns an `imageBuffer` instance, an object that inherits from `ImageBuffer.prototype`

```javascript
const width = 176, height = 144;
let sampleImageBuffer = new ImageBuffer(null, {
	imageWidth: width, imageHeight: height
});
application.add(sampleImageBuffer);

const pixels = new Uint16Array(width * height);
pixels.fill(0xF800);
sampleImageBuffer.buffer = pixels.buffer;
```

> This example assumes the pixel format of the screen is 16 bits per pixel

##### `ImageBuffer.template(anonymous)`

| Arguments | Type | Description
| --- | --- | :--- |
| `anonymous` | `function` | A function that returns an object with properties to initialize the instances that the result creates

Returns a constructor, a function that creates instances of `ImageBuffer.prototype`. The `prototype` property of the result is `ImageBuffer.prototype`. The result also provides a `template` function.

```javascript
let CameraApplication = Application.template($ => ({
	contents: [
		ImageBuffer($, {
			anchor: "PREVIEW", imageWidth: $.camera.width, imageHeight: $.camera.height
		}),
	]
}));
```

The anchor is then used to display each frame the camera captures.

```javascript
onReadable: () => {
	frame = camera.read();
	this.PREVIEW.buffer = frame;
}
```

<a id="imagebuffer-dictionary"></a>
#### Dictionary

Same as for `content` object (see [Dictionary](./piu.md#content-dictionary) in the section [Content Object](./piu.md#content-object)), plus:

| Parameter | Type | Description |
| --- | --- | :--- |
| `imageHeight` | `number` | The height of the image, in pixels. Required.
| `imageType` | `number` | The pixel format of the image: either the pixel format of the screen (`screen.pixelFormat`) or `Bitmap.Gray16` for a 4-bit gray image. Defaults to the pixel format of the screen. An exception is thrown for other pixel formats.
| `imageWidth` | `number` | The width of the image, in pixels. Required.

#### Prototype Description

Prototype inherits from `Content.prototype`.

##### Properties

| Name | Type | Default Value | Read Only | Description |
| --- | --- | --- | --- | :--- |
| `buffer` | `object` | `undefined` | | The buffer that contains the pixels of the image, for example an `ArrayBuffer` instance. Setting this property invalidates this image buffer. Set this property after changing the pixels of the buffer to update the screen.

### Kinetic Scroller Behaviors

- **Source code:** [`piuKineticScroller.js`][6]
- **Relevant Examples:** [kinetic-scroller][23], [conversationalAI][24], [somafm][25]
- **Manifest:** `$(MODDABLE)/modules/piu/MC/kineticScroller`
- **Module:** `piu/KineticScroller`

The kinetic scroller behaviors are `behavior` objects that scroll a `scroller` object with touches, with the physics users expect on a touch screen:

- The contents follow the finger while it drags.
- When the finger is lifted while moving, the contents are tossed: they continue to scroll and slow down progressively.
- When the contents are scrolled beyond their bounds, they bounce back.

Unlike the other extensions, the kinetic scroller behaviors are not `content` objects and the module defines no global properties. The module exports two classes, `HorizontalScrollerBehavior` and `VerticalScrollerBehavior`, to scroll horizontally or vertically.

```json
"include": [
	"$(MODDABLE)/examples/manifest_base.json",
	"$(MODDABLE)/examples/manifest_piu.json",
	"$(MODDABLE)/modules/piu/MC/kineticScroller"
],
```

```javascript
import {} from "piu/MC";
import { HorizontalScrollerBehavior, VerticalScrollerBehavior } from "piu/KineticScroller";
```

A kinetic scroller behavior is used as the behavior of a `scroller` object. It scrolls the first content of the `scroller` object. The `scroller` object must be active to receive touches, and usually has the `backgroundTouch` property set to `true`, so the scroller receives the touches that begin in its contents, and the `clip` property set to `true`.

```javascript
let SampleScroller = Scroller.template($ => ({
	left: 0, right: 0, top: 0, bottom: 0,
	active: true, backgroundTouch: true, clip: true,
	Behavior: VerticalScrollerBehavior,
	contents: [
		Column($, {
			left: 0, right: 0, top: 0,
			contents: $.map(string => new Label(null, { left: 0, right: 0, height: 50, string })),
		}),
	]
}));
application.add(new SampleScroller(["one", "two", "three", "four", "five", "six", "seven", "eight"]));
```

The scroller begins to track a touch once the touch moved several pixels, primarily in the direction of the behavior. The scroller then captures the touch, so its contents trigger the `onTouchCancelled` event. Touches that do not move, or that move in the other direction, are handled by the scroller's contents. This allows, for example, a vertical scroller to contain buttons and horizontal scrollers.

While the scroller follows a touch, is tossed, or bounces back, its `tracking` property is `true`. When the scroller eventually stops, the behavior sets the `tracking` property to `false`, then triggers the `onScrolled` event of the scroller and its contents one last time, and the `onTouchScrolled` event.

If the `looping` property of the `scroller` object is `true`, the contents are never scrolled beyond their bounds, so they never bounce back.

#### Constructor Description

The kinetic scroller behaviors are classes. Applications do not call their constructors: they pass the classes, or classes that extend them, as the `Behavior` property of the dictionary of a `scroller` object.

##### `HorizontalScrollerBehavior`

Scrolls the first content of a `scroller` object horizontally.

##### `VerticalScrollerBehavior`

Scrolls the first content of a `scroller` object vertically.

```javascript
class ListScrollerBehavior extends VerticalScrollerBehavior {
	onScrolled(scroller) {
		trace(`scrolled to ${scroller.scroll.y}\n`);
	}
}
```

Classes that extend the kinetic scroller behaviors can implement any event. However the kinetic scroller behaviors themselves implement the `onCreate`, `onDisplaying`, `onFinished`, `onTimeChanged`, `onTouchBegan`, `onTouchCancelled`, `onTouchEnded`, `onTouchMoved`, and `onUndisplaying` events, and use the clock of the `scroller` object. A class that overrides one of these events must call the overridden method with `super`.

```javascript
class SampleScrollerBehavior extends VerticalScrollerBehavior {
	onCreate(scroller, data) {
		super.onCreate(scroller, data);
		this.index = 0;
	}
}
```

##### Saving the scroll position

The kinetic scroller behaviors keep the data of the `scroller` object in their `data` property. If the data is an object with a `scroll` property, the behavior sets the scroll offsets of the scroller to the value of the property when the scroller is displayed, and sets the property to the scroll offsets of the scroller when the scroller is undisplayed. Applications that rebuild their screens can use this information to restore a scroller where the user left it.

```javascript
const model = { scroll: { x: 0, y: 0 } };
application.add(new SampleScroller(model));
```

#### Prototype Description

Prototypes inherit from `Behavior.prototype`.

##### Properties

| Name | Type | Default Value | Read Only | Description |
| --- | --- | --- | --- | :--- |
| `data` | `*` | | | The data passed to the constructor of the `scroller` object

##### Functions

Classes that extend the kinetic scroller behaviors override these functions to customize how the scroller scrolls.

**`captureTouch(scroller, id, x, y, ticks)`**

| Argument | Type | Description |
| --- | --- | :--- |
| `scroller` | `scroller` | The `scroller` object that is about to track the touch
| `id` | `number` | The identifier of the touch
| `x, y` | `number` | The global coordinates of the touch, in pixels
| `ticks` | `number` | The global time of the touch

Called when the scroller is about to track a touch. The default implementation captures the touch and returns `true`. Return `false` without capturing the touch to prevent the scroller from scrolling.

```javascript
class LockableScrollerBehavior extends VerticalScrollerBehavior {
	captureTouch(scroller, id, x, y, ticks) {
		if (this.data.locked)
			return false;
		return super.captureTouch(scroller, id, x, y, ticks);
	}
}
```

***

**`snap(scroller, position, direction)`**

| Argument | Type | Description |
| --- | --- | :--- |
| `scroller` | `scroller` | The `scroller` object that is scrolling
| `position` | `number` | The scroll offset where the scroller is about to stop, in pixels
| `direction` | `number` | The direction of the scroll: negative, positive, or `0` if the scroller bounces back

Called when the scroller is tossed or bounces back to get the scroll offset where the scroller stops. The default implementation returns `position`. Return another scroll offset to stop the scroller on specific positions, for example to align the scroller on the items of a list.

```javascript
class SnapScrollerBehavior extends VerticalScrollerBehavior {
	snap(scroller, position, direction) {
		const height = 50;
		return Math.round(position / height) * height;
	}
}
```

***

#### Events

The kinetic scroller behaviors trigger the following event, in addition to the `onScrolled` event that the `scroller` object triggers (see [Events](./piu.md#scroller-events) in the section [Scroller Object](./piu.md#scroller-object)).

**`onTouchScrolled(scroller)`**

| Argument | Type | Description |
| --- | --- | :--- |
| `scroller` | `scroller` | The `scroller` object that scrolled

This event is triggered when the scroller stops after having followed a touch. It bubbles from the `scroller` object up the containment hierarchy.

### QRCode Object

- **Source code:** [`piuQRCode.c`][2]
- **Relevant Examples:** [qrcode][14]
- **Manifest:** `$(MODDABLE)/modules/piu/MC/qrcode`
- **Module:** `piu/QRCode`

The `qrCode` object is a `content` object that displays a string as a QR Code. The QR Code is generated by the [`QRCode` class](../data/data.md#qrcode).

```json
"include": [
	"$(MODDABLE)/examples/manifest_base.json",
	"$(MODDABLE)/examples/manifest_piu.json",
	"$(MODDABLE)/modules/piu/MC/qrcode"
],
```

The `qrCode` object uses the colors of its skin: the fill color for the background and the stroke color for the small squares of the QR Code (e.g. modules). If the `qrCode` object has no skin, the background is white and the modules are black.

The QR Code is scaled by the largest integer that allows it to fit in the bounds of the `qrCode` object, then centered. The `qrCode` object has no measured size, so its coordinates must define its width and height.

> For the QR Code to be scanned reliably, use colors with high contrast and leave a margin of background around the modules.

#### Constructor Description

##### `QRCode([behaviorData, dictionary])`

| Argument | Type | Description |
| --- | --- | :--- |
| `behaviorData` | `*` | A parameter that is passed into the `onCreate` function of this QR Code's `behavior`. This may be any type of object, including `null` or a dictionary with arbitrary parameters.
| `dictionary` | `object` | An object with properties to initialize the result. Only parameters specified in the [Dictionary](#qrcode-dictionary) section below will have an effect; other parameters will be ignored.

Returns a `qrCode` instance, an object that inherits from `QRCode.prototype`

```javascript
let sampleQRCode = new QRCode(null, {
	width: 160, height: 160,
	skin: new Skin({ fill: "white", stroke: "blue" }),
	string: "This is a message.",
	maxVersion: 10
});
application.add(sampleQRCode);
```

##### `QRCode.template(anonymous)`

| Arguments | Type | Description
| --- | --- | :--- |
| `anonymous` | `function` | A function that returns an object with properties to initialize the instances that the result creates

Returns a constructor, a function that creates instances of `QRCode.prototype`. The `prototype` property of the result is `QRCode.prototype`. The result also provides a `template` function.

```javascript
const qrCodeSkin = new Skin({ fill: "white", stroke: "black" });
let SampleQRCode = QRCode.template($ => ({
	width: 128, height: 128, skin: qrCodeSkin, string: $
}));
application.add(new SampleQRCode("https://www.moddable.com"));
```

<a id="qrcode-dictionary"></a>
#### Dictionary

Same as for `content` object (see [Dictionary](./piu.md#content-dictionary) in the section [Content Object](./piu.md#content-object)), plus:

| Parameter | Type | Description |
| --- | --- | :--- |
| `maxVersion` | `number` | A number between 1 and 40 inclusive indicating the maximum version of the generated QR Code. The version determines the amount of data the QR Code can contain. Defaults to `40`.
| `string` | `string` | The message to display as a QR Code.

#### Prototype Description

Prototype inherits from `Content.prototype`.

##### Properties

| Name | Type | Default Value | Read Only | Description |
| --- | --- | --- | --- | :--- |
| `string` | `string` | | | The message this QR Code displays. Setting this property generates and displays a new QR Code.

### RoundRect Object

- **Source code:** [`piuRoundRect.c`][3]
- **Relevant Examples:** [roundrect][21]
- **Manifest:** `$(MODDABLE)/modules/piu/MC/outline/manifestRoundRect.json`
- **Module:** `piu/RoundRect`

The `roundRect` object is a `content` object that fills its bounds with a rectangle with rounded corners. The rounded rectangle is anti-aliased and rendered with [outlines](../commodetto/outline/Outlines.md).

```json
"include": [
	"$(MODDABLE)/examples/manifest_base.json",
	"$(MODDABLE)/examples/manifest_piu.json",
	"$(MODDABLE)/modules/piu/MC/outline/manifestRoundRect.json"
],
```

The `roundRect` object uses the fill color of its skin. If the `roundRect` object has no skin, it displays nothing. The stroke color and the borders of the skin are ignored.

The rounded rectangle always has the width and height of the `roundRect` object. Its outline is rebuilt each time it is placed, so the `roundRect` object is resized by resizing its container.

#### Constructor Description

##### `RoundRect([behaviorData, dictionary])`

| Argument | Type | Description |
| --- | --- | :--- |
| `behaviorData` | `*` | A parameter that is passed into the `onCreate` function of this round rect's `behavior`. This may be any type of object, including `null` or a dictionary with arbitrary parameters.
| `dictionary` | `object` | An object with properties to initialize the result. Only parameters specified in the [Dictionary](#roundrect-dictionary) section below will have an effect; other parameters will be ignored.

Returns a `roundRect` instance, an object that inherits from `RoundRect.prototype`

```javascript
let sampleRoundRect = new RoundRect(null, {
	left: 20, right: 20, top: 20, bottom: 20, radius: 16,
	skin: new Skin({ fill: "blue" })
});
application.add(sampleRoundRect);
```

##### `RoundRect.template(anonymous)`

| Arguments | Type | Description
| --- | --- | :--- |
| `anonymous` | `function` | A function that returns an object with properties to initialize the instances that the result creates

Returns a constructor, a function that creates instances of `RoundRect.prototype`. The `prototype` property of the result is `RoundRect.prototype`. The result also provides a `template` function.

```javascript
const blackSkin = new Skin({ fill: "black" });
const whiteSkin = new Skin({ fill: "white" });
let SampleFrame = Container.template($ => ({
	left: 20, right: 20, top: 20, bottom: 20,
	contents: [
		RoundRect($, { left: 0, right: 0, top: 0, bottom: 0, radius: 10, skin: blackSkin }),
		RoundRect($, { left: 2, right: 2, top: 2, bottom: 2, radius: 8, skin: whiteSkin }),
		RoundRect($, {
			left: 8, right: 8, height: 40, bottom: 8, radius: 6, skin: blackSkin,
			corners: RoundRect.bottomLeft | RoundRect.bottomRight
		}),
	]
}));
application.add(new SampleFrame(null));
```

> The example above stacks two `roundRect` objects with different skins, the second inset and with a smaller radius, to draw a rounded border.

<a id="roundrect-dictionary"></a>
#### Dictionary

Same as for `content` object (see [Dictionary](./piu.md#content-dictionary) in the section [Content Object](./piu.md#content-object)), plus:

| Parameter | Type | Description |
| --- | --- | :--- |
| `corners` | `number` | The corners to round, as a combination of `RoundRect.topLeft`, `RoundRect.topRight`, `RoundRect.bottomLeft`, and `RoundRect.bottomRight`. Corners that are not rounded are square. Defaults to all corners.
| `radius` | `number` | The radius of the rounded corners, in pixels. Defaults to `8`.

#### Prototype Description

Prototype inherits from `Content.prototype`.

##### Properties

| Name | Type | Default Value | Read Only | Description |
| --- | --- | --- | --- | :--- |
| `corners` | `number` | `15` | | Which corners of this round rect are round. Setting this property rebuilds the rounded rectangle.
| `radius` | `number` | `8` | | The radius of the rounded corners, in pixels. Setting this property rebuilds the rounded rectangle.

##### Static Properties

| Name | Type | Default Value | Read Only | Description |
| --- | --- | --- | --- | :--- |
| `RoundRect.bottomLeft` | `number` | `4` | ✓ | The bottom left corner
| `RoundRect.bottomRight` | `number` | `8` | ✓ | The bottom right corner
| `RoundRect.topLeft` | `number` | `1` | ✓ | The top left corner
| `RoundRect.topRight` | `number` | `2` | ✓ | The top right corner

### Shape Object

- **Source code:** [`piuShape.c`][4]
- **Relevant Examples:** [clock][15], [figures][16], [shapes][17], [text-clock][18], [text-rotate][19]
- **Manifest:** `$(MODDABLE)/modules/piu/MC/outline`
- **Module:** `piu/shape`

The `shape` object is a `content` object that displays outlines. Outlines are anti-aliased 2D vector graphics, created by filling or stroking a path. See [Outlines](../commodetto/outline/Outlines.md) for how to build paths and how to create, rotate, scale, and translate outlines.

```json
"include": [
	"$(MODDABLE)/examples/manifest_base.json",
	"$(MODDABLE)/examples/manifest_piu.json",
	"$(MODDABLE)/modules/piu/MC/outline"
],
```

The `shape` object uses the fill color and the stroke color of its skin to display a fill outline and a stroke outline. The stroke outline is displayed above the fill outline. If the `shape` object has no skin, it displays nothing.

By default, a `shape` object is empty. Set the `fillOutline` or `strokeOutline` properties to change its fill outline or its stroke outline.

> Usually the same path is used to create the fill outline with `Outline.fill` and the stroke outline with `Outline.stroke`. However, the `shape` object assumes no relationships between the two outlines.

The outline's coordinates are relative to the top left corner of the `shape` object.

The measured width of a `shape` object is the maximum of its fill outline and stroke outline widths. The measured height of a `shape` object is the maximum of its fill outline and stroke outline heights. Consequently, changing the fill outline or the stroke outline of a `shape` object can reflow the layout of its containers.

By default, the outlines are clipped by the bounds of the `shape` object. When the `clip` parameter of the dictionary is `false`,  outlines can extend beyond the bounds of the `shape` object. In that case, changing the outlines invalidates the bounds of the application instead of the bounds of the `shape` object.

The module also provides the `Outline` class as `Shape.Outline`.

#### Constructor Description

##### `Shape([behaviorData, dictionary])`

| Argument | Type | Description |
| --- | --- | :--- |
| `behaviorData` | `*` | A parameter that is passed into the `onCreate` function of this shape's `behavior`. This may be any type of object, including `null` or a dictionary with arbitrary parameters.
| `dictionary` | `object` | An object with properties to initialize the result. Only parameters specified in the [Dictionary](#shape-dictionary) section below will have an effect; other parameters will be ignored.

Returns a `shape` instance, an object that inherits from `Shape.prototype`

```javascript
import {Outline} from "commodetto/outline";

let sampleShape = new Shape(null, {
	width: 120, height: 120,
	skin: new Skin({ fill: "yellow", stroke: "black" })
});
application.add(sampleShape);

const path = new Outline.CanvasPath;
path.arc(60, 60, 50, 0, 2 * Math.PI);
path.closePath();
sampleShape.fillOutline = Outline.fill(path);
sampleShape.strokeOutline = Outline.stroke(path, 4);
```

##### `Shape.template(anonymous)`

| Arguments | Type | Description
| --- | --- | :--- |
| `anonymous` | `function` | A function that returns an object with properties to initialize the instances that the result creates

Returns a constructor, a function that creates instances of `Shape.prototype`. The `prototype` property of the result is `Shape.prototype`. The result also provides a `template` function.

```javascript
class StarBehavior extends Behavior {
	onCreate(shape, points) {
		// a star, centered on the origin
		const path = new Outline.CanvasPath;
		for (let index = 0; index < (points * 2); index++) {
			const angle = index * Math.PI / points;
			const radius = (index & 1) ? 30 : 70;
			const x = radius * Math.sin(angle), y = -radius * Math.cos(angle);
			if (index)
				path.lineTo(x, y);
			else
				path.moveTo(x, y);
		}
		path.closePath();
		shape.fillOutline = Outline.fill(path).translate(80, 80);
		shape.strokeOutline = Outline.stroke(path, 3).translate(80, 80);
	}
}
let StarShape = Shape.template($ => ({
	width: 160, height: 160, skin: new Skin({ fill: "yellow", stroke: "black" }), Behavior: StarBehavior
}));
application.add(new StarShape(5));
```

<a id="shape-dictionary"></a>
#### Dictionary

Same as for `content` object (see [Dictionary](./piu.md#content-dictionary) in the section [Content Object](./piu.md#content-object)), plus:

| Parameter | Type | Description |
| --- | --- | :--- |
| `clip` | `boolean` | If `true`, the outlines are clipped by the bounds of this shape. Defaults to `true`.

#### Prototype Description

Prototype inherits from `Content.prototype`.

##### Properties

| Name | Type | Default Value | Read Only | Description |
| --- | --- | --- | --- | :--- |
| `fillOutline` | `object` | | | The outline object to display with the fill color
| `strokeOutline` | `object` | | | The outline object to display with the stroke color

### SVGImage Object

- **Source code:** [`piuSVGImage.c`][5]
- **Manifest:** `$(MODDABLE)/modules/piu/MC/outline/manifestSVGImage.json`
- **Module:** `piu/SVGImage`

The `svgImage` object is a `content` object that displays vector images. It exists to provide a simulator-compatible way of working with the binary vector image format of Pebble OS: on Pebble OS the `svgImage` object draws such images natively, and this extension draws the same images, with the same programming interface, in the simulator and on other devices.

A vector image is a list of colored shapes, rendered with [outlines](../commodetto/outline/Outlines.md). Unlike the `shape` object, which displays one fill outline and one stroke outline with the colors of its skin, the `svgImage` object displays any number of outlines, each with its own color. The `svgImage` object can rotate, scale, and translate its vector image, and change its opacity.

```json
"include": [
	"$(MODDABLE)/examples/manifest_base.json",
	"$(MODDABLE)/examples/manifest_piu.json",
	"$(MODDABLE)/modules/piu/MC/outline/manifestSVGImage.json"
],
```

The `svgImage` object does not parse SVG files at runtime. Vector images are JavaScript modules that export a description of the vector image by default. Such modules must be defined in the `modules` of your manifest, like the other modules of the application.

```json
"modules": {
	"*": [
		"./main",
		"./hours"
	]
},
```

The module exports an object with `width` and `height` properties and a `shapes` array. Each shape has a `path`, in the format of the paths of the `Outline` class, and `fill` or `stroke` properties, or both.

```javascript
const image = {
	width:14, height:87,
	shapes: [
		{
			path:[
				(new Int32Array([0,-32,-32,1,864,-32,1,800,5536,1,32,5536,48])).buffer,
			],
			fill:{ color:{ r:0, g:0, b:0, a:255 }, rule:0 },
		},
	],
};
export default Object.freeze(image, true);
```

| Property | Type | Description |
| --- | --- | :--- |
| `width` | `number` | The width of the vector image, in pixels
| `height` | `number` | The height of the vector image, in pixels
| `shapes` | `array` | The shapes of the vector image, drawn in order
| `shapes[].path` | `object` | The path of the shape
| `shapes[].fill` | `object` | If present, the path is filled. The `color` property is an object with `r`, `g`, `b`, and `a` properties between 0 and 255. The `rule` property is `Outline.NON_ZERO_RULE` or `Outline.EVEN_ODD_RULE`.
| `shapes[].stroke` | `object` | If present, the path is stroked. The `color` property is an object with `r`, `g`, `b`, and `a` properties between 0 and 255. The `weight`, `linecap`, and `linejoin` properties are the parameters of `Outline.stroke`.
| `frames` | `array` | Instead of the `shapes` property, to define an animated vector image. Each frame is an object with a `shapes` array and a `duration` in milliseconds.

The `svgImage` object loads the vector image when it is bound to the displayed containment hierarchy and unloads the vector image when it is unbound. Loading the vector image creates the outlines and resets the opacity and the transformation.

When an animated vector image is loaded, the `svgImage` object sets its duration to the sum of the durations of the frames. The frame displayed is the frame at the time of the `svgImage` object. Use the `start` and `stop` functions of the `content` object to play and pause the animation.

The measured width and measured height of an `svgImage` object are the width and height of its vector image.

##### Transformation

By default, the center is the center of the vector image, there is no rotation, the scale is `1`, and the translation is `0`.

The transformation is defined by a center, rotation, scale, and translation. The vector image is scaled then rotated around the center, then translated. The center is drawn at its own coordinates relative to the top left corner of the `svgImage` object, so rotating and scaling the vector image doesn't move the center.

By default, the vector image is not clipped by the bounds of the `svgImage` object, so the transformed vector image can extend beyond them. In that case, changing the opacity or the transformation invalidates the bounds of the application instead of the bounds of the `svgImage` object. Set the `clip` parameter of the dictionary to `true` to clip the vector image and limit the invalidation to the bounds of the `svgImage` object.

#### Constructor Description

##### `SVGImage([behaviorData, dictionary])`

| Argument | Type | Description |
| --- | --- | :--- |
| `behaviorData` | `*` | A parameter that is passed into the `onCreate` function of this SVG image's `behavior`. This may be any type of object, including `null` or a dictionary with arbitrary parameters.
| `dictionary` | `object` | An object with properties to initialize the result. Only parameters specified in the [Dictionary](#svgimage-dictionary) section below will have an effect; other parameters will be ignored.

Returns an `svgImage` instance, an object that inherits from `SVGImage.prototype`

```javascript
let sampleSVGImage = new SVGImage(null, {
	left: 0, top: 0, path: "hours.svg"
});
application.add(sampleSVGImage);
```

##### `SVGImage.template(anonymous)`

| Arguments | Type | Description
| --- | --- | :--- |
| `anonymous` | `function` | A function that returns an object with properties to initialize the instances that the result creates

Returns a constructor, a function that creates instances of `SVGImage.prototype`. The `prototype` property of the result is `SVGImage.prototype`. The result also provides a `template` function.

```javascript
class HandBehavior extends Behavior {
	onDisplaying(image) {
		const container = image.container;
		image.cx = 7;
		image.cy = 22;
		image.x = container.x + (container.width >> 1) - image.cx;
		image.y = container.y + (container.height >> 1) - image.cy;
		image.interval = 1000;
		image.start();
	}
	onTimeChanged(image) {
		const date = new Date();
		const fraction = ((date.getHours() % 12) + (date.getMinutes() / 60)) / 12;
		image.r = ((-fraction * 2) - 1) * Math.PI;
	}
}
let HandImage = SVGImage.template($ => ({
	left: 0, top: 0, path: $, Behavior: HandBehavior
}));
application.add(new HandImage("hours.svg"));
```

> This example rotates the hand of a clock around the point of the vector image that is at the center of the container.

<a id="svgimage-dictionary"></a>
#### Dictionary

Same as for `content` object (see [Dictionary](./piu.md#content-dictionary) in the section [Content Object](./piu.md#content-object)), plus:

| Parameter | Type | Description |
| --- | --- | :--- |
| `clip` | `boolean` | If `true`, the vector image is clipped by the bounds of this SVG image. Defaults to `false`.
| `path` | `string` | The specifier of the module that exports the vector image. A `.svg` or `.pdc` extension is replaced by `.js`, so `"hours.svg"` loads the `hours` module.
| `resource` | `object` | The vector image itself, in the format described above. Ignored if the `path` parameter is present.

#### Prototype Description

Prototype inherits from `Content.prototype`.

##### Properties

| Name | Type | Default Value | Read Only | Description |
| --- | --- | --- | --- | :--- |
| `cx` | `number` | | | The horizontal coordinate of the center of the transformation, in pixels. Defaults to half the width of the vector image.
| `cy` | `number` | | | The vertical coordinate of the center of the transformation, in pixels. Defaults to half the height of the vector image.
| `opacity` | `number` | `1` | | The opacity of the vector image, as a number between 0 for transparent and 1 for opaque
| `r` | `number` | `0` | | The rotation angle, in radians
| `s` | `number` | `1` | | The scale. Setting this property sets both the `sx` and `sy` properties. Getting this property returns the average of the `sx` and `sy` properties.
| `sx` | `number` | `1` | | The horizontal scale
| `sy` | `number` | `1` | | The vertical scale
| `tx` | `number` | `0` | | The horizontal translation, in pixels
| `ty` | `number` | `0` | | The vertical translation, in pixels

##### Functions

**`center([x, y])`**

| Argument | Type | Description |
| --- | --- | :--- |
| `x, y` | `number` | The coordinates of the center of the transformation, in pixels

Sets the `cx` and `cy` properties. If `x` or `y` is omitted, the corresponding property is set to the center of the vector image.

```javascript
sampleSVGImage.center(7, 22);
```

***

**`rotate([angle])`**

| Argument | Type | Description |
| --- | --- | :--- |
| `angle` | `number` | The rotation angle, in radians

Sets the `r` property. If `angle` is omitted, the rotation is reset to `0`.

```javascript
sampleSVGImage.rotate(Math.PI / 2);
```

***

**`scale([x, y])`**

| Argument | Type | Description |
| --- | --- | :--- |
| `x` | `number` | The horizontal scale
| `y` | `number` | The vertical scale

Sets the `sx` and `sy` properties. If `y` is omitted, the vertical scale is the same as the horizontal scale. If `x` is omitted, the scale is reset to `1`.

```javascript
sampleSVGImage.scale(0.5);
```

***

**`translate([x, y])`**

| Argument | Type | Description |
| --- | --- | :--- |
| `x` | `number` | The horizontal translation, in pixels
| `y` | `number` | The vertical translation, in pixels

Sets the `tx` and `ty` properties. If `y` is omitted, the vertical translation is the same as the horizontal translation. If `x` is omitted, the translation is reset to `0`.

```javascript
sampleSVGImage.translate(10, 20);
```

***

<!-- Referenced Links -->
[0]: ../../modules/piu/MC/gif/piuGIFImage.c "piuGIFImage.c"
[1]: ../../modules/piu/MC/imageBuffer/piuImageBuffer.c "piuImageBuffer.c"
[2]: ../../modules/piu/MC/qrcode/piuQRCode.c "piuQRCode.c"
[3]: ../../modules/piu/MC/outline/piuRoundRect.c "piuRoundRect.c"
[4]: ../../modules/piu/MC/outline/piuShape.c "piuShape.c"
[5]: ../../modules/piu/MC/outline/piuSVGImage.c "piuSVGImage.c"
[6]: ../../modules/piu/MC/kineticScroller/piuKineticScroller.js "piuKineticScroller.js"

[10]: ../../contributed/first-run-moddable-four "first-run-moddable-four"
[11]: ../../contributed/giphy "giphy"
[12]: ../../examples/io/imagein/camera/camera-test "camera-test"
[13]: ../../examples/io/imagein/camera/camera-balls "camera-balls"
[14]: ../../examples/piu/qrcode "qrcode"
[15]: ../../examples/piu/outline/clock "clock"
[16]: ../../examples/piu/outline/figures "figures"
[17]: ../../examples/piu/outline/shapes "shapes"
[18]: ../../examples/piu/outline/text-clock "text-clock"
[19]: ../../examples/piu/outline/text-rotate "text-rotate"
[20]: ../../examples/piu/gifimage "gifimage"
[21]: ../../examples/piu/roundrect "roundrect"
[22]: ../../examples/piu/imagebuffer "imagebuffer"
[23]: ../../examples/piu/kinetic-scroller "kinetic-scroller"
[24]: ../../contributed/conversationalAI "conversationalAI"
[25]: ../../contributed/somafm "somafm"
[26]: ../../examples/piu/jpeghttp "jpeghttp"
