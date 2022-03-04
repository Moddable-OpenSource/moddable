# Moddable SDK - Piu Examples

Copyright 2021 Moddable Tech, Inc.<BR>
Revised: November 4, 2021

These examples demonstrate how to use features of the [Piu user interface framework](../../documentation/piu/piu.md). Piu is an object-based framework that makes it easier to create complex, responsive layouts.

Most of the examples are designed for a QVGA (320x240) screen, but many feature responsive layouts that work on a variety of screen sizes. All of the examples in this folder run on the desktop simulator with the exception of the `backlight`, `epaper-travel-time`, `one-line`, and `one-line-keyboard` examples.

This document provides a brief description of each example and a preview of each app running on the desktop simulator. If you are looking for an example that demonstrates how to use a specific feature, see the list below. Keep in mind that this list provides only a few recommendations and is not a complete list of examples that use each feature.

- **Images:** <a href="#images">`images`</a>, <a href="#balls">`balls`</a>, <a href="#neon-light">`neon-light`</a>
- **Text:** <a href="#text">`text`</a>, <a href="#cards">`cards`</a>, <a href="#localization">`localization`</a>
- **Animation:** <a href="#easing-equations">`easing-equations`</a>, <a href="#transitions">`transitions`</a>, <a href="#timeline">`timeline`</a>
- **Touch input:** <a href="#drag">`drag`</a>, <a href="#keyboard">`keyboard`</a>, <a href="#map-puzzle">`map-puzzle`</a>
- **Scrolling content:** <a href="#scroller">`scroller`</a>, <a href="#list">`list`</a>
- **Networking:** <a href="#wifi-config">`wifi-config`</a>, <a href="#weather-and-mini-weather">`weather` and `mini-weather`</a>, <a href="#one-line-and-one-line-keyboard">`one-line` and `one-line-keyboard`</a>

***

### `backlight`

The `backlight` example allows you to adjust the backlight brightness on Moddable Two. 

