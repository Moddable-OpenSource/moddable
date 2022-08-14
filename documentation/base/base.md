# Base
Copyright 2017-2022 Moddable Tech, Inc.<BR>
Revised: August 14, 2022

## Table of Contents

* [Timer](#timer)
* [Time](#time)
* [Debug](#debug)
* [UUID](#uuid)
* [deepEqual](#deepequal)
* [structuredClone](#structuredclone)
* [Instrumentation](#instrumentation)
* [Console](#console)
* [CLI](#cli)
* [Worker](#worker)

<a id="timer"></a>
## class Timer

- **Source code:** [timer](../../modules/base/timer)
- **Relevant Examples:** [timers](../../examples/base/timers/)

The `Timer` class provides both time-based callbacks and a delay function.

```js
import Timer from "timer";
```

### `Timer.set(callback[, interval, repeat])`

The `set` function is used to request a function be called once after a certain period.

An immediate timer is called on the next cycle through the run loop. To set an immediate timer, call `set` with a single argument.

```js
Timer.set(id => trace("immediate fired\n"));
```

A one shot timer is called once after a specified number of milliseconds. If the number of milliseconds is zero, a one shot timer is equivalent to an immediate timer.

```js
Timer.set(id => trace("one shot fired\n"), 1000);
```

Calling `set` with a `repeat` is equivalent to a repeating timer with the first callback triggered after the `interval`.

```js
Timer.set(id => trace("repeat fired\n"), 1000, 100);
```

The callback function receives the timer id as the first argument.

***

### `Timer.repeat(callback, interval)`

A repeating timer is called continuously until stopped using the `Timer.clear` function. 

```js
Timer.repeat(id => trace("repeat fired\n"), 1000);
```

The callback function receives the timer id as the first argument.

***

### `Timer.schedule(id [, interval[, repeat]])`

The `schedule` function is used to reschedule an existing timer.

If called with an `interval` but no `repeat`, the timer behaves like a one shot timer created with `Timer.set`. If called with both an `interval` and `repeat`, it behaves like a repeating timer created with `Timer.set` with both `interval` and `repeat` arguments. If called with neither `interval` nor `repeat` arguments, the timer is unscheduled and will not trigger until rescheduled using `Timer.schedule`.

In the following example, the callback function is triggered twice at one second intervals and then rescheduled to once every two seconds.

```js
let state = 0;
Timer.repeat(id => {
	if (0 == state)
		state = 1;
	else if (1 == state) {
		state = 2;
		Timer.schedule(id, 2000, 2000);
	}
}, 1000);
```

> **Note**: If the next trigger time is unknown, unscheduling a timer is preferred to scheduling for a long time in the future. Unscheduling and rescheduling a timer can more efficient than clearing a timer and later allocating a new one.

***

### `Timer.clear(id)`

The `clear` function cancels a timer. The `Timer.set` and `Timer.repeat` functions returns the ID for a timer, which is then passed to clear.

```js
let aTimer = Timer.set(id => trace("one shot\n"), 1000);
Timer.clear(aTimer);
```

> **Note**: Immediate and one shot timers are automatically cleared after invoking their callback function. There is no need to call `clear` except to cancel the timer before it fires.

> **Note**: If `Timer.clear` is passed a value of `undefined` or `null` for the ID, no exception is generated.

***

### `Timer.delay(ms)`

The `delay` function delays execution for the specified number of milliseconds.

```js
Timer.delay(500);	// delay 1/2 second
```

***

<a id="time"></a>
## class Time

- **Source code:** [time](../../modules/base/time)
- **Source code:** [time](../../modules/base/microseconds)
- **Relevant Examples:** [sntp](../../examples/network/sntp/), [ntpclient](../../examples/network/mdns/ntpclient), [ios-time-sync](../../examples/network/ble/ios-time-sync)

The `Time` class provides time functions and a tick counter.

```js
import Time from "time";
```

### `Time.set(seconds)`

The `set` function sets the system time. The `seconds` argument corresponds to the number of seconds elapsed since January 1, 1970, i.e. Unix time.

***

### `timezone` property

The `timezone` property is set to the time zone offset in seconds from UTC.

```js
Time.timezone = +9 * 60 * 60;	// Set time zone to UTC+09:00
```

***

### `dst` property

The `dst` property is set to the daylight saving time (DST) offset in seconds.

```js
Time.dst = 60 * 60;	// Set DST
```

***

### `ticks` property

The `ticks` property returns the value of a millisecond counter. The value returned does not correspond to the time of day. The milliseconds are used to calculate time differences.

```js
let start = Time.ticks;
for (let i = 0; i < 1000; i++)
	;
let stop = Time.ticks;
trace(`Operation took ${stop - start} milliseconds\n`);
```

***

### `microseconds` property  *(optional)*

The `microseconds` property returns the value of a microseconds counter. The value returned does not correspond to the time of day. The microseconds are used to calculate time differences.

To use the `microseconds` property, include its manifest in the project manifest:

```
	"include": [
		...
		"$(MODDABLE)/modules/base/microseconds/manifest.json",
	],
```

The `microseconds` property is used in the same way as the `ticks` property.

```js
const start = Time.microseconds;
for (let i = 0; i < 1000; i++)
	;
const stop = Time.microseconds;
trace(`Operation took ${stop - start} microseconds\n`);
```

> **Note**: Not all hosts implement microseconds. Hosts that do not implement microseconds generate an error at build time.

***

<a id="debug"></a>
## class Debug

- **Source code:** [debug](../../modules/base/debug)

The `Debug` class provides functions that are useful during the development process.

```js
import Debug from "debug";
```

### `Debug.gc([enable])`

The `gc` function can be used to turn the JavaScript garbage collector on and off, as well as to run the garbage collector on-demand.

Calling `Debug.gc` with no arguments runs the garbage collector immediately.

```js
Debug.gc();
```

Calling `Debug.gc` with a single boolean argument enables or disables the garbage collector.

```js
Debug.gc(true)	// enable garbage collector
Debug.gc(false);	// disable garbage collector
```

***

<a id="uuid"></a>
## class UUID

- **Source code:** [uuid](../../modules/base/uuid)

The `UUID` class provides a single function to generate a [UUID](https://en.wikipedia.org/wiki/Universally_unique_identifier) string. 

```js
import UUID from "uuid";
```

> **Note**: Generating a truly unique UUID requires that the device running this function have a unique MAC address and the valid time. Neither of these is guaranteed on all microcontrollers. For production software release, check your device configuration.

### `UUID()`

The `UUID` function returns a new UUID formatted as a string.

```js
let value = UUID();	// 1080B49C-59FC-4A32-A38B-DE7E80117842
```

***

<a id="deepequal"></a>
## function deepEqual(a, b [,options])

- **Source code:** [deepEqual](../../modules/base/deepEqual)
- **Tests:** [deepEqual](../../tests/modules/base/deepEqual)

The `deepEqual` function implements a deep comparison between two JavaScript object. 

```js
import deepEqual from "deepEqual";
```

There is no standard algorithm for deeply comparing two objects in JavaScript, and there are many possible correct approaches. The Moddable SDK adopts the behavior of `assert.deepEqual` and `assert.deepStrictEqual` from Node.js for its implementation of `deepEqual`.

The `deepEqual` function has an optional third parameter, an options object. By default the `deepEqual` [comparison is not-strict](https://nodejs.org/api/assert.html#assertdeepequalactual-expected-message), similar to using `==` for comparisons. Passing `{strict: true}` for the options object uses [strict comparison](https://nodejs.org/api/assert.html#assertdeepstrictequalactual-expected-message), similar to using `===` for comparisons.

```js
const a = {a: 1, b: 0, c: "str"};
const b = {a: 1, b: "0", c: "str"};
deepEqual(a, b);		// true
deepEqual(a, b, {strict: true});		// false

```

The known differences between the Moddable SDK implementation and Node.js will not impact most uses:

- `WeakMap` and `WeakSet` using read-only objects as keys
- XS does not call `valueOf` to compare boxed primitives

***

<a id="structuredclone"></a>
## function structuredClone(object[, transferables])

- **Source code:** [structuredClone](../../modules/base/structuredClone)
- **Tests:** [structuredClone](../../tests/modules/base/structuredClone)

The `structuredClone` function creates a deep copy of a JavaScript object. 

```js
import structuredClone from "structuredClone";
```

The `structuredClone` function in the Moddable SDK implements the [algorithm](https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm) defined by WHATWG for the web platform as much as practical, including circular references and the `transferables` option. 

```js
const a = {a: 1, b: Uint8Array.of(1, 2, 3,)}
const aCopy = structuredClone(a);
```

The Moddable SDK implementation of `structuredClone` implements all [supported types](https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm#supported_types) that are part of the JavaScript language standard. It does not, of course, clone object types that are part of the web platform and not present on embedded systems such as `DOMException`.

<a id="instrumentation"></a>
## class Instrumentation

- **Source code:** [instrumentation](../../modules/base/instrumentation)
- **Relevant Examples:** [instrumentation](../../examples/base/instrumentation)

The `Instrumentation` class returns statistics on the behavior of the runtime, including memory use, open file count, and rendering frame rate.

```js
import Instrumentation from "instrumentation";
```

### `get(what)`

The `get` function returns the value of the instrumented item at the index specified by the `what` parameter. Instrumented items are consecutively numbered starting at index 1.

```js
let pixelsDrawn = Instrumentation.get(1);
```

The table below describes the instrumented items that are available. The following instrumented items are reset at one second intervals: Pixels Drawn, Frames Drawn, Poco Display List Used, Piu Command List Used, Network Bytes Read, Network Bytes Written, and Garbage Collection Count. 

| Index | Short Description | Long Description |
| :---: | :--- | :--- |
| 1 | Pixels drawn | The total number of pixels rendered to by Poco during the current interval. This value includes pixels drawn to a display device and pixels rendered offscreen.
| 2 |Frames drawn | The total number of frames rendered by Poco during the most recent interval. Frames are counted by calls to Poco.prototype.end() and by Piu frame updates.
| 3 | Network bytes read | The total number of bytes received by the Socket module during the current interval. This includes bytes received over TCP and UDP.
| 4 | Network bytes written | The total number of bytes send by the Socket module during the current interval. This includes bytes sent using TCP and UDP.
| 5 | Network sockets |The total number of active network sockets created by the Socket module. This includes TCP sockets, TCP listeners, and UDP sockets.
| 6 | Timers | The number of allocated timers created by the Timer module.
| 7 | Files | The number of open files and directory iterators created the File module.
| 8 | Poco display list used | The peak size in bytes of the Poco display list in the current interval.
| 9 | Piu command list used | The peak size in bytes of the Piu command list in the current interval.
| 10 | System free memory | The number of free bytes in the system memory heap. This value is not available on the simulator.
| 11 | Slot heap size | Number of bytes in use in the slot heap of the primary XS machine. Some of these bytes may be freed when the garbage collector next runs.
| 12 | Chunk heap size | Number of bytes in use in the chunk heap of the primary XS machine. Some of these bytes may be freed when the garbage collector next runs.
| 13 | Keys used | Number of runtime keys allocated by the primary XS machine. Once allocated keys are never deallocated.
| 14 | Garbage collection count | The number of times the garbage collector has run in the current interval.
| 15 | Modules loaded | The number of JavaScript modules that are currently loaded in the primary XS machine. This number does not include modules which are preloaded.
| 16 | Stack peak | The maximum depth in bytes of the stack of the primary XS virtual machine during the current interval.

***

<a id="console"></a>
## class Console

- **Source code:** [console](../../modules/base/console)
- **Relevant Examples:** [console](../../examples/base/console)

The `Console` class implements a serial terminal for debugging and diagnostic purposes. The `Console` module uses `CLI` modules to implement the terminal commands.

<!-- complete -->

***

<a id="cli"></a>
## class CLI

- **Source code:** [cli](../../modules/base/cli)
- **Relevant Examples:** [console](../../examples/base/console), [telnet](../../examples/network/telnet)

The `CLI` class is a plug-in interface for commands used in a command line interface. `CLI` classes implement commands for use by the `Console` module (serial command line) and `Telnet` module (network command line).

<!-- complete -->

***

<a id="worker"></a>
## class Worker

See the [Worker documentation](./worker.md) for more information about the `Worker` class.


