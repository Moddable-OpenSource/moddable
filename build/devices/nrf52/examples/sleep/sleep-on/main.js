/*
 * Copyright (c) 2016-2024  Moddable Tech, Inc.
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
import { Sleep, PowerMode } from "sleep";
import Timer from "timer";

let message = "Hello, world - sleep-on sample";
trace(message + "\n");

let led = new Host.LED.Default;
led.write(1);
Timer.delay(500);
led.close();

Sleep.powerMode = PowerMode.LowPower;		// vs .ConstantLatency
