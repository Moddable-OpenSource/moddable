# DESTM32S display driver
Copyright 2017 Moddable Tech, Inc.<BR>
Revised: December 30, 2017

The DESTM32S display controller drives three different ePaper displays. The displays are available from various sources, including Crystalfontz where we purchased our test units. They use a small adapter board (from [good-display.com](http://www.good-display.com/products_detail/productId=327.html)), though the display controller is part of the ePaper display itself (so called chip-on-glass).

- 122 x 250 black and white (available [here](https://www.crystalfontz.com/product/cfap122250a00213-epaper-display-122x250-eink))
- 128 x 296 black, white, and red (available [here](https://www.crystalfontz.com/product/cfap128296d00290-128x296-epaper-display))
- 104 x 212 black, white, gray, and red (available [here](https://www.crystalfontz.com/product/cfap104212b00213-epaper-104x212-eink))

The black and white display uses a different controller from the two displays that support red pixels. The black and white display updates at approximately 4 frames per second, while the displays with red pixels take approximately 20 seconds per frame (e.g. 3 frames per minute).

The ePaper display drivers in the Moddable SDK use the same API as LCD displays. However, because of their relatively slow refresh rate and other update requirements, they are typically used with dedicated applications which are designed to work well with the unique characteristics of the displays.

The [love-e-ink](../../../examples/piu/love-e-ink) example is designed to work with the black and white ePaper display. It demonstrates how to update parts of the display continuously without the need for long full screen refreshes. The [redandblack](../../../examples/drivers/redandblack) example is designed to work with ePaper displays that have red pixels. It cycles through a slide show of images that contain black, white, and red pixels.

### Adding DESTM32 to a project
To add the SSD1351 driver to a project, include its manifest:

	"include": [
		/* other includes here */
		"$(MODULES)/drivers/destm32s/manifest.json",
	],

If using Commodetto or Piu, set the `screen` property of the `config` object in the manifest to `destm32s` to make SSD1351 the default display driver. Since there is no touch input, set the touch driver name to an empty string to disable it.

	"config": {
		"screen": "destm32s",
		"touch": "",
	},

### Pixel format
The DESTM32S driver requires 8-bit gray or, when the display supports color (e.g. red), 8-bit color pixels as input. When building with `mcconfig`, set the pixel format to `gray256` or `rgb332` on the command line:

	mcconfig -m -p esp -r 90 -f gray256
	mcconfig -m -p esp -r 90 -f rgb332

### Defines
In the `defines` object, declare the pixel `width` and `height`. The dimensions of the display specified in the manifest select the display controller that is used, so it is essential to set this correctly.

For 122 x 250 black and white:

	"defines": {
		"destm32s": {
			"width": 122,
			"height": 250,
		}
	}

For 128 x 296 black, white, and red:

	"defines": {
		"destm32s": {
			"width": 128,
			"height": 296,
		}
	}

For 104 x 212 black, white, gray, and red:

	"defines": {
		"destm32s": {
			"width": 104,
			"height": 212,
		}
	}

### Configuring SPI
The `defines` object must contain the `spi_port`, along with the `DC`, `CS`, and `BUSY`. pin numbers. If a `RST` pin is provided, the device will be reset when the constructor is invoked. If the `cs_port`, `dc_port`, `rst_port`, or `busy_port` properties are not provided, they default to NULL. 

	"defines": {
		"ssd1351": {
			/* other properties here */	
			"cs_pin": 4,
			"dc_pin": 2,
			"rst_pin": 0,
			"busy_pin": 5,
			"spi_port": "#HSPI"
		}
	}

The `hz` property, when present, specifies the SPI bus speed. The default value is 500,000 Hz which is near the maximum SPI speed supported by the controllers.
