# Pins
Copyright 2017-22 Moddable Tech, Inc.<BR>
Revised: October 20, 2022

## Table of Contents

* [Digital](#digital)
* [Monitor](#monitor)
* [Analog](#analog)
* [PWM](#pwm)
* [I2C](#i2c)
* [SMBus](#smbus)
* [Servo](#servo)
* [RMT](#rmt)
* [SPI](#spi)

<a id="digital"></a>
## class Digital

- **Source code:** [digital](../../modules/pins/digital)
- **Relevant Examples:** [blink](../../examples/pins/blink), [button](../../examples/pins/button)

The `Digital` class provides access to the GPIO pins.

```js
import Digital from "pins/digital";
```

### `constructor(dictionary)`
### `constructor([port], pin, mode)`

The Digital constructor establishes a connection to the GPIO pin specified. There are two ways to configure the pin: by passing a dictionary or by passing separate arguments.

When using a dictionary, the `pin` and `mode` property are required. The port property is optional, with a default value of `NULL`.

Pin numbers and port names are device dependent.

```js
let pin = new Digital({pin: 4, mode: Digital.Input});
```

When providing separate arguments, the  `port` and `pin` arguments specify the pin. If the port is not provided, the default port (`NULL`) is used. The `mode` parameters is passed to the instance's `mode` function to configure the GPIO hardware.

```js
let pin = new Digital(4, Digital.Input);
```

To open a GPIO pin on a specific port, use the Digital constructor with the optional first argument.

```js
let blink = 1;
let led = new Digital("gpioPortName", 5, Digital.Output);
	
Timer.repeat(id => {
	blink = blink ^ 1;
	led.write(blink);
}, 500);
```

The following mode values are available.

```js
Digital.Input
Digital.InputPullUp
Digital.InputPullDown
Digital.InputPullUpDown
Digital.Output
Digital.OutputOpenDrain
```

***

### `static read(pin)`

The `read` function sets the pin to `Digital.Input` mode and samples the value of the specified pin, returning 0 or 1. The default port is used.

The following example configures pin 0 as an input and then tests to see if a button connected to the input is pressed. On an ESP8266 NodeMCU board pin 0 is the built-in user button.

```js
if (Digital.read(0))
	trace("button is pressed\n");
```

***

### `static write(pin)`

The `write` function sets the pin to `Digital.Output` mode and its value to either 0 or 1. The default port is used.

The following example configures pin 5 as an output and then blinks it one per second. On the ESP8266 NodeMCU board, pin 5 is the built-in LED.

```js
let blink = 1;
	
Timer.repeat(id => {
	blink = blink ^ 1;
	Digital.write(5, blink);
}, 500);
```

***

### `read()`

Samples the state of the pin and returns it as 0 or 1.

The static `Digital.read` and `Digital.write` do not allow configuring all pin modes. Use the Digital constructor for full configuration, for example setting the input to use an internal pull-up resistor.

```js
let button = new Digital(0, Digital.InputPullUp);
trace(`button state is ${button.read()}`;
```

***

### `write(value)`

Sets the current value of the pin to 0 or 1.

```js
let led = new Digital(0, Digital.Output);
led.write(1);
```

***

### `mode(mode)`

The mode function sets the mode of the pin. Not all pins support all modes, so refer to the hardware documentation for details.

```js
pin.mode(Digital.Input);
```

***

<a id="monitor"></a>
## class Monitor

- **Source code:** [monitor](../../modules/pins/digital/monitor)
- **Relevant Examples:** [monitor](../../examples/pins/monitor)

The `Monitor` class tracks changes in the state of Digital input pins. An instance is configured to trigger on rising and/or falling edge events. When a trigger event occurs, a callback function is invoked. In addition, the instance maintains a counter of the total number of events triggered.

```js
import Monitor from "pins/digital/monitor";
```

**Note**: For efficiency reasons, implementations may only support a single monitor for each pin. Scripts should only instantiate a single monitor for a given pin.

### `constructor(dictionary)`

The constructor takes a single dictionary argument containing properties to configure the instance. The `Monitor` dictionary is an extension of the `Digital` constructor accepting `pin`, `port`, and `mode` arguments. Only input modes are allowed for a `Monitor` instance  and the mode cannot be changed after instantiation.

```js
let monitor = new Monitor({pin: 0, port: "B", mode: Digital.Input, edge: Monitor.Rising});
```

***

### `onChanged()` callback

A script must install an `onChanged` callback on the instance to be invoked with the specified edge events occur. 

```js
monitor.onChanged = function() {
	trace(`Pin value is ${this.read()}\n`);
}
```

The `onChanged` function is called as soon as possible following the event trigger. This may occur some time after the event occurs, as it may take some time to dispatch to a point where the script can be invoked safely. Because of this delay, it is possible the value of the pin may have again changed. For this reason, scripts should not assume the value of the pin when the callback is invoked but instead call the instances `read` function to retrieve the current value.

***

### `read()`

The `read` function returns the current value of the pin.

```js
let value = this.read();
```

***

### `close()`

The `close` function releases the resources associated with the `Monitor` instance.

```js
monitor.close();
```

The monitor is not eligible to be garbage collected until its `close` function is called.

***

### `rises` and `falls` properties

The `rises` and `falls` properties return the total number of the corresponding trigger events since the instance was created. Events may trigger more quickly than the `onChanged` callback can be invoked, which makes maintaining an accurate count of trigger events using the `onChanged` callback impossible.

```js
let triggers = this.rises + this.falls;
```

***

### Example: Receiving notifications
The following example shows how to receive callbacks on rising and falling edge events.

```js
let monitor = new Monitor({pin: 4, mode: Digital.InputPullUp, edge: Monitor.Rising | Monitor.Falling});
monitor.onChanged = function() {
	trace(`Pin value is ${this.read()}\n`);
}
```

***

<a id="analog"></a>
## class Analog

- **Source code:** [analog](../../modules/pins/analog)
- **Relevant Examples:** [simple-analog](../../examples/pins/simpleAnalog/), [analog](../../examples/pins/analog/)

The `Analog` class provides access to the analog input pins.

```js
import Analog from "pins/analog";
```
	
The Analog class provides only static functions. It is not instantiated.

### `static read(pin)`

The `read` function samples the value of the specified pin, returning a value from 0 to 1023. 

Pin numbers are device dependent:
 - The ESP8266 NodeMCU board has a single analog input pin, analog pin number 0.
 - On the ESP32, use the analog channel number, not the associated GPIO number. You can find GPIO number to analog channel mappings for ESP32 and ESP32-S2 in the [ESP-IDF ADC documentation](https://docs.espressif.com/projects/esp-idf/en/v4.2-beta1/esp32/api-reference/peripherals/adc.html#_CPPv414adc1_channel_t). Only ADC1 is supported on the ESP32.

The following example samples an analog temperature sensor, converting the result to celsius degrees. 

	let temperature = (Analog.read(0) / 1023) * 330 - 50;
	trace(`Temperature is ${temperature} degrees celsius\n`);

The example works with a widely-available low-cost [temperature sensor](https://learn.adafruit.com/tmp36-temperature-sensor/overview).

> Caution: The output voltage range of the TMP36 temperature sensor is 0.1V to 2.0V and the input voltage range of the analog to digital converter on the ESP8266 is 0V to 1.0V. To avoid damaging the ESP8266 a voltage divider should be used to reduce the magnitude of the TMP36 output voltage. The ESP8266 NodeMCU board has a voltage divider for this purpose. Other boards may not have a voltage divider.

***

<a id="pwm"></a>
## class PWM

- **Source code:** [pwm](../../modules/pins/pwm)
- **Relevant Examples:** [tricolor-led](../../examples/pins/tricolor-led/)

The `PWM` class provides access to the PWM output pins.

```js
import PWM from "pins/pwm";
```

### `constructor(dictionary)`

The PWM constructor takes a dictionary which contains the pin number to use. Pin numbers are device dependent. 

The dictionary may optionally contain a `port` property. If the port is not provided, the default port (`NULL`) is used.

```js
let pwm = new PWM({ pin: 12 });
```

***

### `write(value)`

Sets the current value of the pin. Value should be between 0 and 1023.

```js
pwm.write(512);
```

***

### `close()`

The `close` function releases the resources associated with the `PWM` instance.

```js
pwm.close();
```

***

<a id="i2c"></a>
## class I2C

- **Source code:** [i2c](../../modules/pins/i2c)
- **Relevant Examples:** [bmp180](../../examples/drivers/bmp180/), [lis3dh](../../examples/drivers/lis3dh)

The `I2C` class provides access to the I2C bus connected to a pair of pins

```js
import I2C from "pins/i2c";
```

### `constructor(dictionary)`

The I2C constructor takes a dictionary which contains the pin numbers to use for clock and data (`scl` and `sda`, respectively), as well as the I2C address of the target device. Pin numbers are device dependent.

```js
let sensor = new I2C({sda: 5, scl: 4, address: 0x48});
```

The constructor dictionary has an optional `hz` property to specify the speed of the I2C bus when using this instance.

```js
let sensor = new I2C({sda: 5, scl: 4, address: 0x48, hz: 1000000});
```

The constructor dictionary also has an optional `timeout` property to specify the I2C clock stretching timeout in microseonds (μs). Support for this property is device dependent.

```js
let sensor = new I2C({sda: 5, scl: 4, address: 0x48, timeout: 600});
```

Many devices have a default I2C bus. On these devices, the `sda` and `scl` parameters may be omitted from the dictionary to use the default I2C bus.

```js
let sensor = new I2C({address: 0x48});
```

***

### `read(count [, buffer])`

The `read` function reads `count` bytes from the target device, returning them in a `Uint8Array`. The maximum value of `count` is 34.

```js
let bytes = sensor.read(4);
```

An optional buffer parameter is used to provide an `ArrayBuffer` to be used to store the result. Using the same buffer for multiple reads can be more efficient by eliminating memory allocations and running the garbage collector less frequently.

```js
let bytes = new UInt8Array(4);
sensor.read(4, bytes.buffer);
sensor.read(2, bytes.buffer);
sensor.read(3, bytes.buffer);
```

***

### `write(...value [, stop])`

The `write` function writes up to 40 bytes to the target device. The write function accepts multiple arguments, concatenating them together to send to the device. The values may be numbers, which are transmitted as bytes, Arrays, TypedArrays, and strings (which are transmitted as UTF-8 encoded text bytes). 

The `write` function accepts an optional final boolean argument, `stop`, which indicates if a stop condition should be sent when the write is complete. If the final argument is not a boolean value, a stop condition is sent.

```js
let sensor = new I2C({address: 0x48});
sensor.write(0);	
```
***

### Example: Reading an I2C temperature sensor

The following example instantiates an I2C connection to a [temperature sensor](https://www.sparkfun.com/products/11931) with address 0x48 connected to pins 4 and 5. 

```js
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
```
	
***

<a id="smbus"></a>
## class SMBus

- **Source code:** [smbus](../../modules/pins/smbus)
- **Relevant Examples:** [HMC5883L](../../examples/drivers/HMC5883L)

The `SMBus` class implements support for the [System Management Bus](https://en.wikipedia.org/wiki/System_Management_Bus) protocol, which is commonly used with I2C devices. The `SMBus` class extends the `I2C` class with additional functions.

```js
import SMBus from "pins/smbus";
```

### `constructor(dictionary)`

The `SMBus` constructor is identical to the `I2C` constructor.

***

### `readByte(register)`

Reads a single byte of data from the specified register.

***

### `readWord(register)`

Reads a 16-bit data value starting from the specified register. The data is assumed to be transmitted in little-endian byte order.

***

### `readBlock(register, count [, buffer])`

Reads count bytes of data starting from the specified register. Up to 34 bytes of data may be read. The data is returned in a `Uint8Array`. `readBlock` accepts an optional `buffer` argument that behaves the same as the optional `buffer` argument to the `I2C` class `read` function.

***

### `writeByte(register, value)`

Writes a single byte of data to the specified register.

***

### `writeWord(register, value)`

Writes a 16-bit data value starting at the specified register. The value is transmitted in little-endian byte order.

***

### `writeBlock(register, ...value)`

Writes the provided data values starting at the specified register. The value arguments are handled in the same way as the arguments to the `write` function of the `I2C` class.

***


### Example: Initializing a triple axis magnetometer

The following example establishes an SMBus connection to a [triple axis magnetometer](https://www.sparkfun.com/products/10530). Once connected, it checks the device ID to confirm that it is the expected device model. It them puts the device into continuous measure mode.

```js
let sensor = new SMBus({address: 0x1E});
	
let id = sensor.readBlock(10, 3);
id = String.fromCharCode(id[0]) + String.fromCharCode(id[1]) + String.fromCharCode(id[2]);
if ("H43" != id)
	throw new Error("unable to verify magnetometer id");

sensor.writeByte(2, 0);	// continuous measurement mode
```

***

	
<a id="servo"></a>
## class Servo

- **Source code:** [servo](../../modules/pins/servo)
- **Relevant Examples:** [servo](../../examples/pins/servo), [servo-sweep](../../examples/pins/servo-sweep)

The `Servo` class uses digital pins to control servo motors. The API is designed around the Arduino Servo class.

```js
import Servo from "pins/servo";
```
	
### `constructor(dictionary)`

The Servo constructor takes a dictionary. The `pin` property is required, and specifies the digital pin to use. The `min` and `max` properties are optional, and specify the range of the pulse duration in microseconds.

```js
let servo = new Servo({pin: 5, min: 500, max: 2400});
```

***

### `write(degrees)`

The `write` call changes the servo position to the number of degrees specified. Fractional degrees are supported.

The following example instantiates a Servo on pin 4 and rotates it to to 45 degrees.

```js
let servo = new Servo({pin: 4});
servo.write(45);
```

***

### `writeMicroseconds(us)`

The `writeMicroseconds` call sets the duration of the pulse width in microseconds. The value provided is pinned by the `min` and `max` values. 

The Servo implementation pulses a digital signal for a number of microseconds that is proportional to the angle of rotation. Scripts may provide the number of microseconds for the signal pulse instead of degrees, for greater precision.

```js
servo.writeMicroseconds(1000);
```

***

<a id="rmt"></a>
## class RMT

- **Source code:** [rmt](../../modules/pins/rmt)
- **Relevant Examples:** [write](../../examples/pins/rmt/write), [read](../../examples/pins/rmt/read)

The `RMT` class provides access to a RMT (remote control) module. `RMT` is supported on ESP32 and ESP32-S2 microcontrollers.

```js
import RMT from "pins/rmt";
```

### `constructor(dictionary)`

The RMT constructor initializes the RMT module and associates it with the specified pin. 

The RMT constructor takes a dictionary. The `pin` property is required, and specifies the pin to associate with the RMT module. 

```js
let rmt = new RMT({pin: 17});
```

The constructor dictionary also has several optional properties:

- The `channel` property specifies the RMT channel to use with this instance, with a default value of `0`. 
- The `divider` property specifies the clock divider used to generate RMT ticks, with a range of `1` to `255` and a default value of `255`.
- The `direction` property specifies whether to use this RMT as an input or an output. Use `"rx"` for input and `"tx"` for output. The default is output.

```js
let rmt = new RMT({pin: 17, channel: 1, divider: 100});
```

When using the RMT for input, there are additional optional properties in the dictionary:

- The `filter` property configures the RMT module to filter out received pulses shorter than a number of ticks, with a range of `0` to `255` and a default value of `0`.
- The `timeout` property specifies the length of a pulse (in ticks) that will trigger the RMT module to enter its idle mode, with a range of `0` to `65_535` and a default value of `5000`.
- The `ringbufferSize` property configures the size of the RMT module's input buffer in bytes, with a default value of `1024`.

```js
let inputRMT = new RMT({pin: 17, channel: 3, divider: 100, direction: "rx", filter: 100, timeout: 7000, ringbufferSize: 512});
```

***

### `write(firstValue, durations)`

The `write` function transmits alternating pulses of 1s and 0s (i.e. high and low voltages) on the configured pin via the RMT module. The `firstValue` parameter specifies if the first pulse in the sequence will be `1` or `0`. The `durations` parameter must be an Array of integers that specify the duration (in RMT module ticks) of each pulse in the sequence to write to the pin.

The following example pulses the pin high for 2000 ticks then low for 5000 ticks, repeating 3 times in total, then ends with a high pulse of 10000 ticks.

```js
rmt.write(1, [2000, 5000, 2000, 5000, 2000, 5000, 10000]);
```

The write is performed asynchronously, so `write` returns immediately. When the write is complete, the `onWriteable` callback is invoked and another `write` may be issued.

***

### `onWriteable()` callback

A script may install an `onWriteable` callback on the instance to be invoked when the RMT module is ready to write data. 

```js
rmt.onWritable = function() {
	rmt.write(1, [2000, 5000, 2000, 5000, 2000, 5000, 10000]);
}
```

The `onWriteable` function is called once when the RMT module is initialized and ready to receive the first call to `write`. It is then subsequently called each time a write completes and the RMT module is ready to transmit a new sequence.

***

### `read(buffer)`

The `read` function returns RMT data in an ArrayBuffer provided by the `buffer` argument. Up to `buffer.byteLength` bytes from the RMT module's ring buffer are returned in the ArrayBuffer. 

`read` returns an object with three properties: 

- The `buffer` property is the ArrayBuffer provided to `read` as an argument, filled with the received pulse duration sequence read by the RMT module. 
- The `count` property is the total number of pulses received in the sequence. 
- The `phase` property is the logic level of the first item in the sequence (`0` or `1`) — subsequent pulses in the `buffer` alternate logic levels.

```js
const data = new Uint16Array(512);

Timer.repeat(() => {
	let result = inputRMT.read(data.buffer);
	if (result.count) {
		let value = result.phase;
		for (let i = 0; i < result.count; i++) {
			trace(`Pulse of value ${value} for duration ${data[i]}\n`);
			value ^= 1;
		}
	}
}, 20);
```

***

### `close()`

The `close` function releases the resources associated with the `RMT` instance.

```js
rmt.close();
```

***

<a id="spi"></a>
## class SPI

There is no JavaScript API to access SPI at this time.
