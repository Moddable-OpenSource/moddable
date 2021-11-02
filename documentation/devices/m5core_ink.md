# Moddable SDK support for M5Core Ink
Updated November 2, 2021

The Moddable SDK Piu examples that do not depend on touch generally seem to work as-is.

## Setup, build, run

There are several example applications in the Moddable SDK that show how to take make best use of the M5Core Ink and its See the [ePaper blog](https://blog.moddable.com/blog/epaper#examples) post for details.

Just `cd` to the directory of the example and build as usual:

```
mcconfig -d -m -p esp32/m5core_ink
```
The display refresh rate is about 3 FPS.

## Porting Status

The following are implemented and working:

- EPD display driver (GDEW0154M09)
- RTC (PCF8563 / BM8563)
- Up / Down / Middle / Power / External buttons 
- LED
- Buzzer
- Battery voltage

## Display Driver

The display driver is a [Poco `PixelsOut`](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/commodetto/commodetto.md#pixelsout-class) implementation. This allows it to use both the [Poco](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/commodetto/poco.md) graphics APIs and[ Piu](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md) user interface framework from the Moddable SDK.

The display driver is written entirely in JavaScript. It uses [Ecma-419 IO](https://419.ecma-international.org/#-9-io-class-pattern) APIs for all hardware access. Performance is excellent, often faster than the EPD class built into the native M5Core Ink library. 

The display driver implements dithering, which allows many levels of gray to be displayed using only black and white pixels. The default dithering algorithm is the venerable [Atkinson dither](https://twitter.com/phoddie/status/1274054345969950720). To change to Burkes or to disable dithering:

```js
screen.dither = "burkes";
screen.dither = "none"
```

Dithering is performed using the new `commodetto/Dither` module.

To support dithering the driver requires that Poco render at 8-bit gray (256 Gray). The driver also only supports full screen updates as partial updates with dithering show seams. Partial updates could, and probably should, be supported as they could be useful when not using dithering.

The driver maintains a back buffer, which is more-or-less necessary because of the way the display controller works. Fortunately the buffer is just 5000 bytes, since it can use 1-bit pixels.

The display driver does a full screen refresh on the first draw after instantiation. To disable this, set `refresh` to false.

```
screen.configure({refresh: false});
```

The display driver uses partial updates after the first frame. To force a full screen update: 

```
screen.configure({refresh: true});
```

A partial update may be performed on power-up to avoid an initial full screen flash. To do this, the previous frame must first be redrawn to put it back into the controller's memory. To do that, first draw the previous frame with `previous` set to true, then draw the next frame as usual. The driver resets the value of `previous` to `false` after one frame is drawn.

```
screen.configure({previous: true, refresh: false});
// draw previous
// draw next

```

Hardware rotation is not supported. It could be, but given that the display is square it isn't obviously useful. Both Poco and Piu support software rotation at build time, so rotation is available if needed, just not at runtime.

## Buttons

All of the buttons are available with callbacks when changed. Here's a basic test that installs callbacks on each.

```js
for (let name in device.peripheral.button) {
	new device.peripheral.button[name]({
		onPush() {
			trace(`Button ${name}: ${this.pressed}\n`);
		}
	})
}
```

## LED

The green LED at the top of the unit is available:

```js
const led = new device.peripheral.led.Default;
led.on = 1;		// full strength
led.on = 0;		// off
led.on = 0.5;		// half strength
```

## Buzzer

The buzzer is implemented to play tones. As with the M5 Speaker API, sounds are played immediately.

Instantiate the buzzer:

```js
const buzzer = new device.peripheral.tone.Default;
```

Play a note by name and octave:

```js
buzzer.note("C", 4);
```

Play a note by name and octave for a fixed duration in milliseconds:

```js
buzzer.note("Bb", 4, 500);
```

Play a tone by frequency:

```js
buzzer.tone(262);
```

Play a tone by frequency for a fixed duration in milliseconds:

```js
buzzer.tone(262, 500);
```

Mute the buzzer (it automatically unmutes on the next note or tone played):

```js
buzzer.mute();
```

Close the buzzer:

```js
buzzer.close();
```

## Battery Voltage

To get the battery voltage:

```js
const battery = new device.peripheral.battery.Default;
const voltage = battery.read();
```

## Battery Powered Operation

To operate M5Core Ink on battery power, the power hold line must be set to 1. This is donee by default in the `setup/target` module.

```
power.main.write(1);
```

To turn the device off when running on battery power, set the power hold line to 0.


```
power.main.write(0);
```
