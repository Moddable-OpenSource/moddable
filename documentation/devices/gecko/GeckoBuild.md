# Gecko

There are currently three flavors of Gecko implemented in the Moddable SDK.

**gecko/giant** is for the EFM32GG STK3700

![Giant Gecko](https://siliconlabs-h.assetsadobe.com/is/image//content/dam/siliconlabs/images/products/microcontrollers/32-bit_mcus/giant_gecko/giant-gecko-starter-kit.jpg)

[Giant Gecko Starter Kit](https://www.silabs.com/products/development-tools/mcu/32-bit/efm32-giant-gecko-starter-kit)

**gecko/mighty** is for the Mighty Gecko

![Mighty Gecko](https://siliconlabs-h.assetsadobe.com/is/image//content/dam/siliconlabs/images/products/Bluetooth/zigbee-and-thread/efr32-mighty-gecko-starter-kit.png)

[EFR32 Mighty Gecko Starter Kit](https://www.silabs.com/products/development-tools/wireless/mesh-networking/mighty-gecko-starter-kit)

**gecko/thunderboard2** is for the Thunderboard Sense 2

![Thunderboard Sense 2](https://siliconlabs-h.assetsadobe.com/is/image//content/dam/siliconlabs/images/products/Bluetooth/thunderboard/thunderboard-sense-2-angled.png)

[Thunderboard™ Sense 2 IoT Development Kit](https://www.silabs.com/products/development-tools/thunderboard/thunderboard-sense-two-kit)

### Install Simplicity Studio

To get started, install [Simplicity Studio](https://www.silabs.com/products/development-tools/software/simplicity-studio). MacOS v.4.2 is currently supported.

Plug in your board when launching for the first time so that Simplicity Studio will update with the correct SDKs. Install 32 bit MCU, Flex and Bluetooth.

As of this writing, the current versions are:
	
	32-bit MCU SDK - 5.3.5.0
	Flex SDK - 2.1.0.0
	Bluetooth SDK - 2.7.0.0

[build note] If the SDK version changes, or you wish to use a specific SDK version, you can change the **SDK_BASE** build define in $MODDABLE/tools/mcconfig/gecko/make.*subplatform*.mk.


Install a sample project for your device or board.

	Giant Gecko: STK3700_powertest
	Mighty Gecko: simple_rail_without_hal
	Thunderboard Sense 2: soc-thunderboard-sense-2, soc-empty

Build, install and run the sample to become familiar with the process.


### Get Moddable Open Source

Follow the macOS [Host environment setup](../../Moddable SDK – Getting Started.md) section of the Getting Started guide.

#### Modifications to the Simplicity Studio project for the Moddable sdk.

You will need to change some settings in the Simplicity Studio project for the application you are building.

Open the properties window for the project and select *C/C++ Build->Settings*.

1) In the **Other Objects** section, click the _add document_ icon ![Add Document](AddDoc.png) to add the Moddable archive to your project.

![Add Archive](AddArchive.png)

2) Select "File system..."

3) Navigate to your moddable directory and select the **xs_gecko.a** archive file.

It is located in $MODDABLE/build/bin/gecko/_platform_/debug/_application_/xs_gecko.a


#### Integrate Moddable runtime

In the app's main.c, add a few things:

	int gResetCause = 0;
	uint32_t wakeupPin = 0;
	
	void assertEFM() { }

In the app's main() function after chip initialization, enable some clocks:

	  CMU_ClockEnable(cmuClock_CORELE, true);
	  CMU_ClockEnable(cmuClock_HFPER, true);
	  CMU_ClockEnable(cmuClock_GPIO, true);

Add some code to get the reset cause:

	  gResetCause = RMU_ResetCauseGet();
	  if (gResetCause & (RMU_RSTCAUSE_EM4RST | RMU_RSTCAUSE_SYSREQRST | RMU_RSTCAUSE_EXTRST)) {
		  wakeupPin = GPIO_EM4GetPinWakeupCause();
	  }
	  RMU_ResetCauseClear();

Do other setup for your device/application as necessary.

Add **xs_setup()** which configures the xs runtime and debugger:

	xs_setup();

In your main loop, call xs_loop(); repeatedly.

	while(1) {
		xs_loop();
	}



#### Heap and stack size

Depending on the processor and project configuration, you may need to adjust the stack size and heap size of the project.

GNU ARM C Compiler -> Symbols
and
GNU ARM Assembler -> Symbols

	__STACK_SIZE=0x1000
	__HEAP_SIZE=0xA000


### Radio
As a base application, use **RAIL: Simple TRX**.

By default, the packet size is 16 bytes. You can increase that by changing the Radio Profile. When Simplicity Studio creates the ISC file, select the **Radio Configuration** tab and change the *Select a radio PHY for selected profile* to **Custom Settings**.

![ISC Custom Settings](ISC-RadioConfig.png)

Scroll down to **Profile options->Packet** and select the **Frame Fixed Length** tab. Change the payload size to the size you need.

![Frame Fixed Length](FrameLength.png)

#### Radio main.c

The changes to main.c are a bit more involved than the simple examples above.
Copy the file /path/to/modified/main.c into your simple_trx project, replacing the existing main.c


## DEBUG
#### gecko_giant
Using UART1

                "debugger": {
                    "tx" : { "pin": "9", "port": "gpioPortB" },
                    "rx" : { "pin": "10", "port": "gpioPortB" },
                    "port": "UART1",
                    "clock": "cmuClock_UART1",
                    "location": "2",
                },
                
Using USART0

                "debugger": {
                    "tx" : { "pin": "0", "port": "gpioPortC" },
                    "rx" : { "pin": "1", "port": "gpioPortC" },
                    "port": "USART0",
                    "clock": "cmuClock_USART0",
                    "location": "5",
                },

	
#### gecko_mighty
Location 1

                "debugger": {
                    "tx": { "pin": "11", "port": "gpioPortB" },
                    "rx": { "pin": "12", "port": "gpioPortB" },
                    "port": "USART1",
                    "clock": "cmuClock_USART1",
                    "location": "19",
                },

Location 2

                "debugger": {
                    "tx": { "pin": "6", "port": "gpioPortB" },
                    "rx": { "pin": "7", "port": "gpioPortB" },
                    "port": "USART3",
                    "clock": "cmuClock_USART3",
                    "location": "10",
                },

#### gecko_thunderboard
Location 1

                "debugger": {
                    "tx": { "pin": "11", "port": "gpioPortD" },
                    "rx": { "pin": "12", "port": "gpioPortD" },
                    "port": "USART1",
                    "clock": "cmuClock_USART1",
                    "location": "19",
                },

Location 2

                "debugger": {
                    "tx": { "pin": "6", "port": "gpioPortB" },
                    "rx": { "pin": "7", "port": "gpioPortB" },
                    "port": "USART3",
                    "clock": "cmuClock_USART3",
                    "location": "10",
                },


## SLEEP
Retention Memory

Retention GPIO

Sleep wakeup port/pin

Idle sleep level (EM1/EM2/EM3)

## Analog

- Ports to read from (0-?)
- How to define input pins

		"analog": {
			"port": "0",
			"ref": "adcRefVDD",
			"input1": "adcPosSelAPORT2XCH9",
			"input2": "adcPosSelAPORT3YCH22",
		},

## SPI

- Individual devices will include their own CS pin
- explain port and location

                    "mosi": { "pin": "0", "port": "gpioPortD" },
                    "miso": { "pin": "1", "port": "gpioPortD" },
                    "sck": { "pin": "2", "port": "gpioPortD" },
                    "port": "1",
                    "location": "1",

## I2C

                "i2c": {
                    "port": "0",
                    "sda": { "pin": 11, "port": "gpioPortC", "location": 16 },
                    "scl": { "pin": 10, "port": "gpioPortC", "location": 14 },
                },


## Digital

Monitor:

on gecko, interrupts by gpio are defined by pin and port is ignored.
