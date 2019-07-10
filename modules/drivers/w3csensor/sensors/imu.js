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
import IMU_SENSOR from "imu_sensor";
import CONFIG from "mc/config";

var sensorInstance;

class IMU extends Sensor {
  get _sensor(){
    return sensorInstance;
  }

  static set _sensor(args){
    
  }
  
  constructor(dictionary) {
		super(dictionary);
    
		if (dictionary) this._dictionary = dictionary;
    
    this._referenceFrame = this._dictionary ? this._dictionary.referenceFrame : "device";
	}
  
  _activate(){
    if (!sensorInstance) sensorInstance = new IMU_SENSOR(this._dictionary || {});
	}
  
  _deactivate() {
		this._sensor.close();
		delete this._sensor;
		delete this._sample;
	}
  get x() {
    if (this._sample){
      if (this._referenceFrame == "device" || CONFIG.rotation == 0) return this._sample.x;
      if (CONFIG.rotation == 270) return -this._sample.y;
      if (CONFIG.rotation == 180) return -this._sample.x;
      if (CONFIG.rotation == 90) return this._sample.y;
    }else{
      return null;
    }
	}
	get y() {
    if (this._sample){
      if (this._referenceFrame == "device" || CONFIG.rotation == 0) return this._sample.y;
      if (CONFIG.rotation == 270) return this._sample.x;
      if (CONFIG.rotation == 180) return -this._sample.y;
      if (CONFIG.rotation == 90) return -this._sample.x;
    }else{
      return null;
    }
	}
	get z() {
		return this._sample ? this._sample.z : null;
	}
}
Object.freeze(IMU.prototype);

export default IMU;
