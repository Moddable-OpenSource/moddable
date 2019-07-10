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

import Sensor from "sensor";
import PROXIMITY_SENSOR from "proximity_sensor";

class ProximitySensor extends Sensor {
	constructor(dictionary) {
		super(dictionary);
		if (dictionary) this._dictionary = dictionary;
	}

	_activate() {
		this._sensor = new PROXIMITY_SENSOR(this._dictionary || {});
	}
	_deactivate() {
		this._sensor.close();
		delete this._sensor;
		delete this._sample;
	}
	_poll() {
		let distanceMM = this._sensor.sample();
		let maxMM = this._sensor.maxRange;
		this._sample = {
			distance: distanceMM / 10,
			max: maxMM / 10,
			near: distanceMM < maxMM
		}
	}

	get distance() {
		return (this._sample && this._sample.distance <= this._sample.max) ? this._sample.distance : null;
	}
	get max() {
		return this._sample ? this._sample.max : null;
	}
	get near() {
		return this._sample ? (this._sample.distance < this._sample.max ? true : null) : null;
	}
}
Object.freeze(ProximitySensor.prototype);

export default ProximitySensor;
