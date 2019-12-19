# nRF52 Low Power Notes

Copyright 2019 Moddable Tech, Inc.<br/>
Author: Brian S. Friedkin<br/>
Revised: December 19, 2019

Warning: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## Low Power Modes
The Nordic nRF52 devices (Moddable Four) provide two low power (sleep) modes to reduce power consumption: **System ON** and **System OFF**. Applications can programatically enter and exit these power modes using Nordic APIs. The [nRF52840 Product Specification](https://infocenter.nordicsemi.com/pdf/nRF52840_PS_v1.0.pdf), Section 5.2 includes current consumption tables for common scenarios.

The Moddable SDK includes APIs to configure and leverage features provided by these low power modes.

### System ON Power Mode
System ON power mode is what the firmware and applications typically enter when idle. In System ON mode, the CPU is put to sleep and the Power Management Unit automatically shuts down any peripheral not in use. When the SoftDevice is enabled, i.e. when BLE is active, a few additional clocks remain active to wake up the CPU on BLE events. Typical current consumption while in System ON power mode is a few micro amps.

System ON power mode is further subdivided into two sub-power modes: **Constant Latency** and **Low Power.** Constant Latency sub-power mode maintains a constant latency by keeping certain oscillators and regulators running, while increasing power consumption. In the Low Power sub-power mode, the Power Management Unit turns these regulators/oscillators on and off based on demand, reducing power consumption, but increasing latency. The default sub-power mode is Low Power.

By default all RAM is powered while in System ON sleep mode, though it is possible to selectively power off RAM banks. Moddable apps do not currently power off RAM while in System ON sleep mode.

The Moddable nRF52 runtime is configured to automatically enter System ON power mode when applications are idle, i.e. waiting for events, messages, interrupts, etc....

### System OFF Power Mode
System OFF power mode is deep sleep. The CPU and most peripherals are asleep and disabled. The device always resets when exiting System OFF power mode. System OFF wake-up can be achieved through GPIO triggers (analog & digital), NFC or pin reset. RAM can be retained in System OFF mode. Typical current consumption while in System OFF power mode is a few hundred nano amps.

## FreeRTOS and Low Power
Moddable apps run on FreeRTOS on nRF52 devices. To conserve energy, FreeRTOS supports a [Tickless Idle](https://www.freertos.org/low-power-tickless-rtos.html) mode. Tickless idle disables the periodic tick interrupt, allowing the CPU to enter low power mode until a task needs to run or interrupt occurs, at which point the RTOS tick value is adjusted.

The core of the tickless idle implementation/hook is provided by our `vApplicationSleep()` function. This function enters System ON sleep mode when the application is idle and exits System ON sleep mode when an interrupt occurs or a task needs to run. 

The main loop in a Moddable app on nRF52 executes all "ready" timers and then waits on messages until the next timer needs to fire. If no timers are active, the app waits for a maximum delay time. This wait triggers the FreeRTOS scheduler, which in turn calls the tickless idle function and puts the app into System ON sleep mode.

The Nordic SDK includes a Power Manager API for configuring low power modes. Because FreeRTOS includes it's own power management hook (tickless idle), the Power Manager APIs cannot be called from the Moddable runtime. Instead, Moddable apps use the provided SoftDevice power API wrappers, when the SoftDevice is active, and/or the [Arm CMSIS hardware abstraction](https://developer.arm.com/tools-and-software/embedded/cmsis) APIs.

## RAM Retention
RAM on nRF52 devices is subdivided into slave regions. Each slave region is further subdivided into sections. RAM sections can be programatically powered off and/or retained in both System ON and OFF power modes. On the nRF52840-DK board, there are eight slave regions that can be controlled. Each slave region has two 4 KB sections.

The Moddable build reserves 512 bytes from one RAM section for retention in System OFF power mode. The number of bytes is arbitrary but a reasonable staring point. The retained section is defined in the linker scripts for SES and GCC. The runtime accesses this RAM buffer as a named section.

## Low Power Constraints
The one big "gotcha" is that System OFF sleep mode is *emulated* on debug builds. This is because the debugger needs certain hardware resources powered in order to maintain a connection. Therefore current consumption measurements are only valid when running release builds.

Similarly all logging and serial connections must be completely disabled when measuring consumption, including the NRF_LOG support available to release build Moddable apps.

## Class Sleep
The Sleep class includes static methods for setting low power modes, configuring deep sleep wake-up triggers, and RAM retention. Some of the APIs and concepts are borrowed from the Gecko `Sleep` class. This section provides an overview of the available methods and features.

```javascript
import Sleep from "sleep";
```

### Functions

#### `install(handler)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `handler` | `function` | Function to call before entering System OFF sleep mode.

The `install` method allows an application to provide a callback function that is called immediately before the device enters System OFF deep sleep mode. Multiple callback functions can be installed.


```javascript
function preSleep()
{
	// retain app settings
	// turn off screen
}

Sleep.install(preSleep);
```

***

#### `deep()`

Call the `deep` function to enter deep sleep, a.k.a. System OFF sleep mode.

To put the device to deep sleep:

```javascript
Sleep.deep();
```

***

#### `resetReason()`

The `resetReason` accessor function returns the reason for the most recent system reset. The `ResetReason` object contains property values corresponding to each reset reason.

### Properties

| Name | Description |
| --- | --- | :--- |
| `RESETPIN` | Reset pin
| `DOG ` | Reset from watchdog timer
| `SREQ ` | Software reset
| `LOCKUP ` | Reset from lockup
| `GPIO ` | Reset triggered from GPIO DETECT (digital) signal
| `LPCOMP ` | Reset triggered from GPIO ANDETECT (analog) signal
| `DIF ` | Reset triggered from debugger interface
| `NFC` | Reset triggered from NFC field detect

To check if reset was triggered by a digital input:

```javascript
import {Sleep, ResetReason} from "sleep";

let reason = Sleep.resetReason;
if (ResetReason.GPIO == reason)
	trace(`Reset was triggered by a digital pin\n`);
```

***

#### `resetPin`

The `resetPin` getter function returns the digital pin corresponding to the most recent reset event.

```javascript
const BUTTON1_PIN = 12;
const BUTTON2_PIN = 13;

let pin = Sleep.resetPin;
if (BUTTON2_PIN == pin)
	trace(`Reset was triggered by button 2\n`);
```

> Note: The `resetPin` accessor function simply reads the device `GPIO->IN` register when this function is called. The digital pin that triggered the reset is not provided by the nRF52 firmware.

***

#### `powerMode()`

The `powerMode ` setter function sets the System ON low power sub-mode. The `PowerMode` object contains property values corresponding to each mode.

### Properties

| Name | Description |
| --- | --- | :--- |
| `ConstantLatency` | Constant Latency sub-mode
| `LowPower` | Low Power sub-mode

To set System ON low power sub-power mode:

```javascript
import {Sleep, PowerMode} from "sleep";

Sleep.powerMode = PowerMode.LowPower;
```

> Note: There is no API to read the current power mode. The underlying `NRF_POWER` register is write-only.

***

#### `getRetainedBuffer()`

The `getRetainedBuffer` method allows an application to retrieve a memory buffer previously retained across System OFF sleep. The function returns an `ArrayBuffer` containing the retained memory when available. If no memory was retained the function returns `undefined`.


```javascript
let buffer = Sleep.getRetainedBuffer();
if (undefined !== buffer) {
	let retained = new Uint8Array(buffer);
}
```

***

#### `setRetainedBuffer(buffer)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `buffer` | `object` | `ArrayBuffer` containing the data to be retained across System OFF sleep. The buffer size must be <= 512 bytes.

The `setRetainedBuffer` method allows an application to retain data across System OFF sleep. A subsequent call to `setRetainedBuffer` replaces any existing retained buffer.


```javascript
let retained = new Uint8Array(100);
for (let i = 0; i < 100; ++i)
	retained[i] = i;
Sleep.setRetainedBuffer(retained.buffer);
```
***

#### `clearRetainedBuffer()`

The `clearRetainedBuffer` method allows an application to clear any existing retained RAM buffer.

To clear any retained RAM and go to sleep:

```javascript
Sleep.clearRetainedBuffer();
Sleep.deep();
```

***

#### `wakeOnAnalog(channel, configuration)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `channel` | `number` | Analog channel configured to wake-up device.
| `configuration` | `object` | Wake-up configuration properties

The `configuration` object contains the following properties:

| Property | Type | Description |
| --- | --- | :--- |
| `value` | `number` | Analog threshold value.
| `mode` | `number` | `AnalogDetectMode` property value corresponding to the trigger mode. `AnalogDetectMode.Up` mode triggers wake-up on an upward crossing of `value`. `AnalogDetectMode.Down` mode triggers wake-up on a downward crossing of `value`. `AnalogDetectMode.Crossing` mode triggers wake-up on a downward or upward crossing.

The `wakeOnAnalog` method allows an application to wake-up from System OFF sleep mode from an analog input trigger. Wake-up occurs when the voltage crosses the provided analog value. The analog value is mapped to a corresponding percentage of the reference voltage.

The `wakeOnAnalog` function uses the low power comparator (LPCOMP), which is available during deep sleep and can reset the device.

To configure wake-up on an upward crossing of 50% of the reference voltage:

```javascript
import {Sleep, ResetReason} from "sleep";

const analogChannel = 5;
Sleep.wakeOnAnalog(analogChannel, 512);		// 10-bit analog resolution
Sleep.deep();

...
// on application launch
if (ResetReason.LPCOMP == Sleep.resetReason)
	trace(`wake on analog\n`);
```

***

#### `wakeOnDigital(pin)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `pin` | `number` | Digital pin configured to wake-up device.

The `wakeOnDigital` method allows an application to wake-up from System OFF sleep mode from a digital input trigger, e.g button press.

To configure wake-up on a button press:

```javascript
import {Sleep, ResetReason} from "sleep";

const PIN = 25;		// Button 4 on nRF52840-DK
const LED = 13;		// LED1 on nRF52840-DK
const OFF = 1;		// active low

// wakeup on pin
Sleep.wakeOnDigital(PIN);

// turn off led while asleep
Digital.write(LED, OFF);
	
Sleep.deep();

...
// on application launch
if (ResetReason.GPIO == Sleep.resetReason)
	trace(`wake on digital\n`);
```

#### `wakeOnInterrupt(pin)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `pin` | `number` | Digital pin connected to interrupt source configured to wake-up device.

The `wakeOnInterrupt` method allows an application to wake-up from System OFF sleep mode from an interrupt input pulse trigger.

To configure wake-up on an interrupt pulse:

```javascript
import {Sleep, ResetReason} from "sleep";

const PIN = 6;	// Pin P0.06 connected to LIS3DH INT pin

Sleep.wakeOnInterrupt(PIN);
Sleep.deep();

...
// on application launch
if (ResetReason.GPIO == Sleep.resetReason)
	trace(`wake on interrupt\n`);
```

> Note: Both the `wakeOnDigital` and `wakeOnInterrupt` functions leverage digital GPIO. Therefore on wakeup it is not possible differentiate between interrupt and digital triggers. The implementation leverages digital sense input in both cases.

## Example apps
Example apps are provided to demonstrate the various `Sleep` class features:

| Name | Description |
| :---: | :--- |
| [deep-sleep]() | Enter deep sleep mode and read the reset reason on launch.
| [ram-retention]() | Retain an `ArrayBuffer` across System OFF sleep mode and verify the contents on reset.
| [wake-on-analog]() | Configure wake-up on analog crossing configurations and read the analog value and reset reason on launch.
| [wake-on-digital]() | Configure wake-up on a button press.
| [wake-on-interrupt]() | Configure wake-up from a connected LIS3DH accelerometer configured to generate interrupts on motion. 

## References
* [nRF52 Product Specification](https://infocenter.nordicsemi.com/pdf/nRF52832_PS_v1.1.pdf)
* [Optimizing Power on nRF52 Designs](https://devzone.nordicsemi.com/nordic/nordic-blog/b/blog/posts/optimizing-power-on-nrf52-designs)
* [FreeRTOS sleep optimization from main task](https://devzone.nordicsemi.com/f/nordic-q-a/55181/freertos-sleep-optimization-from-main-task)

