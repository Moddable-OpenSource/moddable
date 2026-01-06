/*
 * Copyright (c) 2019-2025  Moddable Tech, Inc.
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

class PWM extends Native("xs_pwm_destructor_") {
    constructor(dictionary) { super(); native("xs_pwm_constructor_").call(this, dictionary); }
    close() { return native("xs_pwm_close_").call(this); }
    write() { return native("xs_pwm_write_").call(this); }
    
    get hz() { return native("xs_pwm_get_hz_").call(this); }
    get resolution() { return native("xs_pwm_get_resolution_").call(this); }

    get format() {
        return "number";
    }

    set format(value) {
        if ("number" !== value)
            throw new RangeError;
    }
}

export default PWM;
