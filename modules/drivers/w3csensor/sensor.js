/*
 * Copyright (c) 2018-2019  Moddable Tech, Inc.
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

class Sensor @ "xs_sensor_destructor" {
	constructor(dictionary) @ "xs_sensor"
	start() @ "xs_sensor_start"
	stop() @ "xs_sensor_stop"

	get activated() @ "xs_sensor_get_activated"
	get hasReading() @ "xs_sensor_get_hasReading"
	get timestamp() @ "xs_sensor_get_timestamp"

	set onreading() @ "xs_sensor_set_onreading"
	get onreading() @ "xs_sensor_get_onreading"
	set onactivate() @ "xs_sensor_set_onactivate"
	get onactivate() @ "xs_sensor_get_onactivate"
	set onerror() @ "xs_sensor_set_onerror"
	get onerror() @ "xs_sensor_get_onerror"

	addEventListener(handler) @ "xs_sensor_addEventListener"
	removeEventListener(handler) @ "xs_sensor_removeEventListener"

	// subclasses override
	_activate() {}
	_deactivate() {}
	_poll() {}
}
Object.freeze(Sensor.prototype);

export default Sensor;
