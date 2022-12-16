/*
 * Copyright (c) 2022 Moddable Tech, Inc.
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
  #reading;
  #onAlert;

  constructor(options){
    const {trigger, echo, onAlert, triggerPin, echoPin} = options;

    this.#onAlert = onAlert;

    this.#output = new trigger({
      mode: trigger.Output,
      pin: triggerPin
    });

    this.#input = new echo({
      pin: echoPin,
      mode: echo.RisingToFalling,
      pullUpDown: echo.PullDown,
      onReadable: () => this.#onEcho()
    });

    this.configure({sampleRate: 500});
  }
  
  configure(options = {}){
    const {sampleRate} = options;

    if (undefined !== sampleRate) {
      Timer.clear(this.#timer);
      this.#timer = undefined;
        
      if (sampleRate !== 0) {
        this.#timer = Timer.repeat(() => {
          this.#output.write(1);
          this.#output.write(0);          
        }, sampleRate);
      }
    }
  }

  sample(){
    return this.#reading;
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

    if (value > 35000) {
      const doAlert = (this.#reading !== Number.POSITIVE_INFINITY);
      this.#reading = Number.POSITIVE_INFINITY;
      if (doAlert)
        this.#onAlert?.(this.#reading);  
    } else {
      this.#reading = value / 58;
      this.#onAlert?.(this.#reading);
    }
  }

}

export default Sensor;
