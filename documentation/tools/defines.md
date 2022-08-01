# Using defines in manifests
Copyright 2017 Moddable Tech, Inc.<BR>
Revised: December 11, 2017

## Introduction

The defines block in the manifest creates a set of C language #define preprocessor statements. The defines block is designed to configure the C language implementation of hardware drivers. The defines allow the configuration to occur at build time rather than at runtime. Build time configuration generally results in smaller code and faster execution by allowing unused native code to be removed by conditional compilation in C and linker dead stripping.

This static configuration approach is used instead of a dynamic mechanism which would configure all parameters at runtime. Such an approach is common on Linux systems, but has higher code and time overhead. For microcontroller deployments, a static configuration is optimal and consistent with other aspects of the Moddable SDK which use static configuration (display pixel format and rotation, JavaScript memory heaps, available modules, etc).

### #defines for ILI9341 display driver
The following shows the use of the defines block for the ILI9341 display driver.

	{
		"build": {
			"BUILD": "$(MODDABLE)/build",
			...
		},
		"creation": {
			"static": 32768,
			...
		},
		...
		"defines": {
			"ili9341": {
				"width": 240,
				"height": 320,
				"cs": {
					"port": null,
					"pin": 4
				},
				"dc": {
					"port": null,
					"pin": 2
				},
				"spi": {
					"port": "#HSPI"
				}
			}
		},
		...
	}

The JSON defines configuration parameters of the driver, here width and height, as well as the connections (CS, DC, and SPI). The C #define statements generated are:

	#define MODDEF_ILI9341_WIDTH (240)
	#define MODDEF_ILI9341_HEIGHT (320)
	#define MODDEF_ILI9341_CS_PORT NULL
	#define MODDEF_ILI9341_CS_PIN (4)
	#define MODDEF_ILI9341_DC_PORT NULL
	#define MODDEF_ILI9341_DC_PIN (2)
	#define MODDEF_ILI9341_SPI_PORT "HSPI"

The GPIO pin connections (CS, DC) include both a port name and a pin number. In this example, the host target device does not use the port name, so it is left as null. The SPI port is defined by name, here the string "HSPI" for the ESP8266 "hardware" SPI bus. The "#" prefix on the string indicates that the value in the #define statement should be a quoted string.

