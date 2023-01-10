/*
 * Copyright (c) 2022-2023 Moddable Tech, Inc.
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
    HC-SR04 Sonic Range Finder
*/

import Timer from "timer";

class Sensor {
  #input;
  #output;
  #timer;
  #reading = {
    near: false,
    distance: null,
    max: 400
  }
  #onAlert;
  #readingStale = true;
  #suppressDuplicates = false;

  constructor(options){

    const {trigger, sensor, onAlert} = options;

    this.#onAlert = onAlert;
    
    this.#input = new sensor.io({
      pin: sensor.pin,
      edges: sensor.io.RisingToFalling,
      mode: sensor.io.InputPullDown,
      onReadable: () => this.#onEcho()
    });

    if (trigger !== undefined) {
      this.#output = new trigger.io({
        mode: trigger.io.Output,
        pin: trigger.pin
      });
      
      this.configure({interval: 500, suppressDuplicates: true});
    }
  }
  
  configure(options = {}){
    const {interval, suppressDuplicates} = options;

    if (undefined !== interval) {
      if (this.#output === undefined)
        throw new Error("Configuring interval is only available on instances constructed with a trigger pin.");

      Timer.clear(this.#timer);
      this.#timer = undefined;
        
      if (interval !== 0) {
        this.#timer = Timer.repeat(() => {
          this.#output.write(1);
          this.#output.write(0);          
        }, interval);
      }
    }

    if (undefined !== suppressDuplicates) {
      this.#suppressDuplicates = suppressDuplicates;
    }
  }

  sample(){
    if (!this.#readingStale) {
      this.#readingStale = true;
      return this.#reading;
    }

    return undefined;
  }

  close() {
    Timer.clear(this.#timer);
    this.#timer = undefined;
    
    this.#input?.close()
    this.#output?.close();
    this.#input = this.#output = undefined;
  }

  #onEcho() {
    const value = this.#input.read();
    this.#readingStale = false;

    if (value > 35000) {
      const doAlert = (!(this.#suppressDuplicates) || (this.#reading.near !== false));
      this.#reading.near = false;
      this.#reading.distance = null;
      if (doAlert)
        this.#onAlert?.();  
    } else {
      this.#reading.near = true;
      this.#reading.distance = value / 58;
      this.#onAlert?.();
    }
  }

}

export default Sensor;
