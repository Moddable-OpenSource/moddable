# Base
Copyright 2017 Moddable Tech, Inc.

Revised: November 25, 2017

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## class Timer

The Timer module provides both time based callbacks and a delay function.

	import Timer from "timer";

### Timer.set(callback[, interval, repeat])

The `set` function is used to request a function be called once after a certain period.

An immediate timer is called on the next cycle through the run loop. To set an immediate timer, call `set` with a single argument.

	Timer.set(id => trace("immediate fired\n");

A one shot timer is called once after a specified number of milliseconds. If the number of milliseconds is zero, a one shot timer is equivalent to an immediate timer.

	Timer.set(id => trace("one shot fired\n"), 1000);

Calling `set` with a `repeat` is equivalent to a repeating timer with the first callback triggered after the `interval`.

	Timer.set(id => trace("repeat fired\n", 1000, 100);
	
The callback function receives the timer id as the first argument.

### Timer.repeat(callback, interval)

A repeating timer is called continuously until stopped using the `Timer.clear` function. 

	Timer.repeat(id => trace("repeat fired\n"), 1000);

The callback function receives the timer id as the first argument.

### Timer.schedule(id, interval[, repeat])

The `schedule` function is used to reschedule an existing timer.

In the following example, the callback function is triggered twice at one second intervals and then rescheduled to once every two seconds.

	let state = 0;
	Timer.repeat(id => {
		if (0 == state)
			state = 1;
		else if (1 == state) {
			state = 2;
			Timer.schedule(id, 2000, 2000);
		}
	}, 1000);

### Timer.clear(id)

The `clear` function cancels a timer. The `Timer.set` and `Timer.repeat` functions returns the ID for a timer, which is then passed to clear.

	let aTimer = Timer.set(id => trace("one shot\n"), 1000);
	Timer.clear(aTimer);

> **Note**: Immediate and one shot timers are automatically cleared after invoking their callback function. There is no need to call `clear` except to cancel the timer before it fires.

### Timer.delay(ms)

The `delay` function delays execution for the specified number of milliseconds.

	Timer.delay(500);	// delay 1/2 second

## class Time

The Time module provides time functions and a tick counter.

	import Time from "time";

### Time.set(seconds)

The `set` function sets the system time. The `seconds` argument corresponds to the number of seconds elapsed since January 1, 1970, i.e. Unix time.


### timezone

The `timezone` property is set to the time zone offset in seconds from UTC.

	Time.timezone = +9 * 60 * 60;	// Set time zone to UTC+09:00

### dst

The `dst` property is set to the daylight saving time (DST) offset in seconds.

	Time.dst = 60 * 60;	// Set DST

### ticks

The `ticks` property returns the value of a millisecond counter. The value returned does not correspond to the time of day. The milliseconds are used to calculate time differences.

	let start = Time.ticks;
	for (let i = 0; i < 1000; i++)
		;
	let stop = Time.ticks;
	trace(`Operation took ${stop - start} milliseconds\n`);

## class Debug

The Debug module provides functions that are useful during the development process.

### Debug.gc([enable])

The `gc` function can be used to turn the JavaScript garbage collector on and off, as well as to run the garbage collector on-demand.

Calling `Debug.gc` with no arguments runs the garbage collector immediately.

	Debug.gc();

Calling `Debug.gc` with a single boolean argument enables or disables the garbage collector.

	Debug.gc(true)	// enable garbage collector
	Debug.gc(false);	// disable garbage collector

## class UUID

The UUID module provides a single function to generate a [UUID](https://en.wikipedia.org/wiki/Universally_unique_identifier) string. 

	import UUID from "uuid";

> **Note**: Generating a truly unique UUID requires that the device running this function have a unique MAC address and the valid time. Neither of these is guaranteed on all micro-controllers. For production software release, check your device configuration.

### UUID()

The `UUID` function returns a new UUID formatted as a string.

	let value = UUID();	// 1080B49C-59FC-4A32-A38B-DE7E80117842

## class Instrumentation

The Instrumentation class returns statistics on the behavior of the runtime, including memory use, open file count, and rendering frame rate.

	import Instrumentation from "instrumentation";

### get(what)

The `get` function returns the value of the instrumented item at the index specified by the what parameter. Instrumented items are consecutively numbered started at one.

	let pixelsDrawn = Instrumentation.get(1);

The following instrumented items are available. The following instrumented items are reset at one second intervals: Pixels Drawn, Frames Drawn, Poco Display List Used, Piu Command List Used, Network Bytes Read, Network Bytes Written, and Garbage Collection Count. 

#### Pixels drawn (1)

The total number of pixels rendered to by Poco during the current interval. This value includes pixels drawn to a display device and pixels rendered offscreen.

#### Frames drawn (2)

The total number of frames rendered by Poco during the most recent interval. Frames are counted by calls to Poco.prototype.end() and by Piu frame updates.

#### Network bytes read (3)

The total number of bytes received by the Socket module during the current interval. This includes bytes received over TCP and UDP.

#### Network bytes written (4)

The total number of bytes send by the Socket module during the current interval. This includes bytes sent using TCP and UDP.

#### Network sockets (5)

The total number of active network sockets created by the Socket module. This includes TCP sockets, TCP listeners, and UDP sockets.

#### Timers (6)

The number of allocated timers created by the Timer module.

#### Files (7)

The number of open files and directory iterators created the File module.

#### Poco display list used (8)

The peak size in bytes of the Poco display list in the current interval.

#### Piu command list used (9)

The peak size in bytes of the Piu command list in the current interval.

#### System free memory (10)

The number of free bytes in the system memory heap. This value is not available on the simulator.

#### Slot heap size (11)

Number of bytes in use in the slot heap of the primary XS machine. Some of these bytes may be freed when the garbage collector next runs.

#### Chunk heap size (12)

Number of bytes in use in the chunk heap of the primary XS machine. Some of these bytes may be freed when the garbage collector next runs.

#### Keys used (13)

Number of runtime keys allocated by the primary XS machine. Once allocated keys are never deallocated.

#### Garbage collection count (14)

The number of times the garbage collector has run in the current interval.

#### Modules loaded (15)

The number of JavaScript modules that are currently loaded in the primary XS machine. This number does not include modules which are preloaded.

#### Stack peak (16)

The maximum depth in bytes of the stack of the primary XS virtual machine during the current interval.

## class Console

The Console module implements a serial terminal for debugging and diagnostic purposes. The Console module uses CLI modules to implement the terminal commands.

<!-- complete -->

## class CLI

The CLI module is a plug-in interface for commands used in a command line interface. CLI classes implement commands for use by the Console module (serial command line) and Telnet module (network command line).

<!-- complete -->
