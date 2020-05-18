/*
 * Copyright (c) 2018  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

class Monitor @ "xs_digital_monitor_destructor" {
	constructor(dictionary) @ "xs_digital_monitor";
	close() @ "xs_digital_monitor_close"
	read() @ "xs_digital_monitor_read"
	get rises() @ "xs_digital_monitor_get_rises"
	get falls() @ "xs_digital_monitor_get_falls"

	get pwm_duty() @ "xs_digital_monitor_get_pwm_duty"
	get pwm_freq() @ "xs_digital_monitor_get_pwm_freq"
}
Monitor.Rising = 1;
Monitor.Falling = 2;
Monitor.PWM = 4;

Object.freeze(Monitor.prototype);

export default Monitor;
