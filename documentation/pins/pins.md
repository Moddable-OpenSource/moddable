# Pins
Copyright 2017-18 Moddable Tech, Inc.

Revised: February 23, 2018

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.


## class Digital

The `Digital` class provides access to the GPIO pins.

	import Digital from "pins/digital";

Pin numbers and port names are device dependent.

### Reading a button

The following example configures pin 0 as an input and then tests to see if a button connected to the input is pressed. On an ESP8266 NodeMCU board pin 0 is the built-in user button.

	if (Digital.read(0))
		trace("button is pressed\n");

### Reading a button with a built-in pull up resistor

The static `Digital.read` and `Digital.write` do not allow configuring all pin modes. Use the Digital constructor for full configuration, for example setting the input to use an internal pull-up resistor.

	let button = new Digital(0, Digital.InputPullUp);
	trace(`button state is ${button.read()}`;

### Blinking an LED

The following example configures pin 5 as an output and then blinks it one per second. On the ESP8266 NodeMCU board, pin 5 is the built-in LED.

	let blink = 1;
	
	Timer.repeat(id => {
		blink = blink ^ 1;
		Digital.write(5, blink);
	}, 500);

### Blinking an LED on a specific port

To open a GPIO pin on a specific port, use the Digital constructor with the optional first argument.

	let blink = 1;
	let led = new Digital("gpioPortName", 5, Digital.Output);
	
	Timer.repeat(id => {
		blink = blink ^ 1;
		led.write(blink);
	}, 500);

### static read(pin)

The `read` function sets the pin to `Digital.Input` mode and samples the value of the specified pin, returning 0 or 1. The default port is used.

### static write(pin)

The `write` function sets the pin to `Digital.Output` mode and its value to either 0 or 1. The default port is used.

### constructor(dictionary)
### constructor([port], pin, mode)

The Digital constructor establishes a connection to the GPIO pin specified. There are two ways to configure the pin, by passing a dictionary or by passing separate arguments.

When using a dictionary, the `pin` and `mode` property are required. The port property is optional, with a default value of `NULL`.

	let pin = new Digital({pin: 4, mode: Digital.Input});

When providing separate arguments, the  `port` and `pin` arguments specify the pin. If the port is not provided, the default port (`NULL`) is used. The `mode` parameters is passed to the instance's `mode` function to configure the GPIO hardware.

	let pin = new Digital(4, Digital.Input);

### mode(mode)

The mode function sets the mode of the pin. Not all pins support all modes, so refer to the hardware documentation for details. The following mode values are available.

	Digital.Input
	Digital.InputPullUp
	Digital.InputPullDown
	Digital.InputPullUpDown
	Digital.Output
	Digital.OutputOpenDrain

### read()

Samples the state of the pin and returns it as 0 or 1.

### write(value)

Sets the current value of the pin to 0 or 1.

## class Monitor

The `Monitor` class tracks changes in the state of Digital input pins. An instance is configured to trigger on rising and/or falling edge events. When a trigger event occurs, a callback function is invoked. In addition, the instance maintains a counter of the total number of events triggered.

	import Monitor from "pins/digital/monitor";

**Note**: For efficiency reasons, implementations may only support a single monitor for each pin. Scripts should only instantiate a single monitor for a given pin.

### Receiving notifications
The following example shows how to receive callbacks on rising and falling edge events.

	let monitor = new Monitor({pin: 4, mode: Digital.InputPullUp,
						edges: Monitor.Rising | Monitor.Falling});
	monitor.onChanged = function() {
		trace(`Pin value is ${this.read()}\n`);
	}

### constructor(dictionary)
The constructor takes a single dictionary argument containing properties to configure the instance. The `Monitor` dictionary is an extension of the `Digital` constructor accepting `pin`, `port`, and `mode` arguments. Only input modes are allowed for a `Monitor` instance  and the mode cannot be changed after instantiation.

	let monitor = new Monitor({pin: 0, port: "B", mode: Digital.Input,
							edges: Monitor.Rising});

### close()
The `close` function releases the resources associated with the `Monitor` instance.

	monitor.close();

The monitor is not eligible to be garbage collected until its `close` function is called.

### onChanged() callback
A script must install an `onChanged` callback on the instance to be invoked with the specified edge events occur. 

	monitor.onChanged = function() {
		trace(`Pin value is ${this.read()}\n`);
	}

The `onChanged` function is called as soon as possible following the event trigger. This may occur some time after the event occurs, as it may take some time to dispatch to a point where the script can be invoked safely. Because of this delay, it is possible the value of the pin may have again changed. For this reason, scripts should not assume the value of the pin when the callback is invoked but instead call the instances `read` function to retrieve the current value.

### read()
The `read` function returns the current value of the pin.

	let value = this.read();

### `rises` and `falls` properties
The `rises` and `falls` properties return the total number of the corresponding trigger events since the instance was created. Events may trigger more quickly than the `onChanged` callback can be invoked, which makes maintaining an accurate count of trigger events using the `onChanged` callback impossible.

	let triggers = this.rises + this.falls;

## class Analog

The `Analog` class provides access to the analog input pins.

	import Analog from "pins/analog";

Pin numbers are device dependent.
	
The Analog class provides only static functions. It is not instantiated.

### Reading analog temperature sensor

The following example samples an analog temperature sensor, converting the result to celsius degrees. The ESP8266 NodeMCU board has a single analog input pin, analog pin number 0.

	let temperature = (Analog.read(0) / 1023) * 330 - 50;
	trace(`Temperature is ${temperature} degrees celsius\n`);

The example works with a widely-available low-cost [temperature sensor](https://learn.adafruit.com/tmp36-temperature-sensor/overview).

Caution: The output voltage range of the TMP36 temperature sensor is 0.1V to 2.0V and the input voltage range of the analog to digital converter on the ESP8266 is 0V to 1.0V. To avoid damaging the ESP8266 a voltage divider should be used to reduce the magnitude of the TMP36 output voltage. The ESP8266 NodeMCU board has a voltage divider for this purpose. Other boards may not have a voltage divider.

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

	let sensor = new I2C({sda: 5, scl: 4, address: 0x48});
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

The I2C constructor takes a dictionary which contains the pins numbers to use for clock and data (`scl` and `sda` respectively), as well as the I2C address of the target device.

	let sensor = new I2C({sda: 5, scl: 4, address: 0x48});

The constructor dictionary has an optional `hz` property to specify the speed of the I2C bus when using this instance.

	let sensor = new I2C({sda: 5, scl: 4, address: 0x48, hz: 1000000});

Many devices have a default I2C bus. On these devices, the `sda` and `scl` parameters may be omitted from the dictionary to use the default I2C bus.

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

## class Servo

The `Servo` class uses digital pins to control servo motor. The API is designed around the Arduino Servo class.

	import Servo from "pins/servo";

### Setting servo by degrees

The following example instantiates a Servo on pin 4 and rotates it to to 45 degrees.

	let servo = new Servo({pin: 4});
	servo.write(45);

### Setting servo by microseconds

The Servo implementation pulses a digital signal for a number of microseconds that is proportional to the angle of rotation. Scripts may provide the number of microseconds for the signal pulse instead of degrees, for greater precision.

	servo.writeMicroseconds(1000);

### constructor(dictionary)

The Servo constructor takes a dictionary. The `pin` property is required, and specifies the digital pin to use. The `min` and `max` properties are optional, and specify the range of the pulse duration in microseconds.

### write(degrees)

The `write` call changes the servo position to the number of degrees specified. Fractional degrees are supported.

### writeMicroseconds(us)

The `writeMicroseconds` call sets the duration of the pulse width in microseconds. The value provided is pinned by the `min` and `max` values. 

## class SPI

There is no JavaScript API to access SPI at this time.
