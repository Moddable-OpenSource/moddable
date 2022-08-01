# DotStar display driver
Copyright 2018 Moddable Tech, Inc.<BR>
Revised: January 3, 2018

The DotStar display controller drives strings of color LED lights. The displays are available from [Adafuit](https://learn.adafruit.com/adafruit-dotstar-leds/overview) in various lengths and configurations

These LEDs are not strictly a display, but they can be seen as one row of a display. There are options to purchase them in a organized in a rectangular grid, which is like a display.

The [dotstar](../../../examples/drivers/dotstar) example works with a 144 LED DotStar string. It scans through an image one row at a time to update the pixels on the DotStar string.

### Adding DotStar to a project
To add the DotStar driver to a project, include its manifest:

	"include": [
		/* other includes here */
		"$(MODULES)/drivers/dotstar/manifest.json",
	],

### Pixel format
The DotStar driver requires 16-bit color (`rgb565le`) pixels.

	mcconfig -m -p esp -f rgb565le

### Defines
In the `defines` object, the optional `brightness` property may be set, where 255 is brightest and 0 is off.

	"defines": {
		"dotstar": {
			"brightness": 64,
		}
	}

### Configuring SPI
The `defines` object must contain the `spi_port`. 

	"defines": {
		"dotstar": {
			/* other properties here */	
			"spi_port": "#HSPI"
		}
	}

The `hz` property, when present, specifies the SPI bus speed. The default value is 20,000,000 Hz which is near the maximum SPI speed supported by the controllers.
