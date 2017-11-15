# Pins
Copyright 2017 Moddable Tech, Inc.

Revised: November 7, 2017

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.


## class Digital

The `Digital` class provides access to the GPIO pins.

<!-- 11/7/2017 BSF
The import statement examples throughout this document need to be updated to match the existing pins module organization and export:
	import Digital from "pins/digital";
-->
	import {Digital} from "pins";

Pin numbers are device dependent.
	
The Digital class provides only static functions. It is not instantiated.

### Reading a button

The following example configures pin 0 as an input and then tests to see if a button connected to the input is pressed. On an ESP8266 NodeMCU board pin 0 is the built-in user button.

	Digital.configure(0, 1);		// built in button
	if (Digital.read(0))
		trace("button is pressed\n");

### Blinking an LED

The following example configures pin 5 as an output and then blinks it one per second. On the ESP8266 NodeMCU board, pin 5 is the built-in LED.

	Digital.configure(5, 0);

	let blink = 1;
	Digital.write(5, blink);
	
	Timer.repeat(id => {
		blink = blink ^ 1;
		Digital.write(5, blink);
	}, 500);

### configure(pin, mode)

The `configure` function sets a pin as either an input or output. It should be called before `read` and `write`. Set the mode to 0 for an output, 1 for an input, and 2 for input with pull-up resistor.

### read(pin)

The `read` function samples the value of the specified pin, returning 0 or 1.

### write(pin)

The `write` function sets the value of the specified pin to either 0 or 1.

## class Analog

The `Analog` class provides access to the analog input pins.

	import {Analog} from "pins";

Pin numbers are device dependent.
	
The Analog class provides only static functions. It is not instantiated.

### Reading analog temperature sensor

The following example samples an analog temperature sensor, converting the result to celsius degrees. The ESP8266 NodeMCU board has a single analog input pin, analog pin number 0.

	let temperature = (Analog.read(0) / 1024) * 330;
	trace(`Temperature is ${temperature} degrees celsius\n");

The example works with a widely-available low-cost [temperature sensor](https://learn.adafruit.com/tmp36-temperature-sensor/overview).

### read(pin)

The `read` function samples the value of the specified pin, returning a value from 0 to 1023.

## class PWM

The `PWM` class provides access to the PWM output pins.

	import {PWM} from "pins";

PWM pins are Digital pins configured for output that are toggled between 0 and 1 at a specified frequency.

Pin numbers are device dependent.

The PWM class provides only static functions. It is not instantiated.

### Fan controller

The following example uses two PWM pins to control the direction and speed of a [fan motor](http://www.jameco.com/z/FAN-01-OSEPP-Fan-Motor-Module_2258970.html). The script toggles the fan between clockwise and counterclockwise rotation every three-quarters of a second. The fan motor's two PWMs are connected to Digital pins 4 and 2.

	let fan = +1;
	Digital.configure(4, 0);
	Digital.configure(2, 0);
	Timer.repeat(() => {
		fan = -fan;
		if (fan > 0) {
			PWM.write(4, 384);
			PWM.write(2, 0);
		}
		else {
			PWM.write(2, 384);
			PWM.write(4, 0);
		}
	}, 750);

<!-- 11/7/2017 BSF
Document the optional third "frequency" argument for PWM.write.
-->
### write(pin, value)

The write function sets the frequency of the PWM pin. The value is between 0 and 1023.

## class I2C

The `I2C` class provides access to the I2C bus connected to a pair of pins

	import {I2C} from "pins";

Pin numbers are device dependent.

## Reading an I2C temperature sensor

The following example instantiates an I2C connection to a [temperature sensor](https://www.sparkfun.com/products/11931) with address 0x48 connected to pins 4 and 5. 

	let sensor = new I2C({sda: 5, clock: 4, address: 0x48});
	sensor.write(0);					// set register address 0
	let value = sensor.read(2);	// read two bytes

	// convert data to celsius
	value = (value[0] << 4) | (value[1] >> 4);
	if (value & 0x800) {
		value -= 1;
		value = ~value & 0xFFF;
		value = -value;
	}
	value *= 0.0625;

	trace(`Celsius temperature: ${value}\n`);

<!-- 11/7/2017 BSF
Document the optional "hz" dictionary property.
-->
### constructor(dictionary)

The I2C constructor takes a dictionary which contains the pins numbers to use for clock and data, as well as the I2C address of the target device.

	let sensor = new I2C({sda: 5, clock: 4, address: 0x48});

<!-- 11/7/2017 BSF
Document the optional client provided ArrayBuffer second argument to read().
-->
### read(count)

The `read` function reads `count` bytes from the target device, returning them in an `Array`. The maximum value of `count` is 34.

<!-- 11/7/2017 BSF
Regarding note below, it looks like read() now always returns the client ArrayBuffer or an Uint8Array.
-->
> **Note**: In the future, an `ArrayBuffer` may be optionally returned.

### write(...value)

The `write` function writes up to 34 bytes to the target device. The write function accepts multiple arguments, concatenating them together to send to the device. The values may be numbers, which are transmitted as bytes, and strings (which are transmitted as UTF-8 bytes). 

<!-- 11/7/2017 BSF
Regarding note below, it looks write does now support arrays for the values.
-->
> **Note**: In the future, `ArrayBuffers` may also be used as values.

## class SMBus

<!-- 11/7/2017 BSF
"is extends" -> "extends" below
-->
The `SMBus` class implements support for the [System Management Bus](https://www.bing.com/search?q=smbus&form=APMCS1&PC=APMC) protocol, which is commonly used with I2C devices. The SMBus class is extends the `I2C` class with additional functions.

	import SMBus from "smbus";

The `SMBus` constructor is identical to the `I2C` constructor.

### Initializing a triple axis magnetometer

The following example establishes an SMBus connection to a [triple axis magnetometer](https://www.sparkfun.com/products/10530). Once connected, it checks the device ID to confirm that it is the expected device model. It them puts the device into continuous measure mode.

	let sensor = new SMBus({sda: 5, clock: 4, address: 0x1E});
	
	let id = sensor.readBlockDataSMB(10, 3);
	id = String.fromCharCode(id[0]) + String.fromCharCode(id[1]) + String.fromCharCode(id[2]);
	if ("H43" != id)
		throw new Error("unable to verify magnetometer id");

	sensor.writeByteDataSMB(2, 0);	// continuous measurement mode

### readByteDataSMB(register)

Reads a single byte of data from the specified register.

### readWordDataSMB(register)

Reads a 16-bit data value starting from the specified register. The data is assumed to be transmitted in little-endian byte order.

<!-- 11/7/2017 BSF
Document the optional client provided buffer argument for readBlockDataSMB. Also the data is returned in the client buffer or Uint8Array.
-->
### readBlockDataSMB(register, count)

Reads count bytes of data starting from the specified register. Up to 34 bytes of data may be read. The data is returned in an `Array`.

### writeByteDataSMB(register, value)

Writes a single byte of data to the specified register.

### writeWordDataSMB(register, value)

Writes a 16-bit data value starting at the specified register. The value is transmitted in little-endian byte order.

### writeBlockDataSMB(register, ...value)

Writes the provided data values starting at the specified register. The value arguments are handled in the same way as the arguments to the `write` function of the `I2C` class.

<!-- 11/7/2017 BSF
The N.B. comment regarding in SMBus.js needs to be updated.
-->

<!-- 11/7/2017 BSF
The pins directory also contains the SPI implementation, but that is a native code interface. Probably should be added here if/when we ever provide a JS wrapper class.
-->
