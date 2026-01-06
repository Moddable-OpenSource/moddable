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

class RTC extends Native("xs_rtc_destructor") {
	constructor(options) { super(); native("xs_rtc_constructor").call(this, options); }
	close() { return native("xs_rtc_close").call(this); }
	set time(value) { native("xs_rtc_time_set").call(this, value); }
	get time() { return native("xs_rtc_time_get").call(this); }
	configure(value) {
		const alarm = value?.alarm;
		if (undefined !== alarm)
			native("xs_rtc_alarm").call(this, Number(alarm));
	}
}

export default RTC;
