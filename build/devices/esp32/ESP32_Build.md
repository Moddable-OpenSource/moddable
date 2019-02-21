# ESP32 Setup and Build

Note: this is preliminary. I hope to get the build consolidated to remove the split xs library / esp main builds.

## ESP Dev Tools Setup

The github for esp-idf has a Readme with links to setting up the development environment and a sample project.

[Do that.](https://github.com/espressif/esp-idf)

I made a directory at my **HOME** directory called esp32 and put the items in there.

There should be a **~/esp32/esp-idf** and **~/exp32/xtensa-esp32-elf**.

Set the IDF_PATH variable and put the compiler in your path.

	# export IDF_PATH=$HOME/esp32/esp-idf
	# export PATH=$HOME/esp32/xtensa-esp32-elf/bin:$PATH
	

## ESP Hardware setup

The USB port is used for flashing and viewing console messages. **printf()** works and will output to the console.

On your Mac, in a console, look for the device name that the Mac has assigned to your ESP32.

	# ls /dev/cu.*
	
You will see something like:

	/dev/cu.Bluetooth-Incoming-Port    /dev/cu.usbserial-DN02MPJK
	/dev/cu.SLAB_USBtoUART

In **$MODDABLE/build/devices/esp32/xsProj/sdkconfig** change this line to match your device:

	CONFIG_ESPTOOLPY_PORT="/dev/cu.usbserial-DN027YYS"

You may also need to change the path to python.

	# which python
	/usr/bin/python

Then in **$MODDABLE/build/devices/esp32/xsProj/sdkconfig** change

	CONFIG_PYTHON="python"
	
To make the changes to **sdkconfig** take place, run **make** in the xsProj direcotry.

	# cd $MODDABLE/build/devices/esp32/xsProj
	# make
	

To enable **xsbug**, you'll need a 3.3v FTDI or serial->USB connector.

	 esp32 === FTDI
	   GND <-> GND
	   p16 <-> TX
	   p17 <-> RX

The ILI9341 is connected as follows:

	   esp32 === ILI9341
		3.3v <-> VCC
		 GND <-> GND
		 g15 <-> CS
		 g0  <-> RESET
		 g2  <-> D/C
		 g13 <-> SDI
		 g14 <-> SCK
		3.3v <-> LED

## xs / piu

### helloworld

To test your setup, build the **helloworld** test app.

	# cd $MODDABLE/examples/helloworld
	# mcconfig -d -m -p esp32

This will build the xs archive for compiling into the esp32 application.

Build the esp32 application.

	# cd $MODDABLE/build/devices/esp32/xsProj
	# make

Upload the esp32 app and watch the output.

	# make flash
	# make monitor

You can do both at once.

	# make flash monitor

### balls

Build balls.

	# cd $MODDABLE/examples/piu/balls
	# mcconfig -d -m -p esp32

Modify the esp32 build script to find the **balls** archive instead of **helloworld**.

In **$MODDABLE/build/devices/esp32/xsProj/main/component.mk** change the **APP_NAME** to **balls**.

<mark>NOTE</mark>: if you change the application, you will need to delete the esp32 build target to ensure the new xs archive is linked.

	You can do either:
		# cd $MODDABLE/build/devices/esp32/xsProj
		# rm build/xs_esp32.*
	or
		# cd $MODDABLE/build/devices/esp32/xsProj
		# make clean
		
	The former is faster.

Build the esp32 application.

	# cd $MODDABLE/build/devices/esp32/xsProj
	# make

Upload the esp32 app and watch the output.

	# make flash monitor
	
There should be bouncing stuff on the screen.


# xsbug

When running **xsbug** you will need to start the serial2xsbug to make the connection. The cu.usbserial will be different than the one used to flash the device.

	serial2xsbug /dev/cu.usbserial-AH03IMK2 115200 8N1 build/xs_esp32.elf

You may need to hit the reset button on the esp32 after it flashes if xsbug doesn't connect.

# Notes

gpio, spi and ili9341 are in **build/devices/esp32**

	