### Optional defines
A device driver may support optional #defines. The ILI9341, for example, allows the SPI bus speed to be configured, the horizontal and vertical flip to be enabled, and the Reset pin to be supported.

	"defines": {
		"ili9341": {
			"width": 240,
			"height": 320,
			"hz": 10000000,
			"flipX": true,
			"flipY": false,
			...
			"rst": {
				"port": null,
				"pin": 0
			},
			...

To support option #defines, the driver implementation provides default values and behaviors. The ILI9341 C code implements default values for the hz, flipX, and flipY defines as follows:

	#ifndef MODDEF_ILI9341_HZ
		#define MODDEF_ILI9341_HZ (40000000)
	#endif
	#ifndef MODDEF_ILI9341_FLIPX
		#define MODDEF_ILI9341_FLIPX (false)
	#endif
	#ifndef MODDEF_ILI9341_FLIPY
		#define MODDEF_ILI9341_FLIPY (false)
	#endif

Many deployments do not need to reset the ILI9341 display explicitly, as the automatic reset at power-up is sufficient. The reset pin has a behavior which is only implemented when the reset pin is defined in the manifest. 
	
	#ifdef MODDEF_ILI9341_RST_PIN
		SCREEN_RST_INIT;
		SCREEN_RST_ACTIVE;
		modDelayMilliseconds(10);
		SCREEN_RST_DEACTIVE;
		modDelayMilliseconds(1);
	#endif

The implementations of flipX and flipY use the #define directly allowing the compiler to remove the corresponding code when either value is false.

	data[0] = 0x48;
	if (MODDEF_ILI9341_FLIPX)
		data[0] ^= 0x40;
	if (MODDEF_ILI9341_FLIPY)
		data[0] ^= 0x80;
	ili9341Command(sd, 0x36, data, 1);

The ESP8266 platform does not use a port name for GPIO pins, so the ILI9341 sets the port to NULL when it is not specified:

	#ifndef MODDEF_ILI9341_CS_PORT
		#define MODDEF_ILI9341_CS_PORT NULL
	#endif
	#ifndef MODDEF_ILI9341_DC_PORT
		#define MODDEF_ILI9341_DC_PORT NULL
	#endif

This allows a more concise statement of the pin connections:

	"defines": {
		"ili9341": {
			...
			"cs": {
				"pin": 4
			},
			"dc": {
				"pin": 2
			},
			...
		}
	},

Or simply:

	"defines": {
		"ili9341": {
			...
			"cs_pin": 4,
			"dc_pin": 2,
			...
		}
	},

### Platform overrides
For each driver, the configuration settings (e.g. width, height, flipX, flipY, hz) are typically consistent across all target platforms. The connections, however, are almost always different. The #defines block follows the pattern of mcconfig platform blocks by allowing a platform specific block to add values and override others.

The portion of the ILI9341 configuration shared across all devices could be:

	"defines": {
		"ili9341": {
			"width": 240,
			"height": 320,
			"flipX": true,
			"flipY": true,
		}
	},

Here is the platform defines block ESP8266, Gecko, and Zephyr platforms:

	"platforms": {
		"esp": {
			"modules": {
				...
			},
			...
			"defines": {
				"cs": {
					"pin": 4
				},
				"dc": {
					"pin": 2
				},
				"spi": {
					"port": "#HSPI"
				}
			},
		},
		"gecko": {
			"modules": {
				...
			},
			...
			"defines": {
				"cs": {
					"port": "#gpioPortD",
					"pin": 3
				},
				"dc": {
					"port": "#gpioPortD",
					"pin": 5
				},
				"spi": {
					"port": "#gpioPortD"
				}
			},
		},
		"zephyr": {
			"modules": {
				...
			},
			...
			"defines": {
				"dc": {
					"port": "#GPIO_2",
					"pin": 12
				},
				"spi": {
					"port": "#SPI_0"
				}
			}
		}
	}

### Application overrides
An application can override specific parameters of the defines for a given driver. For example, if a particular device configuration requires a slower speed SPI connection to the display, that can be specified in the application's manifest:

	{
		"include": "../all.json",
		"modules": {
			"*": "./main"
		},
		"defines": {
			"ili9341": {
				"hz": 500000
			}
		}
	}

### Configuration data
The defines block is most often used to define numbers, booleans, and strings. It can also be used to define arrays of numbers, which is useful for more complex configurations, such as device registers.

	"defines": {
		"ili9341": {
			"width": 240,
			"height": 320,
			"registers": [0, 54, 32, 99, 255, 255, 255, 0 65]
		}
	},

The registers property is output as a statically initialized C array:

	#define MODDEF_ILI9341_REGISTERS {0, 54, 32, 99, 255, 255, 255, 0, 65}

This can be used as:

	static const uint8_t gRegisters = MODDEF_ILI9341_REGISTERS;
	int i;

	for (i = 0; i < sizeof(gRegisters); i++)
		;	// gRegisters[i]

Because JSON allows only decimal values, hex and binary values must be converted to decimal values. An alternative is to define the registers property as a string in C language syntax:

		"registers": "{0x00, 0x38, 0x20, 0x63, 0xFF, 0xFF, 0xFF, 0x00 0x41}"

JSON does not allow line breaks in string literals. To allow multiline string literals, an array of strings is converted to a multiline `#define`. Each element of the array is output as a separate line. The following JSON,

		"registers": [
		   "0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,",
		   "0xCF, 3, 0x00, 0xC1, 0X30,",
		   "0xE8, 3, 0x85, 0x00, 0x78"
		]

generates this `#define`

		#define MODDEF_ILI9341_REGISTERS \
		0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02, \
		0xCF, 3, 0x00, 0xC1, 0X30, \
		0xE8, 3, 0x85, 0x00, 0x78

which can be used as follows:

		static const uint8_t gRegisters[] = {
			MODDEF_ILI9341_REGISTERS
		};


### Multiple devices
The manifest can contain #define data for several devices:

	"defines": {
		"ili9341": {
			"width": 240,
			"height": 320,
		},
		"xpt2046": {
			"width": 240,
			"height": 320,
			"hz": 1000000,
		}
	},

## Original idea
The idea and syntax to include #define data in the manifest was suggested by Shotaro Uchida.
