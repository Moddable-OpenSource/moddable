# MCP230XX

Copyright 2017 Moddable Tech, Inc.

Revised: Dec 19, 2017

The [MCP23008](http://www.microchip.com/wwwproducts/en/MCP23008) device provides 8-bit, general purpose, parallel I/O expansion for I2C bus applications. (Description from MCP23008 product page)

The [MCP23017](http://www.microchip.com/wwwproducts/en/MCP23017) device provides 16-bit, general purpose, parallel I/O expansion for I2C bus applications. (Description from MCP23017 product page)


## module "MCP230XX"

The driver module "MCP230XX" exports the following:

```js
export { 
  MCP23008, 
  MCP23017
};
```



## class MCP23008

The `MCP23008` class produces instances that represent a single MCP23008 IC on the I2C bus. The `MCP23008` class extends an internal `Expander` class, which extends the `SMBus` class. `Expander` is not exported. 

Instance objects of `MCP23008` contain 8 `Pin` instance object entries.


```js
import Timer from "timer";
import {MCP23008} from "MCP230XX";


export default function() {
  let leds = new MCP23008(); // defaults to 0x20!
  let mask = 0x88;

  Timer.repeat(() => {
    mask = ((mask << 1) | (mask >> 7)) & 0xFF;

    for (let i = 0; i < 8; i++) {
      leds[i].write(mask & (1 << i) ? 1 : 0);
    }
  }, 50);
}
```

![](ESP8266-MCP23008-leds.png)

### Properties

| Property Name | Description | Read Only |
|---------------| ----------- | ----------|
| `length` | Number of pins in collection: `8` | Yes |
| `offset` | Register offset: `0` | Yes |
| `IODIR` | `IODIR` register: `0x00` | Yes |
| `GPIO` | `GPIO` register: `0x06` | Yes |
| `GPPU` | `GPPU` register: `0x09` | Yes |
| 0-8 | `Pin` instances | Yes |


### Methods

#### constructor({ [address], [hz], [sda], [scl], [inputs], [pullups] })

  | Property | Type   | Value/Description    | Default | Required |
  |----------|--------|----------------------|---------|----------|
  | `address`  | Number | The address of the I2C device | `0x20` | no |
  | `hz`       | Number | The clock speed of the I2C device. | 100kHz | no       |
  | `sda`      | Number | The I2C sda (data) pin.     | 4 | no       |
  | `scl`      | Number | The I2C scl (clock) pin.     | 5 | no       |
  | `inputs`   | Byte | A byte representing the input/output initialization state of the 8 GPIO pins. `1` for input, `0` for output | `0b11111111` | no       |
  | pullups  | Byte | A byte representing the pullup initialization state of the 8 GPIO pins. `1` for pullup, `0` for default | `0b00000000` | no       |

#### bankWrite(byte)

The method will temporarily set the _mode/direction_ of all pins to _output_ mode and write all pins at once. 

```js
let expander = new MCP23008(); // defaults to 0x20!
expander.bankWrite(0b11111111); // Set all pins to 1
```

#### bankRead() -> byte

The method will temporarily set the _mode/direction_ of all pins to _input_ mode and read all pins at once.

```js
let expander = new MCP23008(); // defaults to 0x20!
trace(`${expander.bankRead()}\n`); 
```


## class MCP23017

The `MCP23017` class produces instances that represent a single MCP23017 IC on the I2C bus. The `MCP23017` class extends an internal `Expander` class, which extends the `SMBus` class.

Instance objects of `MCP23017` contain 16 `Pin` instance object entries.


```js
import Timer from "timer";
import {MCP23017} from "MCP230XX";


export default function() {
  let leds = new MCP23017(); // defaults to 0x20!
  let mask = 0x8888;

  Timer.repeat(() => {
    mask = ((mask << 1) | (mask >> 15)) & 0xFFFF;

    for (let i = 0; i < 16; i++) {
      leds[i].write(mask & (1 << i) ? 1 : 0);
    }
  }, 50);
}
```

![](ESP8266-MCP23017-leds.png)

### Properties

| Property Name | Description | Read Only |
|---------------| ----------- | ----------|
| `length` | Number of pins in collection: `16` | Yes |
| `offset` | Register offset: `1` | Yes |
| `IODIR` | `IODIR` register: `0x00` | Yes |
| `GPIO` | `GPIO` register: `0x0C` | Yes |
| `GPPU` | `GPPU` register: `0x12` | Yes |
| 0-16 | `Pin` instances | Yes |


### Methods

#### constructor({ [address], [hz], [sda], [scl], [inputs], [pullups] })

  | Property | Type   | Value/Description    | Default | Required |
  |----------|--------|----------------------|---------|----------|
  | `address`  | Number | The address of the I2C device | `0x20` | no |
  | `hz`       | Number | The clock speed of the I2C device. | 100kHz | no       |
  | `sda`      | Number | The I2C sda (data) pin.     | 4 | no       |
  | `scl`      | Number | The I2C scl (clock) pin.     | 5 | no       |
  | `inputs`   | Word | A byte representing the input/output initialization state of the 8 GPIO pins. `1` for input, `0` for output | `0b1111111111111111` | no       |
  | `pullups`  | Word | A byte representing the pullup initialization state of the 16 GPIO pins. `1` for pullup, `0` for default | `0b0000000000000000` | no       |

#### bankWrite(word)

The method will temporarily set the _mode/direction_ of all pins to _output_ mode and write all pins at once. 

```js
let expander = new MCP23017(); // defaults to 0x20!
expander.bankWrite(0b1111111111111111); // Set all pins to 1
```

#### bankRead() -> word

The method will temporarily set the _mode/direction_ of all pins to _input_ mode and read all pins at once.

```js
let expander = new MCP23017(); // defaults to 0x20!
trace(`${expander.bankRead()}\n`); 
```

## class Pin

The `Pin` class represents a single pin within a `MCP23008` instance object. `Pin` is not exported.  


```js
import Timer from "timer";
import { MCP23008 } from "MCP230XX";


export default function() {
  const leds = new MCP23008({
    inputs: 0b00000000
  });
  
  leds[0].write(1);
  leds[1].write(0);
  leds[2].write(1);
  leds[3].write(0);
  leds[4].write(1);
  leds[5].write(0);
  leds[6].write(1);
  leds[7].write(0);
}
```

![](ESP8266-MCP23008-leds.png)

## Properties

| Property Name | Description | Read Only |
|---------------| ----------- | ----------|
| `pin` | The GPIO pin number | Yes |
| `expander` | The instance of `Expander` that this `Pin` belongs to | Yes |


## Methods

#### constructor({ pin, expander })

  | Property | Type   | Value/Description    | Default | Required |
  |----------|--------|----------------------|---------|----------|
  | pin  | Number | The GPIO pin number | n/a | yes |
  | expander | Number | The instance of `Expander` that created this `Pin` instance | n/a | yes       |
  
#### mode(mode)

Set the _mode/direction_ of the pin by  object. 

#### read()

Set the _mode/direction_ to _input_ and read the value of the pin object.

#### write(value)

Set the _mode/direction_ to _output_ and write the value to the pin object.



## Manifest Example

```
{
  "include": [
    "$(MODDABLE)/examples/manifest_base.json",
    "$(MODULES)/drivers/mcp230/manifest.json"
  ],
  "modules": {
    "*": [
      "./main",
    ]
  },
  "platforms": {
    "esp": {
      "modules": {
        "*": "$(MODULES)/pins/i2c/esp/*",
      },
    },
    "esp32": {
      "modules": {
        "*": "$(MODULES)/pins/i2c/esp32/*",
      },
    },
  }
}
```
