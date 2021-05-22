/*
 * Copyright (c) 2019-2021  Moddable Tech, Inc.
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

import Timer from "timer";
import device from "embedded:provider/builtin";

class System {
	static deepSleep() @ "xs_system_deepSleep"
	static restart() @ "xs_system_restart"

	static resolve(name, callback) @ "xs_system_resolve"

	static setTimeout(callback, delay) {
		return Timer.set(callback, delay);
	}
	static clearTimeout(id) {
		Timer.clear(id);
	}
	static setInterval(callback, delay) {
		return Timer.repeat(callback, delay);
	}
	static clearInterval(id) {
		Timer.clear(id);
	}
}

globalThis.System = System;
globalThis.device = Object.freeze(device, true);
