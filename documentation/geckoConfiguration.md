Some notes on gecko and configuration.

In the manifest **platforms**, **defines** section there are defines for the various peripherals to specify what pins, ports, clocks, irq, etc. to use.

	"retention": false,

**retention** refers to retention memory, a small block of memory that is retained when the gecko is in EM4 hibernate mode to enable passing of state and other data between restarts of the software. Retention memory requires running the real-time clock and consumes power. Turn it off if unneeded to save power.

The **spi**, **i2c** and **analog** sections specify pins, ports, location and other configuration for those interfaces:

	"spi": {
		"mosi": { "pin": 6, "port": "gpioPortA", },
		"miso": { "pin": 7, "port": "gpioPortA", },
		"sck": { "pin": 8, "port": "gpioPortA", },
		"cs": { "pin": 9, "port": "gpioPortA", },
		"port": "2",
		"location": "1",
	},

The **spi** section defines the specific GPIO pins to be used for MOSI, MISO, SCK (clock) and a default CS (chip select). It also specifies which USART to use (port) and the pin location for the USART port.

	"i2c": {
		"port": "0",
		"sda": { "pin": 11, "port": "gpioPortC", "location": 16 },
		"scl": { "pin": 10, "port": "gpioPortC", "location": 14 },
	},
	
The **i2c** section defines the specfic GPIO pins to be used for SDA and SCL, as well as which I2C device to use (port).

	"analog": {
		"port": "0",
		"power": { "pin": 9, "port": "gpioPortC" },
		"ref": "adcRefVDD",
		"input1": "adcPosSelAPORT3YCH23",
	},

The **analog** section defines the ADC device to use (port), what GPIO pin used to power the analog device, what voltage reference to use, and what specific channel to use for the ADC device (input#).

----
Some modules also have configuration sections:

	"destm32s": {
		"width": 122,
		"height": 250,
		"hz": 40000000,
		"cs": { "pin": 9, "port": "gpioPortA", },
		"dc": { "pin": 9, "port": "gpioPortD", },
		"rst": { "pin": 8, "port": "gpioPortD", },
		"busy": { "pin": 10, "port": "gpioPortD", },
		"spi_port": "2",
	},

**width** and **height** specify the size of the screen.<br>
**hz** specifies the SPI speed.<br>
**spi_port** defines the USART port that this module will use. <br>
For the GPIO pins to drive this display:<br>
**cs** (chip select)<br>
**dc** (data/command)<br>
**rst** (reset)<br>
**busy** (device is busy)<br>

The **destm32s** device can be instantiated in code like:

	let render = new Poco(new DESTM32S({clear: false, powerOff: true}), {rotation: 90, pixels: 256});

In this case, the **clear** option set to false prevents the screen from clearing to all white on instantiation. **powerOff** set to true causes the device to power down after drawing a frame (for power savings).