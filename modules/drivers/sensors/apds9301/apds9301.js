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
    APDS-9301 Ambient Light Sensor
    Datasheet: https://docs.broadcom.com/docs/AV02-2315EN
*/

import Timer from "timer";

const Register = Object.freeze({
  CONTROL:        0x00,
  TIMING:         0x01,
  THRESHLOWLOW:   0x02,
  THRESHHIGHLOW:  0x04,
  INTERRUPT:      0x06,
  ID:             0x0A,
  DATA0LOW:       0x0C,
  DATA1LOW:       0x0E,
});

const Control = Object.freeze({
  POWEROFF:       0x0,
  POWERON:        0x3
});

const Timing = Object.freeze({
  GAINBIT:        0x10,
  MANUALBIT:      0x8,
  TIMINGBITS:     0x3,
  INTEG137:       0b00,
  INTEG101:       0b01,
  INTEG402:       0b10,
  INTEGMANUAL:    0b11,
});

const Command = Object.freeze({
  COMMAND:        0b10000000,
  CLEARINT:       0b01000000,
  WORD:           0b00100000
});

class Sensor {
  #io;
  #monitor;
  #onAlert;

  #configuration = {}

  constructor(options){
    this.#io = new options.sensor.io({
      hz: 400_000,
      address: 0x39,
      ...options.sensor
    });

    // Reset
    try {
      this.#writeByte(Register.CONTROL, Control.POWEROFF);
      this.#writeByte(Register.CONTROL, Control.POWERON, true);
    } catch {
      this.close();
      throw new Error("reset failed");
    }

    let idCheck = false;

    try {
      idCheck = (0x50 === this.#readByte(Register.ID));
    } catch {
      this.close();
      throw new Error("i2c error during id check");
    }

    if (!idCheck) {
      this.close();
      throw new Error("unexpected sensor ID");
    }

    const {alert, onAlert} = options;
    if (alert !== undefined && onAlert !== undefined) {
      this.#onAlert = onAlert;
      this.#monitor = new options.alert.io({
        mode: alert.io.InputPullUp,
        edge: alert.io.Falling,
        ...alert,
        onReadable: () => this.#onAlert()
      })
    }

    this.configure({
      highGain: false,
      manualTime: 0,
      integrationTime: 402,
      thresholdLow: 0,
      thresholdHigh: 0xFFFF,
      thresholdPersistence: 1
    });
  }
  
  configure(options){
    const {highGain, manualTime, integrationTime, thresholdLow, thresholdHigh, thresholdPersistence} = options;
    const configuration = this.#configuration;
    
    // Timing
    if (highGain !== undefined)
      configuration.highGain = (highGain === true);

    if (manualTime !== undefined)
      configuration.manualTime = manualTime;

    if (integrationTime !== undefined) {
      if (integrationTime !== 13.7 && integrationTime !== 101 && integrationTime !== 402)
        throw new RangeError("invalid integration time");
      configuration.integrationTime = integrationTime;
    }

    if (highGain !== undefined || manualTime !== undefined || integrationTime !== undefined) {
      let timing = 0;

      if (configuration.highGain)
        timing |= Timing.GAINBIT;

      if (configuration.manualTime > 0) {
        timing |= Timing.INTEGMANUAL;
      } else {
        if (configuration.integrationTime === 13.7) {
          timing |= Timing.INTEG137;
        } else if (configuration.integrationTime === 101) {
          timing |= Timing.INTEG101;
        } else if (configuration.integrationTime === 402) {
          timing |= Timing.INTEG402;
        } else {
          throw new RangeError("invalid integration time");
        }
      }

      this.#writeByte(Register.TIMING, timing);
    }
    
    // Alert Thresholds
    if (thresholdHigh !== undefined || thresholdLow !== undefined || thresholdPersistence !== undefined) {
      if (thresholdLow !== undefined) {
        if (thresholdLow < 0 || thresholdLow > 0xFFFF)
          throw new RangeError("invalid thresholdLow");
        configuration.thresholdLow = thresholdLow;
        this.#writeWord(Register.THRESHLOWLOW, thresholdLow, true);
      }

      if (thresholdHigh !== undefined) {
        if (thresholdHigh < 0 || thresholdHigh > 0xFFFF)
          throw new RangeError("invalid thresholdHigh");
        configuration.thresholdHigh = thresholdHigh;
        this.#writeWord(Register.THRESHHIGHLOW, thresholdHigh, true);
      }

      if (thresholdPersistence !== undefined) {
        if (thresholdPersistence < 0 || thresholdPersistence > 15)
          throw new RangeError("invalid thresholdPersistence");
        configuration.thresholdPersistence = thresholdPersistence;
      }
 
      if (configuration.thresholdHigh === 0xFFFF && configuration.thresholdLow === 0) {
        this.#writeByte(Register.INTERRUPT, configuration.thresholdPersistence & 0b1111, true);
      } else {
        this.#writeByte(Register.INTERRUPT, 0b00010000 | (configuration.thresholdPersistence & 0b1111), true);
      }
    }
  }

  sample(){
    const configuration = this.#configuration;

    if (configuration.manualTime) {
      let value = 0b00001011;
      if (configuration.highGain)
        value |= Timing.GAINBIT;
      this.#writeByte(Register.TIMING, value);
      value = value & 0b11110111;
      Timer.delay(configuration.manualTime);
      this.#writeByte(Register.TIMING, value);
    }

    let ch0 = this.#readWord(Register.DATA0LOW, true);
    let ch1 = this.#readWord(Register.DATA1LOW, true);
    
    const ratio = ch1/ch0;
    let scaler = 1;
    
    if (configuration.manualTime === 0) {
      switch (configuration.integrationTime){
        case Timing.INTEG137:
          if (ch0 >= 5047 || ch1 >= 5047) return {overflow: true};
          scaler = 1/0.034;
          break;
        case Timing.INTEG101:
          if (ch0 >= 37177 || ch1 >= 37177) return {overflow: true};
          scaler = 1/0.252;
          break;
        case Timing.INTEG402:
          if (ch0 >= 65535 || ch1 >= 65535) return {overflow: true};
          scaler = 1;
          break;
      }
    }else {
      if (ch0 >= 65535 || ch1 >= 65535) return {overflow: true};
      const t = (configuration.manualTime * 0.800653);
      scaler = 1 / (t / 322);
    }
    
    if (!configuration.highGain)
      scaler *= 16;
    
    const rawCH0 = ch0;
    ch0 *= scaler;
    ch1 *= scaler;
  
    let luxVal;
    
    if (ratio <= 0.5)
      luxVal = (0.0304 * ch0) - ((0.062 * ch0) * (Math.pow(ratio, 1.4)));
    else if (ratio <= 0.61)
      luxVal = (0.0224 * ch0) - (0.031 * ch1);
    else if (ratio <= 0.8)
      luxVal = (0.0128 * ch0) - (0.0153 * ch1);
    else if (ratio <= 1.3)
      luxVal = (0.00146 * ch0) - (0.00112*ch1);
    else
      luxVal = 0;
  
    return {
      illuminance: luxVal,
      raw: rawCH0,
      scaled: ch0,
      ir: ch1
    }
  }

  close() {
    if (this.#io) {
      this.#writeByte(Register.CONTROL, Control.POWEROFF);
      this.#io.close();
    }
    this.#monitor?.close();
    this.#monitor = this.#io = undefined;
  }

  get configuration() {
    return { ...this.#configuration};
  }

  get identification() {
    return {
      model: "Broadcom APDS9301",
      classification: "AmbientLight",
      revision: (this.#readByte(Register.ID) & 0b1111)
    }
  }

  //I2C Commands. APDS-9301 uses a slightly modified SMB-style protocol.

  #readByte(register, setClearBit) {
    let command = register | Command.COMMAND;
    if (setClearBit)
      command |= Command.CLEARINT;
    return this.#io.readUint8(command);
  }

  #writeByte(register, value, setClearBit) {
    let command = register | Command.COMMAND;
    if (setClearBit)
      command |= Command.CLEARINT;
    return this.#io.writeUint8(command, value);
  }

  #readWord(register, setClearBit) {
    let command = register | Command.COMMAND | Command.WORD;
    if (setClearBit)
      command |= Command.CLEARINT;
    return this.#io.readUint16(command);
  }

  #writeWord(register, value, setClearBit) {
    let command = register | Command.COMMAND | Command.WORD;
    if (setClearBit)
      command |= Command.CLEARINT;
    return this.#io.writeUint16(command, value);
  }
}

export default Sensor;
