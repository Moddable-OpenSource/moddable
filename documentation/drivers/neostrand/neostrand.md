# NeoStrand driver

The NeoStrand driver is a subclass of the [NeoPixel](http://blog.moddable.com/blog/neopixels/) driver to add an effects engine for dynamic color displays with strings or strips of NeoPixel (WS2811/WS2812-like) LEDs.

Terminology:

* A **strand** is a strip of NeoPixels.
* An **effect** is a group of settings and an action function which makes a change to state of the NeoPixel strip or takes some other action.
* A **scheme** is a set of effects that are applied to the strip.

## Adding NeoStrand to a Project

To add the NeoStrand functionality to a project, add the path to the module in your **manifest.json**.

	"modules": {
		"*": [
			/* other modules here */
			"$(MODDABLE)/modules/drivers/neostrand/*",
       ],
    },

### Using NeoStrand

Import the NeoStrand class:

```js
import NeoStrand from "neostrand";
```

NeoStrand is subclass of NeoPixel, so we use the same constructor dictionary to set the length, pin connected to the Data signal on the LED strip, and order of the pixels.

```js
const strand = new NeoStrand({length: 50, pin: 22, order: "RGB"});
```

### Schemes

A scheme is a set of effects to apply to a strand. A scheme has one or more effects applied in the specified order. Setting the new scheme takes effect immediately, even if the strand is already running.

```js
let schemes = [];

let marqueeDictionary = { strand };

schemes.push( [ new NeoStrand.Marquee( marqueeDictionary ) ]);

strand.setScheme(schemes[0]);
strand.start(50);
```

The parameter sent to `start` defines the delay between updates of the effects. It is the number of milliseconds between calls to the effect function. Reducing the number may cause some effects to render more smoothly at the expense of CPU load. If it is set too high, long strands may not fully update or update incorrectly.

### Effect Dictionary

Effect parameters are specified in a dictionary.

```js
let marqueeDictionary = { strand, start: 0, end: (strand.length / 2) };
```
The dictionary must contain the strand. `start` and `end` define a span of pixels to be influenced by the effect. Use `Object.assign` to copy the dictionary if you want to make effect variants.

```js
let marqee2 = Object.assign({}, marqueeDictionary,
		{ start: (strand.length / 2) + 1, end: strand.length, reverse: 1 });
```

A scheme can contain more than one effect.

```js
schemes.push( [ new NeoStrand.Marquee( marqueeDictionary ),
                new NeoStrand.Marquee( marquee2 ) ]) );
```

### Control

You can start and stop the scheme at any time. The strand state will remain. You may want to set all LEDs to 0 after stopping the scheme.

```js
strand.stop();
strand.fill(strand.makeRGB(0, 0, 0));
strand.update();
```

### Effects

Effects are what make up a scheme. They usually cause one or more of the LEDs to change state over time. However, since the effect action function is JavaScript, you can do just about anything - trigger GPIOs, play a sound, fire a cannon.

Effects are subclassed from `NeoStrandEffect` and take a dictionary of configuration parameters. The configuration dictionary _must_ contain the `strand` to which this effect will be applied.

```js
    let baseDictionary = { strand, start: 0, end: strand.length, loop: 1 };
```
There are a number of common parameters that are used in almost every effect. Effects may also specify their own parameters.

A [list of built-in effects](#builtin) can be found later in this document.

#### Common Effect Parameters

| Parameter | default | range | description |
|---|---|---|---|
strand | (none) | | The NeoStrand to which this effect applied
start | 0 | [0..strand.length] | index of first pixel of effect
end | strand.length | [0..strand.length] | index of last pixel of effect
duration | 1000 | | (ms) duration of one cycle of the effect
reverse | 0 | [0, 1] | reverse effect 
loop | 1 | [0, 1] | loop the effect 

`start` and `end` define the span of pixels that this effect will operate on. `duration` is the length of one cycle of the effect.

When one cycle of the effect is complete, the effect will restart if the `loop` parameter is set to `1`.

If loop is `0` the effect's `onComplete()` function will be called.

`reverse` will reverse the effect, starting at the end state, and finishing with the begin state.

### Custom Effects

An effect is a subclassed from the **NeoStrandEffect** class. The constructor sets parameters from the supplied dictionary. The `activate` method sets up a [`timeline`](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md#timeline-object). In the example below, the `effectValue` varys from 0 to `effect.dur` and is called when the value changes.

```js
class MyEffect extends NeoStrandEffect {
    constructor(dictionary) {
        super(dictionary);
        this.name = "MyEffect"
}

activate(effect) {
    effect.timeline.on(effect, { effectValue: [ 0, effect.dur ] }, effect.dur, null, 0);
    effect.reset(effect);
}

set effectValue(value) {
    for (let i=this.start; i<this.end; i++) {
        if (0 == (i % this.size))
            this.color = this.strand.makeRGB(Math.random() * this.max, Math.random() * this.max, Math.random() * this.max);
            this.strand.set(i, this.color, this.start, this.end);
        }
    }
}
```

Use the custom effect by creating an instance with the desired parameters and including it in an array sent to the `changeScheme` function:

```js
strand.setScheme( [ new MyEffect(strand) ] );
```

<a id="builtin"></a>
## Built-in Effects

The following effects are built-in:

* [Marquee](#marquee)
* [Hue Span](#huespan)
* [Sine](#sine)
* [Pulse](#pulse)
* [Pattern](#pattern)
* [Dim](#dim)


<a id="marquee"></a>
### Marquee

The `Marquee` effect is the common ant-march lighting effect.

A `sizeA` and `sizeB` parameters define the pattern. For example { sizeA: 1, sizeB: 3 } will produce the repeating pattern:  [ A, B, B, B, A, B, B, B, A ... ]

| Parameter | default | range | description |
|---|---|---|---|
strand | (none) | | The NeoStrand to which this effect applied
start | 0 | [0..strand.length] | index of first pixel of effect
end | strand.length | [0..strand.length] | index of last pixel of effect
reverse | 0 | [0, 1] | reverse effect 
loop | 1 | [0, 1] | loop the effect 
duration | 1000 | | (ms) time of one complete cycle of the pattern
sizeA | 1 | [1..strand.length] | number of pixels in the A part of pattern
sizeB | 3 | [1..strand.length] | number of pixels in the B part of pattern
rgbA | { r: 0, g: 0, b: 0x13 } | | RGB A color elements
rgbB | { r:0xff, g:0xff, b:0xff } | | RGB B color elements


<a id="huespan"></a>
### Hue Span

The `HueSpan` effect is a smooth fade between colors. Like a color-wheel, it cycles through the hue in HSV color space.


| Parameter | default | range | description |
|---|---|---|---|
strand | (none) | | The NeoStrand to which this effect is applied
start | 0 | [0..strand.length] | index of first pixel of effect
end | strand.length | [0..strand.length] | index of last pixel of effect
size | \<strand.length\> | [0..strand.length] | length of one hue cycle (in pixels)
reverse | 0 | [0, 1] | reverse effect 
loop | 1 | [0, 1] | loop the effect
duration | 1000 | | (ms) time of one complete color wheel cycle
speed | 1.0 | | speed multiplier
position | 0 | [0..1] | starting HSV hue position
saturation | 1.0 | [0..1] | HSV saturation
value | 1.0 | [0..1] | HSV value


<a id="sine"></a>
### Sine

The `Sine` effect varys a color component in a sine wave fashion.

`value` = y = (sin(_t_) + pos) * amplitude

| Parameter | default | range | description |
|---|---|---|---|
strand | (none) | | The NeoStrand to which this effect is applied
start | 0 | [0..strand.length] | index of first pixel of effect
end | strand.length | [0..strand.length] | index of last pixel of effect
amplitude | 1 | [0..1] | amount of change (y axis multiplier)
size | \<strand.length\> | [1..strand.length] | length of one cycle (0-2π radians) (in pixels)
reverse | 0 | [0, 1] | reverse effect 
loop | 1 | [0, 1] | loop the effect 
duration | 1000 | | (ms) time of one complete sine cycle
speed | 1.0 | | speed multiplier
loop | 1 | [0, 1] | loop the effect
position | 0 | [0..1] | starting x position
vary | "b" | ["r","g","b",<br>"h","s","v"] | color component to vary
value | 1.0 | [0..1] | HSV value



<a id="pulse"></a>
### Pulse

The `Pulse` effect sets `size` number of pixels at a location and then moves it toward either (or both) ends. The `mode` parameter specifies whether to add, subtract or set the pixels' RGB values.

| Parameter | default | range | description |
|---|---|---|---|
strand | (none) | | The NeoStrand to which this effect is applied
start | 0 | [0..strand.length] | index of first pixel of effect
end | strand.length | [0..strand.length] | index of last pixel of effect
size | 3 | [1..strand.length] | (pixels) size of a pulse
reverse | 0 | [0, 1] | reverse effect 
loop | 1 | [0, 1] | loop the effect 
direction | 0 | [-1, 0, 1] | direction of effect 
position | "random" | [-strand.length..strand.length] | index of the pulse starting pixel<br>negative numbers are off-strand and okay<br>"random" picks a random starting location
duration | 3000 | | (ms) time of one pulse cycle
mode | 1 | [-1, 0, 1] | **1** to add, **-1** to subtract, or **0** set the pixel color
fade | 0.2 | [0..1] | how quickly tails fade
rgb | { r: 0x80, g: 0x80, b: 0x80 } | | RGB color elements


<a id="pattern"></a>
### Pattern

The `Pattern` sets a fixed RGB pattern of pixels.

| Parameter | default | range | description |
|---|---|---|---|
strand | (none) | | The NeoStrand to which this effect is applied
start | 0 | [0..strand.length] | index of first pixel of effect
end | strand.length | [0..strand.length] | index of last pixel of effect
pattern | [ 0x130000, 0x131313 ] | | array of RGB colors
mode | 1 | [-1, 0, 1] | **1** to add, **-1** to subtract, or **0** set the pixel color

<a id="dim"></a>
### Dim

The `Dim` effect reduces the color for each pixel in the span over over the `duration`. For example, if a pixel is fully on (ie. 0xFFFFFF) it will gradually decrease to fully off when `duration` ms elapses.

| Parameter | default | range | description |
|---|---|---|---|
strand | (none) | | The NeoStrand to which this effect is applied
start | 0 | [0..strand.length] | index of first pixel of effect
end | strand.length | [0..strand.length] | index of last pixel of effect
duration | 1000 | | (ms) time to reduce a full-brightness pixel to off


<!--
<a id="ease"></a>
### Ease

The `Ease` effect uses the easing equations to move a pixel.

A `size` and `sizeB` parameters define the pattern. For example { sizeA: 1, sizeB: 3 } will produce the repeating pattern:  [ A, B, B, B, A, B, B, B, A ... ]

| Parameter | default | range | description |
|---|---|---|---|
strand | (none) | | The NeoStrand to which this effect is applied
start | (none) | [0..strand.length] | index of the first pixel of effect
end | (none) | [0..strand.length] | index of the last pixel of effect
size | 3 | [1..strand.length] | (pixels) size of a pulse
reverse | 0 | [0, 1] | reverse effect 
loop | 1 | [0, 1] | loop the effect
direction | 0 | [-1, 1] | direction of effect 
easing | null | | easing equation to use, null uses a linear easing function
rgb | { r: 0x80, g: 0x80, b: 0x80 } | | RGB color elements


<a id="bounce"></a>
### Bounce

The `Bounce` effect bounces a small segment (ball) of pixels back and forth, erasing (setting to black) the pixel in the front and back of the ball.

| Parameter | default | description |
|---|---|---|
strand | (none) | The NeoStrand to which this effect is applied
start | (none) | index of the first pixel of effect
end | (none) | index of the last pixel of effect
size | 3 | number of pixels of the 'ball'
first | \<start\> | index of the pulse starting pixel<br>negative numbers are off-strand and okay<br>"random" picks a random starting location
direction | 1 | starting direction of travel (1 or -1)
speed | 1.0 | number of pixels traveled per iteration
r, g, b | 0xff (white) | RGB color elements


<a id="sparkle"></a>
### Sparkle

The `Sparkle` effect applies a varying color value to a small segment of pixels. The pixels lighten and darken the color while gradually fading.

| Parameter | default | description |
|---|---|---|
strand | (none) | The NeoStrand to which this effect is applied
start | (none) | index of the first pixel of effect
end | (none) | index of the last pixel of effect
size | 5 | number of pixels of the sparkle
range | 100 | [0-255] how quickly the pixels fade (random from 0-range)
decay | 15 | [0-255] how quickly the overall sparkle fades
next | 500 | time (ms) between new sparkle locations
r, g, b | 0xff (white) | RGB color elements


<a id="fire"></a>
### Fire

The `Fire` effect. Adapted from [Fire2012](https://blog.kriegsman.org/2014/04/04/fire2012-an-open-source-fire-simulation-for-arduino-and-leds/).

Fire is affected by three parameters. The first `cooling` controls how fast the flame cools down. More cooling means shorter flames. The recommended values are between 20 and 100.

The second `sparking` controls the chance (out of 255) that a spark will ignite. A higher value makes the fire more active. The recommended values are between 50 and 200.

The third `delay` controls the speed of fire activity. A higher value makes the flame flicker more slowly. The recommended values are between 0 and 20.


| Parameter | default | description |
|---|---|---|
strand | (none) | The NeoStrand to which this effect is applied
start | (none) | index of the first pixel of effect
end | (none) | index of the last pixel of effect
direction | 1 | starting direction of travel (1 or -1)
cooling | 55 | [0-255] number of pixels of the sparkle
sparking | 120 | [0-255] number of pixels of the sparkle
delay | 15 | time (ms) between actions

-->

## Making a custom effect

To make your own effect, pick an existing one that most closely matches your vision and go from there.

Let's make a random color effect. It will set `size` pixels to a random color, change colors every `duration` ms.

<a id="Random"></a>

| Parameter | default | range | description |
|---|---|---|---|
strand | (none) | | The NeoStrand to which this effect is applied
start | 0 | [0..strand.length] | index of first pixel of effect
end | strand.length | [0..strand.length] | index of last pixel of effect
duration | 1000 | | (ms) time between color changes
size | 5 | [0..strand.length] | size of each color
max | 127 | [0..255] | maximium value of random RGB component

Using *Pattern* as a starting point, we'll change the class name and constructor, set up the timeline in `activate` and provide a setter for the changing `effectValue`. The `loopPrepare` function will be called before a looping effect starts or restarts.
	

```js
class RandomColor extends NeoStrandEffect {
    constructor(dictionary) {
        super(dictionary);
        this.name = "RandomColor"
        this.size = dictionary.size ? dictionary.size : 5;
        this.max = dictionary.max ? dictionary.max : 127;
        this.loop = 1;			// force loop
    }

    loopPrepare(effect) { effect.colors_set = 0; }

    activate(effect) {
        effect.timeline.on(effect, { effectValue: [ 0, effect.dur ] }, effect.dur, null, 0);
        effect.reset(effect);
    }

    set effectValue(value) {
        if (0 == this.colors_set) {
            for (let i=this.start; i<this.end; i++) {
                if (0 == (i % this.size))
                    this.color = this.strand.makeRGB(Math.random() * this.max, Math.random() * this.max, Math.random() * this.max);
                this.strand.set(i, this.color, this.start, this.end);
            }
            this.colors_set = 1;
        }
    }
}

```

Then create and add the effect your scheme list and give it a try.

```js
	schemes.push( [ new RandomColor( {  strand } ) ]);
```
