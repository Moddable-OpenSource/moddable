/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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


/*
	time
*/

class Time {
	static set(value) { return native("xs_time_set").call(this, value); }

	static get timezone() { return native("xs_time_timezone_get").call(this); }
	static set timezone(it) { native("xs_time_timezone_set").call(this, it); }

	static get dst() { return native("xs_time_dst_get").call(this); }
	static set dst(it) { native("xs_time_dst_set").call(this, it); }

	static get ticks() { return native("xs_time_ticks").call(this); }

	static delta(start, end) { return native("xs_time_delta").call(this, start, end); }
};

export default Time;
