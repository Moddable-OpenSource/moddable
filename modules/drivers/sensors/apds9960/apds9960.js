/*
 * Copyright (c) 2023 Moddable Tech, Inc.
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
    Broadcom APDS-9960 Proximity, Ambient Light, RGB, and Gesture Sensor
    Driver currently supports only Ambient Light and Proximity
    Datasheet: https://docs.broadcom.com/docs/AV02-4191EN
*/

const Register = Object.freeze({
  AILTL: 0x84,
  AIHTL: 0x86,
  PERS: 0x8C,
  ENABLE: 0x80,
  ATIME: 0x81,
  WTIME: 0x83,
  PILT: 0x89,
  PIHT: 0x8B,
  PROX_PULSE: 0x8E,
  CONTROLONE: 0x8F,
  CONFIGTWO: 0x90,
  ID: 0x92,
  STATUS: 0x93,
  CDATAL: 0x94,
  CDATAH: 0x95,
  PDATA: 0x9C,
  POFFSET_UR: 0x9D,
  POFFSET_DL: 0x9E,
  GPENTH: 0xA0,
  GEXTH: 0xA1,
  GPULSE: 0xA6,
  GFLVL: 0xAE,
  GSTATUS: 0xAF,
  GFIFO_U: 0xFC,
  GFIFO_D: 0xFD,
  GFIFO_L: 0xFE,
  GIFIO_R: 0xFF
});

class Sensor {
  #io;
  #monitor;
  #onAlert;
  #alsBlock;
  #alsView;
  #maxCount = 1025;