> For more information about the backlight, see the **Backlight** section of the [Moddable Two documentation](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/moddable-two.md#backlight).

***

### `balls`

![](http://www.moddable.com/assets/piu-gifs/balls.gif)

The `balls` example shows a full frame rate animation of bouncing balls on the screen. It is not designed for a specific screen size so it is useful for testing displays.

***

### `bars`

![](http://www.moddable.com/assets/piu-gifs/bars.gif)

The `bars` example renders black and white images and uses the Piu `Port` object to draw bars. It also shows how to use the `Timeline` and `Transition` objects, which are two distinct ways of creating animations.

***

### `cards`

![](http://www.moddable.com/assets/piu-gifs/cards.gif)

The `cards` example uses the Piu `Timeline` object to animate Moddable employees' business cards.

***

<!--
### `clut`

***
-->

### `color-picker`

![](http://www.moddable.com/assets/piu-gifs/color-picker.gif)

The `color-picker` example provides a simple user interface for selecting a color. Drag the picker around the colored image to select a color. The selected color is displayed in the header bar.

> For more information about the color picker implementation, see our blog post [A Color Picker for Microcontrollers](https://blog.moddable.com/blog/colorpicker/).

***

### `countdown`

![](http://www.moddable.com/assets/piu-gifs/countdown.gif)

The `countdown` example counts down to a date specified in the code.  The numbers and text subtly fade between different colors.

***

### `drag` and `drag-color`

![](http://www.moddable.com/assets/piu-gifs/drag.gif) ![](http://www.moddable.com/assets/piu-gifs/drag-color.gif)

The `drag` and `drag-color` examples are great for testing touch on displays. You can move the objects by touching them and dragging across the screen. Multitouch is supported.

***

### `easing-equations`

![](http://www.moddable.com/assets/piu-gifs/easing-equations.gif)

The `easing-equations` example demonstrates the use of the easing equations built into the Moddable SDK along with the Piu `Timeline` object for creating animations. Easing equations are useful for creating smooth, natural-looking animations.

***

### `epaper-flashcards`

<img src="https://www.moddable.com/assets/piu-gifs/epaper-flashcards" width=250>

The `epaper-flashcards` example shows a sequence of flash cards. The answer is displayed when tapped. A swipe left or swipe right moves to the previous or next card.

> For more information about this example and ePaper displays, see our blog post [Getting the Most from ePaper Displays](https://blog.moddable.com/blog/epaper).

***

### `epaper-photos`

<img src="http://www.moddable.com/assets/piu-gifs/epaper-photos" width=180>

The `epaper-photos` example displays a slideshow of photographs.

> For more information about this example and ePaper displays, see our blog post [Getting the Most from ePaper Displays](https://blog.moddable.com/blog/epaper).

***

### `epaper-travel-time`

The `epaper-epaper-travel-time` example displays the travel time between your home and work. It uses the Google Maps Web API to determine the current travel time.

> For more information about this example and ePaper displays, see our blog post [Getting the Most from ePaper Displays](https://blog.moddable.com/blog/epaper).

***

### `hardware-rotation`

<img src="https://www.moddable.com/assets/piu-gifs/hardware-rotation.png" width=180>

The `hardware-rotation` example rotates the image and text on the display every three seconds. This example is only compatible with display controllers that support hardware rotation.

> For more information about hardware rotation, see our blog post [Run-time Display Rotation](https://blog.moddable.com/blog/rotate/).

***

### `heartrate`

![](http://www.moddable.com/assets/piu-gifs/heartrate.gif)

The `heartrate` example generates a random number each second and displays it as part of a sample UI for a heartrate monitor.

***

### `horizontal-expanding-keyboard`

![](http://www.moddable.com/assets/piu-gifs/horizontal-expanding-keyboard.gif)

The `horizontal-expanding-keyboard` example demonstrates the use of the expanding keyboard module to create an on-screen keyboard for a touch screen. The horizontal expanding keyboard is designed to make touch input easier on 320x240 displays..

*See also the `keyboard` and `vertical-expanding-keyboard` examples.*

> For more information about the expanding keyboard, see our blog post [Introducing an Expanding Keyboard for Small Screens](https://blog.moddable.com/blog/expanding-keyboard/).

***

### `images`

![](http://www.moddable.com/assets/piu-gifs/images.gif)

The `images` example demonstrates how to render GIFs, JPEGs, and PNGs.

***

### `keyboard`

![](http://www.moddable.com/assets/piu-gifs/keyboard.gif)

The `keyboard` example demonstrates the use of the keyboard module to create an on-screen keyboard for a touch screen. The keyboard module may be used on many screen sizes.

*See also the `horizontal-expanding-keyboard` and `vertical-expanding-keyboard` examples.*

***

### `list`

![](http://www.moddable.com/assets/piu-gifs/list.gif)

The `list` example uses a Piu `Port` object to create a scrolling list of items that may be tapped. 

***

### `localization`

![](http://www.moddable.com/assets/piu-gifs/localization.gif)

The `localization` example translates text on the screen to the language selected. You can read more about how localization is implemented in the Moddable SDK [here](../..//documentation/piu/localization.md).

***

### `love-e-ink`

![](http://www.moddable.com/assets/piu-gifs/love-e-ink.gif)

The `love-e-ink` example is designed for the 128x296 Crystalfontz ePaper display. It updates small portions of the screen at a time and displays the message "Moddable ♥ Eink."

***

### `love-js`

![](http://www.moddable.com/assets/piu-gifs/love-js.gif)

The `love-js` example is designed for a 128x128 pixel screen. It plays an animation of three different images to display the message "Moddable ♥ JavaScript."

***

### `map-puzzle`

![](http://www.moddable.com/assets/piu-gifs/map-puzzle.gif)

The `map-puzzle` example implements a simple puzzle for a touch screen. Move pieces of the puzzle by touching them while dragging across the screen.

***

### `neon-light`

![](http://www.moddable.com/assets/piu-gifs/neon-light.gif)

The `neon-light` example displays English and Japanese text over a colorful, animated background.

***

### `one-line` and `one-line-keyboard`


![](http://www.moddable.com/assets/piu-gifs/one-line.gif)  ![](http://www.moddable.com/assets/piu-gifs/one-line-keyboard.gif)

The `one-line` and `one-line-keyboard` examples are designed to work together. The `one-line` example opens a WebSocket server and displays its IP address on screen. The `one-line-keyboard` example opens a WebSocket client that connects to the `one-line` server and sends user-inputted text for it to display.

***

### `outline/clock`

<img src="http://www.moddable.com/assets/piu-gifs/outline-clock.png" width=200>

The `outline/clock` example renders an analog clock using Canvas outlines.

***

### `outline/figures`

<img src="http://www.moddable.com/assets/piu-gifs/outline-figures.png" width=200>

The `outline/figures` example contains mods to draw all of the figures from the Outline documentation.

***

### `outline/shapes`

<img src="http://www.moddable.com/assets/piu-gifs/outline-shapes.png" width=160>

The `outline/shapes` example renders four different outline shapes in a manner similar to the balls example.

***

### `preferences`

<img src="https://www.moddable.com/assets/piu-gifs/preferences.png" width=190>

The `preferences` example demonstrates how to set preferences that are saved across boot. Tap a color to change the background color and set a background color preference. When the device is rebooted, the background color will be the last color selected.

***

### `scroller`

![](http://www.moddable.com/assets/piu-gifs/scroller.gif)

The `scroller` example shows how to create vertical and horizontal scrolling content. Tap the title bar to toggle between the two directions. 

***

### `sound`

<img src="http://www.moddable.com/assets/piu-gifs/sound.png" width=190>

The `sound` example demonstrates how to use the Piu `Sound` object. Tap the play button to play a sound. Tap the volume buttons to adjust the volume.

***

### `spinner`

![](http://www.moddable.com/assets/piu-gifs/spinner.gif)

The `spinner` example implements a simple spinning animation, useful for loading screens.

***

### `spiral`

![](http://www.moddable.com/assets/piu-gifs/spiral.gif)

The `spiral` example uses a Piu `Port` object to draw spirals of random sizes on the screen.

***

### `text`

![](http://www.moddable.com/assets/piu-gifs/text.gif)

The `text` example uses the Piu `Text` object to render a variety of text styles with different sizes, colors, and alignments.

***

### `timeline`

![](http://www.moddable.com/assets/piu-gifs/timeline.gif)

The `timeline` example demonstrates the use of the `to`, `from`, and `on` functions of the Piu `Timeline` object. Tap an option to see an animation that uses the corresponding function.

***

### `transitions`

![](http://www.moddable.com/assets/piu-gifs/transitions.gif)

The `transitions example demonstrates the use of the Piu comb and wipe transitions. These are useful for creating full screen transitions.

***

### `vertical-expanding-keyboard`

![](http://www.moddable.com/assets/piu-gifs/vertical-expanding-keyboard.gif)

The `vertical-expanding-keyboard` example demonstrates the use of the expanding keyboard module to create an on-screen keyboard for a touch screen. The vertical expanding keyboard is designed to make touch input easier on 240x320 displays.

*See also the `keyboard` and `horizontal-expanding-keyboard` examples.*

> For more information about the expanding keyboard, see our blog post [Introducing an Expanding Keyboard for Small Screens](https://blog.moddable.com/blog/expanding-keyboard/).

***


### `weather` and `mini-weather`

![](http://www.moddable.com/assets/piu-gifs/weather.gif)

The `weather` and `mini-weather` examples display the weather forecast for five different cities. Forecasts are retrieved by sending HTTP requests to a cloud service.

***

### `wifi-config`

![](http://www.moddable.com/assets/piu-gifs/wifi-config.gif)

The `wifi-config` example allows the user to configure the Wi-Fi network by selecting from a list of available networks. The on-screen keyboard is used to enter the password for secure networks.

***