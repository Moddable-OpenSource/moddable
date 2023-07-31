/*
 * Copyright (c) 2020  Moddable Tech, Inc.
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

class WakeableAnalog @ "xs_wakeableanalog_destructor" {
	constructor(dictionary) @ "xs_wakeableanalog_constructor"
	close() @ "xs_wakeableanalog_close"
	read() @ "xs_wakeableanalog_read"

	get wakeupReason() @ "xs_wakeableanalog_get_wakeup_reason"

	get format() {
		return "number";
	}
	set format(value) {
		if ("number" !== value)
			throw new RangeError;
	}
	
	get resolution() {
		return 10;	
	}
}

export default WakeableAnalog;