# Pins
Copyright 2017 Moddable Tech, Inc.

Revised: November 15, 2017

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.


## class Digital

The `Digital` class provides access to the GPIO pins.

	import Digital from "pins/digital";

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

	import Analog from "pins/analog";

Pin numbers are device dependent.
	
The Analog class provides only static functions. It is not instantiated.

### Reading analog temperature sensor

The following example samples an analog temperature sensor, converting the result to celsius degrees. The ESP8266 NodeMCU board has a single analog input pin, analog pin number 0.

	let temperature = (Analog.read(0) / 1024) * 330;
	trace(`Temperature is ${temperature} degrees celsius\n`);

The example works with a widely-available low-cost [temperature sensor](https://learn.adafruit.com/tmp36-temperature-sensor/overview).

### read(pin)

The `read` function samples the value of the specified pin, returning a value from 0 to 1023.

## class PWM

The `PWM` class provides access to the PWM output pins.

	import PWM from "pins/pwm";

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

### write(pin, value [, frequency])

The write function sets the value of the PWM pin. The value is between 0 and 1023. The frequency defaults to 1 KHz and is be changed with optional frequency parameter.

## class I2C

The `I2C` class provides access to the I2C bus connected to a pair of pins

	import I2C from "pins/i2c";

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

### constructor(dictionary)

The I2C constructor takes a dictionary which contains the pins numbers to use for clock and data, as well as the I2C address of the target device.

	let sensor = new I2C({sda: 5, clock: 4, address: 0x48});

The constructor dictionary has an optional `hz` property to specify the speed of the I2C bus when using this instance.

	let sensor = new I2C({sda: 5, clock: 4, address: 0x48, hz: 1000000});

Many devices have a default I2C bus. On these devices, the sda and clock parameters may be omitted from the dictionary to use the default I2C bus.

	let sensor = new I2C({address: 0x48});

### read(count [, buffer])

The `read` function reads `count` bytes from the target device, returning them in a `Uint8Array`. The maximum value of `count` is 34.

	let bytes = sensor.read(4);

An optional buffer parameter is used to provide an `ArrayBuffer` to be used to store the result. Using the same buffer for multiple reads can be more efficient by eliminating memory allocations and running the garbage collector less frequently.

	let bytes = new UInt8Array(4);
	sensor.read(4, bytes.buffer);
	sensor.read(2, bytes.buffer);
	sensor.read(3, bytes.buffer);

### write(...value)

The `write` function writes up to 34 bytes to the target device. The write function accepts multiple arguments, concatenating them together to send to the device. The values may be numbers, which are transmitted as bytes, Arrays, TypedArrays, and strings (which are transmitted as UTF-8 encoded text bytes). 

## class SMBus

The `SMBus` class implements support for the [System Management Bus](https://en.wikipedia.org/wiki/System_Management_Bus) protocol, which is commonly used with I2C devices. The `SMBus` class extends the `I2C` class with additional functions.

	import SMBus from "pins/smbus";

The `SMBus` constructor is identical to the `I2C` constructor.

### Initializing a triple axis magnetometer

The following example establishes an SMBus connection to a [triple axis magnetometer](https://www.sparkfun.com/products/10530). Once connected, it checks the device ID to confirm that it is the expected device model. It them puts the device into continuous measure mode.

	let sensor = new SMBus({address: 0x1E});
	
	let id = sensor.readBlock(10, 3);
	id = String.fromCharCode(id[0]) + String.fromCharCode(id[1]) + String.fromCharCode(id[2]);
	if ("H43" != id)
		throw new Error("unable to verify magnetometer id");

	sensor.writeByte(2, 0);	// continuous measurement mode

### readByte(register)

Reads a single byte of data from the specified register.

### readWord(register)

Reads a 16-bit data value starting from the specified register. The data is assumed to be transmitted in little-endian byte order.

### readBlock(register, count [, buffer])

Reads count bytes of data starting from the specified register. Up to 34 bytes of data may be read. The data is returned in a `Uint8Array`. `readBlock` accepts an optional `buffer` argument that behaves the same as the optional `buffer` argument to the `I2C` class `read` function.

### writeByte(register, value)

Writes a single byte of data to the specified register.

### writeWord(register, value)

Writes a 16-bit data value starting at the specified register. The value is transmitted in little-endian byte order.

### writeBlock(register, ...value)

Writes the provided data values starting at the specified register. The value arguments are handled in the same way as the arguments to the `write` function of the `I2C` class.


## class SPI

There is no JavaScript API to access SPI at this time.
