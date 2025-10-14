/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

class RTC @ "xs_rtc_destructor" {
	constructor(options) @ "xs_rtc_constructor"
	close() @ "xs_rtc_close"
	set time(value) @ "xs_rtc_time_set"
	get time() @ "xs_rtc_time_get"
	configure(value) {
		const alarm = value?.alarm;
		if (undefined !== alarm)
			configure.call(this, Number(alarm));
	}
}

function configure(alarm) @ "xs_rtc_alarm"

export default RTC;
