/*
 * Copyright (c) 2018  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

/* This example is to demonstrate sleep on SiLabs Gecko devices */

import Sleep from "sleep";
import Timer from "timer";

export default function() {
	let wakeupCause = Sleep.getWakeupCause();
	let kCycleSleepTime = 5000;

	if (wakeupCause & Sleep.PowerOnReset)		// device power on
		trace("Woke: PowerOnReset\n");
	if (wakeupCause & Sleep.ExternalReset)		// external wake button pressed
		trace("Woke: ExternalReset\n");
	if (wakeupCause & Sleep.SysRequestReset)	// system initiated reboot
		trace("Woke: SysRequestReset\n");
	if (wakeupCause & Sleep.EM4WakeupReset)		// woke from EM4 sleep
		trace("Woke: EM4WakeupReset\n");
	if (wakeupCause & Sleep.BrownOutReset)		// woke from brownout
		trace("Woke: BrownOutReset\n");
	if (wakeupCause & Sleep.LockupReset)		// woke from lockup
		trace("Woke: LockupReset\n");
	if (wakeupCause & Sleep.WatchdogReset)		// woke by watchdog timer
		trace("Woke: WatchdogReset\n");

	// reset counter if first time on or debugger reset
	if ((wakeupCause & Sleep.SysRequestReset) ||
		(wakeupCause & Sleep.PowerOnReset))
		Sleep.setPersistentValue(0, 0);

	// which pin woke device
	if (wakeupCause & Sleep.ExternalReset)
		trace(" wakeup pin: " + Sleep.getWakeupPin() + "\n");

	let storedValue = Sleep.getPersistentValue(0);

	trace("PersistentValue[0] = " + storedValue + "\n");

	let cycle = 1;

	trace("Set idle sleep level 1\n");
	Sleep.setIdleSleepLevel(1);

	Timer.repeat(() => {
		trace("Cycle # " + cycle + "\n");
		switch (cycle) {
			case 1:
				trace("Set idle sleep level 2\n");
				Sleep.setIdleSleepLevel(2);
				break;
			case 2:
				trace("Set idle sleep level 3\n");
				Sleep.setIdleSleepLevel(3);
				break;
			default:
				trace("About to EM4 sleep\n");
				storedValue = storedValue + 1;
				Sleep.setPersistentValue(0, storedValue);
				Sleep.doSleepEM4(20000);
				break;
		}
		cycle = cycle + 1;
	}, kCycleSleepTime);

}