  #configuration = {
    enabled: {
      GEN: false,
      PIEN: false,
      AIEN: false,
      WEN: false,
      PEN: false,
      AEN: false,
      PON: false
    },
    alsIntegrationCycles: 1,
    alsGain: 1,
    proximityGain: 1
  }

  constructor(options){
    const io = new options.sensor.io({
      hz: 400_000,
      address: 0x39,
      ...options.sensor
    });

    // Reset
    try {
      io.writeUint8(Register.ENABLE, 0x00);
      io.writeUint8(Register.ENABLE, 0x01);
    } catch {
      io.close();
      throw new Error("reset failed");
    }

    this.#io = io;

    let idCheck = false;

    try {
      idCheck = (0xAB === this.#io.readUint8(Register.ID));
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
        onReadable: () => {
          this.#io.readUint8(0xE7); // have to do a fake read to clear the interrupt
          this.#onAlert();
        }
      })
    }

    this.#alsBlock = new ArrayBuffer(8);
    this.#alsView = new DataView(this.#alsBlock);

    this.configure({
      on: true,
      enableALS: true,
      enableProximity: true,
      enableGesture: true,
      proximityGain: 2,
      alsIntegrationCycles: 10,
      alsGain: 1,
      alsThresholdLow: 0,
      alsThresholdHigh: 0xFFFF,
      alsThresholdPersistence: 1,
      proximityThresholdPersistence: 3,
      proximityThresholdHigh: 0xFF,
      proximityThresholdLow: 0,
      gestureThresholdEnter: 70,
      gestureThresholdExit: 10,
      enableWait: true,
      waitTime: 2.78,
      LEDBoost: 100,
      proximityPulseLength: 32,
      proximityPulseCount:16,
      proximityOffsetUR: 0,
      proximityOffsetDL: 0,
      gesturePulseLength: 32,
      gesturePulseCount: 16
    });
  }
  
  configure(options){
    const {
          enableALS, on, alsIntegrationCycles, alsGain, proximityGain, alsThresholdHigh, 
          alsThresholdLow, alsThresholdPersistence, proximityThresholdPersistence, enableProximity, 
          proximityThresholdLow, proximityThresholdHigh, enableGesture, gestureThresholdEnter, gestureThresholdExit,
          enableWait, waitTime, LEDBoost, proximityPulseLength, proximityPulseCount, proximityOffsetUR, proximityOffsetDL,
          gesturePulseLength, gesturePulseCount} = options;
    const configuration = this.#configuration;
    const enabled = this.#configuration.enabled
    const io = this.#io;
    
    if (enableALS !== undefined) {
      enabled.AEN = enableALS;
    }

    if (enableProximity !== undefined) {
      enabled.PEN = enableProximity;
    }

    if (enableGesture !== undefined) {
      enabled.GEN = enableGesture;
    }

    if (enableWait !== undefined) {
      enabled.WEN = enableWait
    }

    if (on !== undefined) {
      enabled.PON = on;
    }

    if (alsGain !== undefined) {
      configuration.alsGain = alsGain;
    }

    if (proximityGain !== undefined) {
      configuration.proximityGain = proximityGain;
    }

    if (proximityPulseLength !== undefined) {
      if (proximityPulseLength !== 4 && proximityPulseLength !== 8 && proximityPulseLength !== 16 && proximityPulseLength !== 32)
        throw new RangeError("invalid proximityPulseLength");

      configuration.proximityPulseLength = proximityPulseLength;
    }

    if (proximityPulseCount !== undefined) {
      if (proximityPulseCount < 1 || proximityPulseCount > 64)
        throw new RangeError("invalid proximityPulseCount");

      configuration.proximityPulseCount = proximityPulseCount;
    }

    if (proximityPulseLength !== undefined || proximityPulseCount !== undefined) {
      const LENfield = (configuration.proximityPulseLength == 32 ? 3 : Math.floor(configuration.proximityPulseLength / 8)) << 6;
      const PULSEfield = configuration.proximityPulseCount - 1;
      const reg = LENfield | PULSEfield;
      io.writeUint8(Register.PROX_PULSE, reg);
    }

    if (proximityOffsetUR !== undefined) {
      if (proximityOffsetUR < -127 || proximityOffsetUR > 127)
        throw new RangeError("invalid proximityOffsetUR");

      let reg = Math.abs(proximityOffsetUR);
      if (proximityOffsetUR < 0)
        reg |= 0b10000000;
      io.writeUint8(Register.POFFSET_UR);
    }

    if (proximityOffsetDL !== undefined) {
      if (proximityOffsetDL < -127 || proximityOffsetDL > 127)
        throw new RangeError("invalid proximityOffsetDL");

      let reg = Math.abs(proximityOffsetDL);
      if (proximityOffsetDL < 0)
        reg |= 0b10000000;
      io.writeUint8(Register.POFFSET_DL);
    }

    if (waitTime !== undefined) { // does not yet support WAITLONG
      const closestValid = Math.round(waitTime / 2.78) * 2.78;

      if (closestValid < 2.78 || closestValid > 712)
        throw new RangeError("invalid waitTime setting");

      configuration.waitTime = closestValid;
      const value = 256 - Math.round(waitTime / 2.78);
      io.writeUint8(Register.WTIME, value);
    }

    if (LEDBoost !== undefined) {
      if (LEDBoost !== 100 && LEDBoost !== 150 && LEDBoost !== 200 && LEDBoost !== 300)
        throw new RangeError("invalid LED boost");
      
      configuration.LEDBoost = LEDBoost;
      const field = (LEDBoost === 100 ? 0 : Math.floor(LEDBoost / 100)) << 4;
      const value = 0b00000001 | field;
      io.writeUint8(Register.CONFIGTWO, value);
    }

    if (alsGain !== undefined || proximityGain !== undefined) {
      let regValue = 0;
      let fieldValue = 0;

      switch (configuration.alsGain) {
        case 1:
          fieldValue = 0b00;
          break;
        case 4:
          fieldValue = 0b01;
          break;
        case 16:
          fieldValue = 0b10;
          break;
        case 64:
          fieldValue = 0b11;
          break;
        default:
          throw new Error("invalid alsGain setting");
      }
      regValue |= fieldValue;

      switch (configuration.proximityGain) {
        case 1: 
          fieldValue = 0b0000;
          break;
        case 2:
          fieldValue = 0b0100;
          break;
        case 4:
          fieldValue = 0b1000;
          break;
        case 8:
          fieldValue = 0b1100;
          break;
        default:
          throw new Error("invalid proximityGain setting");
      }
      regValue |= fieldValue;

      io.writeUint8(Register.CONTROLONE, regValue);
    }

    if (alsIntegrationCycles !== undefined) {
      if (alsIntegrationCycles > 256 || alsIntegrationCycles < 0)
        throw new Error("invalid alsIntegrationCycles setting");

      configuration.alsIntegrationCycles = alsIntegrationCycles;
      const regValue = 256 - alsIntegrationCycles;
      this.#maxCount = 1025 * alsIntegrationCycles;
      io.writeUint8(Register.ATIME, regValue);
    }

    if (gesturePulseLength !== undefined) {
      if (gesturePulseLength !== 4 && gesturePulseLength !== 8 && gesturePulseLength !== 16 && gesturePulseLength !== 32)
        throw new RangeError("invalid gesturePulseLength");

      configuration.gesturePulseLength = gesturePulseLength;
    }

    if (gesturePulseCount !== undefined) {
      if (gesturePulseCount < 1 || gesturePulseCount > 64)
        throw new RangeError("invalid gesturePulseCount");

      configuration.gesturePulseCount = gesturePulseCount;
    }

    if (gesturePulseLength !== undefined || gesturePulseCount !== undefined) {
      const LENfield = (configuration.gesturePulseLength == 32 ? 3 : Math.floor(configuration.gesturePulseLength / 8)) << 6;
      const PULSEfield = configuration.gesturePulseCount - 1;
      const reg = LENfield | PULSEfield;
      io.writeUint8(Register.GPULSE, reg);
    }

    // Alert Thresholds
    if (alsThresholdLow !== undefined) {
      if (alsThresholdLow < 0 || alsThresholdLow > 0xFFFF)
        throw new RangeError("invalid alsThresholdLow");
      configuration.alsThresholdLow = alsThresholdLow;
      io.writeUint16(Register.AILTL, alsThresholdLow);
    }

    if (alsThresholdHigh !== undefined) {
      if (alsThresholdHigh < 0 || alsThresholdHigh > 0xFFFF)
        throw new RangeError("invalid thresholdHigh");
      configuration.alsThresholdHigh = alsThresholdHigh;
      io.writeUint16(Register.AIHTL, alsThresholdHigh);
    }

    if (proximityThresholdLow !== undefined) {
      if (proximityThresholdLow < 0 || proximityThresholdLow > 0xFF)
        throw new RangeError("invalid proximityThresholdLow");
      configuration.proximityThresholdLow = proximityThresholdLow;
      io.writeUint8(Register.PILT, proximityThresholdLow);
    }

    if (proximityThresholdHigh !== undefined) {
      if (proximityThresholdHigh < 0 || proximityThresholdHigh > 0xFF)
        throw new RangeError("invalid proximityThresholdHigh");
      configuration.proximityThresholdHigh = proximityThresholdHigh;
      io.writeUint8(Register.PIHT, proximityThresholdHigh);
    }

    if (proximityThresholdPersistence !== undefined || alsThresholdPersistence !== undefined) {
      let APERS, PPERS;
      if (alsThresholdPersistence !== undefined) {
        APERS = apersFromCycles(alsThresholdPersistence);
        if (APERS === undefined)
          throw new RangeError("invalid alsThresholdPersistence");
        configuration.alsThresholdPersistence = alsThresholdPersistence;
      } else {
        APERS = apersFromCycles(configuration.alsThresholdPersistence);
      }

      if (proximityThresholdPersistence !== undefined) {
        if (proximityThresholdPersistence < 0 || proximityThresholdPersistence > 15)
          throw new RangeError("invalid proximityThresholdPersistence");
        configuration.proximityThresholdPersistence = proximityThresholdPersistence;
      }
      PPERS = configuration.proximityThresholdPersistence << 4;

      io.writeUint8(Register.PERS, (PPERS | APERS));
    }

    if (alsThresholdLow !== undefined || alsThresholdHigh !== undefined) {      
      if (configuration.alsThresholdHigh === 0xFFFF && configuration.alsThresholdLow === 0) {
        configuration.enabled.AIEN = false;
      } else {
        configuration.enabled.AIEN = true;
      }
      io.readUint8(0xE7); // clear interrupts
    }

    if (proximityThresholdHigh !== undefined || proximityThresholdLow !== undefined) {
      if (configuration.proximityThresholdHigh === 0xFF && configuration.proximityThresholdLow === 0) {
        configuration.enabled.PIEN = false;
      } else {
        configuration.enabled.PIEN = true;
      }
      io.readUint8(0xE7); // clear interrupts
    }

    if (gestureThresholdEnter !== undefined) {
      if (gestureThresholdEnter < 0 || gestureThresholdEnter > 0xFF)
        throw new RangeError("invalid gestureThresholdEnter");
      if (gestureThresholdEnter & 0b00010000)
        throw new RangeError("Note: Bit 4 must be set to 0");
      configuration.gestureThresholdEnter = gestureThresholdEnter;
      io.writeUint8(Register.GPENTH, gestureThresholdEnter);
    }

    if (gestureThresholdExit !== undefined) {
      if (gestureThresholdExit < 0 || gestureThresholdExit > 0xFF)
        throw new RangeError("invalid gestureThresholdExit");

      configuration.gestureThresholdExit = gestureThresholdExit;
      io.writeUint8(Register.GEXTH, gestureThresholdExit);
    }

    if (on !== undefined || enableALS !== undefined || alsThresholdLow !== undefined || alsThresholdHigh !== undefined || proximityThresholdHigh !== undefined || proximityThresholdLow !== undefined) {
      let e = 0;

      if (enabled.GEN)
        e |= 0b01000000;

      if (enabled.PIEN)
        e |= 0b00100000;

      if (enabled.AIEN)
        e |= 0b00010000;

      if (enabled.WEN)
        e |= 0b00001000;

      if (enabled.PEN)
        e |= 0b00000100;

      if (enabled.AEN)
        e |= 0b00000010;

      if (enabled.PON)
        e |= 0b00000001;

      io.writeUint8(Register.ENABLE, e);
    }
  }

  sample(){
    const configuration = this.#configuration;
    const enabled = configuration.enabled;
    const io = this.#io;

    if (!enabled.PON)
      return undefined;

    const status = io.readUint8(Register.STATUS);

    let result = {};

    if (enabled.AEN && status & 0x01) {
      io.readBuffer(Register.CDATAL, this.#alsBlock);

      // APDS-9960 datasheet offers no guidance on converting from raw values to Lux, so "illuminance" is not provided in this driver's sample.
      // Values are instead [0,1] from darkness to max saturation, based on current settings.
      result.lightmeter = {
        clear: this.#alsView.getUint16(0, true) / this.#maxCount,
        red: this.#alsView.getUint16(2, true) / this.#maxCount,
        green: this.#alsView.getUint16(4, true) / this.#maxCount,
        blue: this.#alsView.getUint16(6, true) / this.#maxCount
      }
    }
    
    // APDS-9960 datasheet offers no guidance on converting from raw values to distance, so "distance" is not provided in this driver's sample.
    // `proximity` value is [0,1] from nothing sensed to max saturation
    if (enabled.PEN && status & 0b00000010) {
      result.proximity = {
        proximity: (io.readUint8(Register.PDATA)  / 0xFF)
      }
    }

    if (enabled.GEN) {
      const gStatus = io.readUint8(Register.GSTATUS);
      if (gStatus & 0x01) {
        const count = io.readUint8(Register.GFLVL);
        const data = io.readBuffer(Register.GFIFO_U, count * 4);
        const view = new DataView(data);

        if (count >= 4) {
          let first = [-1, -1, -1, -1];
          // let last = [-1, -1, -1, -1];  // not yet used
          const enter = configuration.gestureThresholdEnter;
          const exit = configuration.gestureThresholdExit;

          for (let i = 0; i < count; i++) {
            for (let j = 0; j < 4; j++) {
              const value = view.getUint8((4*i) + j);

              if (value >= enter && (first[j] == -1))
                first[j] = i;

              // if (value >= exit && (first[j] !== -1))
              //   last[j] = i;
            }
          }
          
          if (first.every(element => { return element >= 0; })) {
            const deltaUpDown = first[0] - first[1];
            const deltaLeftRight = first[2] - first[3];

            if (Math.abs(deltaUpDown) > Math.abs(deltaLeftRight)) {
              if (deltaUpDown < 0) {
                result.gestureDetector = {gesture: "UP"};
              } else {
                result.gestureDetector = {gesture: "DOWN"};
              }
            } else {
              if (deltaLeftRight < 0) {
                result.gestureDetector = {gesture: "LEFT"};
              } else {
                result.gestureDetector = {gesture: "RIGHT"};
              }
            }
          }
        }
      }
    }

    if (Object.keys(result).length > 0)
      return result;
  }

  close() {
    if (this.#io) {
      this.#io.writeUint8(Register.ENABLE, 0x00);
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
      model: "Broadcom APDS-9960",
      classification: "AmbientLight-Gesture-Proximity"
    }
  }
}

function apersFromCycles(cycles) {
  if (cycles < 0 || cycles > 60)
    return;

  if (cycles > 3 && ((cycles % 5) !== 0))
    return;

  if (cycles <= 3)
    return cycles;

  return (cycles / 5) + 3;
}

export default Sensor;
