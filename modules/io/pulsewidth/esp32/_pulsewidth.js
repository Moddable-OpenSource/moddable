/*
 * Copyright (c) 2022-2025  Moddable Tech, Inc.
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

class PulseWidth extends Native("xs_pulsewidth_destructor") {
	constructor(options) { super(); native("xs_pulsewidth_constructor").call(this, options); };
	close() { return native("xs_pulsewidth_close").call(this); };
	read() { return native("xs_pulsewidth_read").call(this); };

    get format() {
        return "number";
    }

    set format(value) {
        if ("number" !== value)
            throw new RangeError;
    }
};

PulseWidth.Rising = 1;
PulseWidth.Falling = 2;

PulseWidth.Input = 0;
PulseWidth.InputPullUp = 1;
PulseWidth.InputPullDown = 2;

PulseWidth.RisingToFalling = 1;
PulseWidth.FallingToRising = 2;
PulseWidth.RisingToRising = 3;
PulseWidth.FallingToFalling = 4;

export default PulseWidth;
