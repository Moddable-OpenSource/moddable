# Giant Gecko Setup and Build

### Install the Simplicity Studio

Use Gecko SDK v4.4.0.

Use ARM v4.9.3 toolchain.

## Build an app

Building an app is a two step process. First, you will use **mcconfig** to build xs and modules. These will be bundled into an archive **xs_gecko.a**. The archive then needs to be specified in the Project properties in Simplicity Studio. The application will then be built in Simplicity Studio, along with the RTOS and main() driver.


Use the platform **-p gecko** in mcconfig for the Silicon Labs Giant Gecko

	mcconfig -m -p gecko

After building the app, you will need to specify the path for the built archive to the Simplicity Studio project. See below:

#### Import project

xstest project is at **$MODDABLE/build/devices/gecko/SimplicityProject.zip**

Import it into Simplicity Studio.

#### Modify project build settings
Select the project, left-click the project and select **Properties**. Choose **C/C++ Build** -> **Settings** -> **GNU ARM C Compiler** -> **Includes**. Change the paths with /Users/mkellner/moddable to paths that suit your machine.

Fix up the Includes paths to match your system.

	$MODDABLE/xs/includes
	$MODDABLE/xs/sources/gecko
	$MODDABLE/modules/base/instrumentation

Add some Symbols to the C compiler:

	gecko=1
	mxInstrument=0
	MODINSTRUMENTATION=0
	mxDebug=0

Add "m" to Linker Libraries to include the math library.

Add "other objects" to the linker: $MODDABLE/build/bin/gecko/rgb565le/debug/balls/xs\_gecko.a - *the output from mcconfig building any app will ultimately be* "xs\_gecko.a"

Use the hammer icon (or _Project->Build Project_ menu item) to build the project.

Use the bug icon (or _Run->Debug_ menu item) to upload the code to the device and start a debug session. You will have to click the "continue" button to continue past the startup breakpoint.

### xsBug

To build for xsBug, you will need to build your app with the -d option (for debug). Then in the Simplicity Studio project, you will need to change the Project properties:

**C Compiler** -> Symbols -> add

	mxdebug=1


##### Connecting an FTDI for xsBug

Use GND, PC0 (tx), PC1 (rx)

Start **xsBug**

Find the device and use it in the **serial2xsbug** command to connect to xsBug.

	serial2xsbug /dev/cu.usbserial-A104OHLA 115200 8N1

### Connecting ILI9341


![](stk3700-breakoutHeaders.png)

## Notes.   at the top near the search box is a **Tools** button. The Commander is there.

Connect to the J-Link device. Then Connect to the Target device. Then click the **Flash** icon. Then the **Unlock debug access** button.

## Memory

By default, the Giant Gecko is set to use very little memory. There is a lot of memory onboard to work with if you set it up.

In the **GNU ARM Assembler->Symbols** add

	__HEAP_SIZE=36000
	__STACK_SIZE=4096

In **main.c**'s main() function, the routine **xTaskCreate**'s 3rd parameter, set to 4096.

	  xTaskCreate( xsLoop, (const char *)"xsLoop", 4096, NULL, TASK_PRIORITY, NULL);

In **xs/sources/gecko/FreeRTOSConfig.h** set the **configTOTAL\_HEAP\_SIZE** define to match the HEAP\_SIZE above.

	#define configTOTAL_HEAP_SIZE    (( size_t )(36000))

## Interfaces

There are many interfaces on the Giant Gecko device. Many can be mapped to the same pins, so care must be taken to ensure there is no overlap. The existing configuration is as follows:

##### xsBug

	USART0, Loc #5
		PC0 - TX
		PC1 - RX

##### SPI

	USART1, Loc #1
		PD0 - MOSI/TX
		PD1 - MISO/RX
		PD2 - SCK

##### I2C

	I2C1, Loc #1
		PB11 - SDA
		PB12 - SCL

## Sleep levels

To define a different sleep level to use, set the **configSLEEP\_MODE** in **$MODDABLE/xs/sources/gecko/FreeRTOSConfig.h**

	#define configSLEEP_MODE           ( 3 )

Rebuild everything.
