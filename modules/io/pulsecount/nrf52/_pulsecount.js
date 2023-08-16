/*
 * Copyright (c) 2021 Moddable Tech, Inc.
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

class PulseCount @ "xs_pulsecount_destructor_" {
	constructor(options) @ "xs_pulsecount_";
	close() @ "xs_pulsecount_close_";
	read() @ "xs_pulsecount_read_";
	write(count) @ "xs_pulsecount_write_";

    get format() {
        return "number";
    }

    set format(value) {
        if ("number" !== value)
            throw new RangeError;
    }

};

export default PulseCount;
