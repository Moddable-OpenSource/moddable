# Timers
Copyright 2017 Moddable Tech, Inc.

Revised: November 7, 2017

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## class Timer

<!-- 11/7/2017 BSF
The Timer module does not provide a tick counter. That is provided by the Time module. Should we document the Time module here before Timer?
-->

The Timer module provides both time based callbacks and a tick counter. The Timer module exports a class with only static functions, so it does not need to be instantiated using `new`.

	import Timer from "timer";

	let ms = Timer.ticks();

### Timer.ticks()

The `ticks` function returns the value of a millisecond counter. The value returned does not correspond to the time of day. The milliseconds are used to calculate time differences.

	let start = Timer.ticks();
	for (let i = 0; i < 1000; i++)
		;
	let stop = Timer.ticks();
	trace(`Operation took ${stop - start} milliseconds\n`);

<!-- 11/7/2017 BSF
Timer.set now supports an optional third repeat interval parameter. The optional second parameter specifies a delay before invoking the callback.
-->
### Timer.set(callback[, interval])

The `set` function is used to request a function be called once after a certain period.

An immediate timer is called on the next cycle through the run loop. To set an immediate timer, call `set` with a single argument.

	Timer.set(id => trace("immediate fired\n");

A one shot timer is called once after a specified number of milliseconds. If the number of milliseconds is zero, a one shot timer is equivalent to an immediate timer.

	Timer.set(id => trace("one shot fired\n"), 1000);

The callback function receives the timer id as the first argument.

### Timer.repeat(callback, interval)

A repeating timer is called continuously until stopped using the `Timer.clear` function. 

	Timer.repeat(id => trace("repeat fired\n"), 1000,;

The callback function receives the timer id as the first argument.

### Timer.clear(id)

The `clear` function cancels a timer. The `Timer.set` and `Timer.repeat` functions returns the ID for a timer, which is then passed to clear.

	let aTimer = Timer.set(id => trace("one shot\n"), 1000);
	Timer.clear(aTimer);

> **Note**: Immediate and one shot timers are automatically cleared after invoking their callback function. There is no need to call `clear` except to cancel the timer before it fires.

<!-- 11/7/2017 BSF
Timer.schedule and Timer.delay need to be documented.
-->

