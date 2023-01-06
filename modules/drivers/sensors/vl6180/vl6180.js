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
	VL6180 Time-of-Flight Range Finder
		https://www.sparkfun.com/products/12785
    Datasheet: https://cdn.sparkfun.com/datasheets/Sensors/Proximity/DM00112632.pdf
    Application Note: https://www.st.com/content/ccc/resource/technical/document/application_note/d5/bb/ec/94/7d/1e/40/a0/DM00122600.pdf/files/DM00122600.pdf/jcr:content/translations/en.DM00122600.pdf
*/

const Register = Object.freeze({
	IDENTIFICATION__MODEL_ID:                 0x000,
  IDENTIFICATION__MODEL_REV_MAJOR:          0x001,
  IDENTIFICATION__MODEL_REV_MINOR:          0x002,
  IDENTIFICATION__MODULE_REV_MAJOR:         0x003,
  IDENTIFICATION__MODULE_REV_MINOR:         0x004,
  IDENTIFICATION__DATE_HI:                  0x006,
  IDENTIFICATION__DATE_LOW:                 0x007,
  IDENTIFICATION__TIME:                     0x008,
  SYSTEM__MODE_GPIO1:                       0x011,
  SYSTEM__INTERRUPT_CONFIG_GPIO:            0x014,
  SYSTEM__INTERRUPT_CLEAR:                  0x015,
  SYSTEM__FRESH_OUT_OF_RESET:               0x016,
  SYSRANGE__START:                          0x018,
  SYSRANGE__INTERMEASUREMENT_PERIOD:        0x01B,
  SYSRANGE__VHV_RECALIBRATE:                0x02E,
  SYSRANGE__VHV_REPEAT_RATE:                0x031,
  SYSALS__INTERMEASUREMENT_PERIOD:          0x03E,
  SYSALS__ANALOGUE_GAIN:                    0x03F,
  SYSALS__INTEGRATION_PERIOD:               0x040,
  RESULT__INTERRUPT_STATUS_GPIO:            0x04F,
  RESULT__RANGE_VAL:                        0x062,
  READOUT__AVERAGING_SAMPLE_PERIOD:         0x10A,
  RESULT__ALS_VAL:                          0x050,
  SYSALS__START:                            0x038,
  SYSRANGE__MAX_CONVERGENCE_TIME:           0x01C,
  SYSRANGE__CROSSTALK_COMPENSATION_RATE:    0x01E,
  SYSRANGE__CROSSTALK_VALID_HEIGHT:         0x021,
  SYSRANGE__EARLY_CONVERGENCE_ESTIMATE:     0x022
});

const MAX_RANGE = 254;
const MAX_RANGE_CM = 25.4;

const LUX_PER_COUNT = 0.32;

const initializationData = Object.freeze([
  0x0207, 0x01,
  0x0208, 0x01,
  0x0096, 0x00,
  0x0097, 0xfd,
  0x00e3, 0x00,
  0x00e4, 0x04,
  0x00e5, 0x02,
  0x00e6, 0x01,
  0x00e7, 0x03,
  0x00f5, 0x02,
  0x00d9, 0x05,
  0x00db, 0xce,
  0x00dc, 0x03,
  0x00dd, 0xf8,
  0x009f, 0x00,
  0x00a3, 0x3c,
  0x00b7, 0x00,
  0x00bb, 0x3c,
  0x00b2, 0x09,
  0x00ca, 0x09,
  0x0198, 0x01,
  0x01b0, 0x17,
  0x01ad, 0x00,
  0x00ff, 0x05,
  0x0100, 0x05,
  0x0199, 0x05,
  0x01a6, 0x1b,
  0x01ac, 0x3e,
  0x01a7, 0x1f,
  0x0030, 0x00
]);

class VL6180 {
  #io;
  #rangingMode = 0;
  #rangingFrequency = 90;
  #averagingSamplePeriod = 48;
  #maxConvergenceTime = 49;
  #crosstalkCompensationRate = 0;
  #crosstalkValidHeight = 20;
  #earlyConvergenceEstimate = 0;
  #enableSampleReadyPolling = false;
  #analogueGain = 1;
  #internalGain = LUX_PER_COUNT / 1.01;
  #alsMode = 0;
  #alsFrequency = 2550;
  #wordBuffer;
  #byteBuffer;
  #wordView;
  #byteView;
  
  constructor(options){
    const io = this.#io = new options.sensor.io({
      address: 0x29,
      hz: 400_000,
      ...options.sensor
    });

    this.#wordBuffer = new ArrayBuffer(2);
    this.#wordView = new DataView(this.#wordBuffer);

    this.#byteBuffer = new ArrayBuffer(1);
    this.#byteView = new DataView(this.#byteBuffer);
    
    let idCheck = false;

    try {
      idCheck = (0xB4 === this.#readByte(Register.IDENTIFICATION__MODEL_ID));
    } catch (e) {
      this.close();
      throw new Error("I2C error in ID check");
    }

    if (!idCheck) {
      this.close();
      throw new Error("unexpected sensor");
    }

    //mandatory settings from VL6180 Application Note
    for (let i = 0; i < initializationData.length; i += 2) 
      this.#writeByte(initializationData[i], initializationData[i+1]);
    
    //recommended default settings from VL6180 Application Note not covered by configure()
    this.#writeByte(Register.SYSRANGE__VHV_REPEAT_RATE, 0xFF); // sets the # of range measurements after which auto calibration of system is performed
    this.#writeWord(Register.SYSALS__INTEGRATION_PERIOD, 0x63); // Set ALS integration time to 100ms
    this.#writeByte(Register.SYSRANGE__VHV_RECALIBRATE, 0x01); // perform a single temperature calibration of the ranging sensor 

    this.configure( {
      averagingSamplePeriod: 0x30,
      analogueGain: 1,      
      rangingFrequency: 90,
      alsFrequency: 490,
      enableSampleReadyPolling: true
    });
  }

  configure(options = {}) {
    const {rangingMode, rangingFrequency, averagingSamplePeriod, maxConvergenceTime, crosstalkCompensationRate, crosstalkValidHeight, earlyConvergenceEstimate, enableSampleReadyPolling, analogueGain, alsMode, alsFrequency} = options;


    if (undefined !== rangingMode) {
      if (rangingMode < 0 || rangingMode > 1)
        throw new Error("invalid rangingMode");

      if (rangingMode === 1 && this.#rangingMode === 0) {
        this.#writeByte(Register.SYSRANGE__START, 0x03);
      } else if (rangingMode === 0 && this.#rangingMode === 1) {
        this.#writeByte(Register.SYSRANGE__START, 0x00);
      }
      this.#rangingMode = rangingMode;
    }

    if (undefined !== rangingFrequency) {
      if (rangingFrequency < 10 || rangingFrequency >= 2550)
        throw new Error("rangingFrequency out of range");
      
      const registerFrequency = Math.floor(rangingFrequency / 10);
      
      this.#rangingFrequency = registerFrequency * 10;
      this.#writeByte(Register.SYSRANGE__INTERMEASUREMENT_PERIOD, registerFrequency);
    }

    if (undefined !== averagingSamplePeriod) {
      if (averagingSamplePeriod < 0 || averagingSamplePeriod > 255)
        throw new Error("averagingSamplePeriod out of range");
      this.#averagingSamplePeriod = averagingSamplePeriod;
      this.#writeByte(Register.READOUT__AVERAGING_SAMPLE_PERIOD, averagingSamplePeriod);
    }

    if (undefined !== maxConvergenceTime) {
      if (maxConvergenceTime < 1 || maxConvergenceTime > 63)
        throw new Error("maxConvergenceTime out of range");
      this.#maxConvergenceTime = maxConvergenceTime;
      this.#writeByte(Register.SYSRANGE__MAX_CONVERGENCE_TIME, maxConvergenceTime);
    }

    if (undefined !== crosstalkCompensationRate) {
      this.#crosstalkCompensationRate = crosstalkCompensationRate;
      this.#writeByte(Register.SYSRANGE__CROSSTALK_COMPENSATION_RATE, crosstalkCompensationRate);
    }

    if (undefined !== crosstalkValidHeight) {
      this.#crosstalkValidHeight = crosstalkValidHeight;
      this.#writeByte(Register.SYSRANGE__CROSSTALK_VALID_HEIGHT, crosstalkValidHeight);
    }

    if (undefined !== earlyConvergenceEstimate) {
      this.#earlyConvergenceEstimate = earlyConvergenceEstimate;
      this.#writeByte(Register.SYSRANGE__EARLY_CONVERGENCE_ESTIMATE, earlyConvergenceEstimate);
    }

    if (undefined !== enableSampleReadyPolling) {
      if (enableSampleReadyPolling !== true && enableSampleReadyPolling !== false)
        throw new Error("enableSampleReadyPolling must be a Boolean");

      this.#enableSampleReadyPolling = enableSampleReadyPolling;
      this.#writeByte(Register.SYSTEM__MODE_GPIO1, enableSampleReadyPolling ?  0x10 : 0x00);
      this.#writeByte(Register.SYSTEM__INTERRUPT_CONFIG_GPIO, 0x24);
    }

    if (undefined !== analogueGain) {
      let value, internalGain;

      if (analogueGain === 40) {
        value = 0x47;
        internalGain = LUX_PER_COUNT / 40;
      } else if (analogueGain === 20) {
        value = 0x40;
        internalGain = LUX_PER_COUNT / 20;
      } else if (analogueGain === 10) {
        value = 0x41;
        internalGain = LUX_PER_COUNT / 10.32;
      } else if (analogueGain === 5) {
        value = 0x42;
        internalGain = LUX_PER_COUNT / 5.21;
      } else if (analogueGain === 2.5) {
        value = 0x43;
        internalGain = LUX_PER_COUNT / 2.6;
      } else if (analogueGain === 1.67) {
        value = 0x44;
        internalGain = LUX_PER_COUNT / 1.72;
      } else if (analogueGain === 1.25) {
        value = 0x45;
        internalGain = LUX_PER_COUNT / 1.28;
      } else if (analogueGain === 1) {
        value = 0x46;
        internalGain = LUX_PER_COUNT / 1.01;
      } else {
        throw new Error("invalid analogueGain");
      }
      this.#internalGain = internalGain;
      this.#analogueGain = analogueGain;
      this.#writeByte(Register.SYSALS__ANALOGUE_GAIN, value);
    }

    if (undefined !== alsMode) {
      if (alsMode < 0 || alsMode > 1)
        throw new Error("invalid alsMode");
      
      if (alsMode === 1 && this.#alsMode === 0) {
        this.#writeByte(Register.SYSALS__START, 0x03);
      } else if (alsMode === 0 && this.#alsMode === 1) {
        this.#writeByte(Register.SYSALS__START, 0x01);
      }

      this.#alsMode = alsMode;
    }

    if (undefined !== alsFrequency) {
      if (alsFrequency < 10 || alsFrequency > 2550)
        throw new Error("alsFrequency out of range");
      
      const registerFrequency = Math.floor(alsFrequency / 10);
    
      this.#alsFrequency = registerFrequency * 10;
      this.#writeByte(Register.SYSALS__INTERMEASUREMENT_PERIOD, registerFrequency);
    }
  }

  get configuration() {
    return {
      rangingMode: this.#rangingMode,
      rangingFrequency: this.#rangingFrequency,
      averagingSamplePeriod: this.#averagingSamplePeriod,
      maxConvergenceTime: this.#maxConvergenceTime,
      crosstalkCompensationRate: this.#crosstalkCompensationRate,
      crosstalkValidHeight: this.#crosstalkValidHeight,
      earlyConvergenceEstimate: this.#earlyConvergenceEstimate,
      enableSampleReadyPolling: this.#enableSampleReadyPolling,
      analogueGain: this.#analogueGain,
      alsMode: this.#alsMode,
      alsFrequency: this.#alsFrequency
    }
  }

  get identification() {
    return {
      model: "ST VL6180X",
      classification: "AmbientLight-Proximity",
      revision: {
        modelID: this.#readByte(Register.IDENTIFICATION__MODEL_ID),
        modelRevMajor: this.#readByte(Register.IDENTIFICATION__MODEL_REV_MAJOR) & 0b111,
        modelRevMinor: this.#readByte(Register.IDENTIFICATION__MODEL_REV_MINOR) & 0b111,
        moduleRevMajor: this.#readByte(Register.IDENTIFICATION__MODULE_REV_MAJOR) & 0b111,
        moduleRevMinor: this.#readByte(Register.IDENTIFICATION__MODULE_REV_MINOR) & 0b111,
      },
      uniqueID: (this.#readByte(Register.IDENTIFICATION__DATE_HI) << 24) | (this.#readByte(Register.IDENTIFICATION__DATE_LOW) << 16) | this.#readWord(Register.IDENTIFICATION__TIME)
    }
  }

  sample() {
    if (this.#rangingMode === 0) {
      this.#writeByte(Register.SYSRANGE__START, 0x01);
    }
    let thresholdEvent = 0;
    while (!(thresholdEvent & 0b100)) {
      thresholdEvent = this.#readByte(Register.RESULT__INTERRUPT_STATUS_GPIO);
    }
    this.#writeByte(Register.SYSTEM__INTERRUPT_CLEAR, 0x07);
    let distance = this.#readByte(Register.RESULT__RANGE_VAL);

    if (this.#alsMode === 0) {
      this.#writeByte(Register.SYSALS__START, 0x01);
    }
    thresholdEvent = 0;
    while (!(thresholdEvent & 0b100000)) {
      thresholdEvent = this.#readByte(Register.RESULT__INTERRUPT_STATUS_GPIO);
    }
    const illuminance = this.#readWord(Register.RESULT__ALS_VAL) * this.#internalGain;
    this.#writeByte(Register.SYSTEM__INTERRUPT_CLEAR, 0x07);
    
    return {
      proximity: {
        distance: (distance / 10),
        near: (distance <= MAX_RANGE),
        max: MAX_RANGE_CM
      },
      lightmeter: {
        illuminance
      }
    };
  }

  close() {
    this.#io?.close();
    this.#io = undefined;
  }

  #setupRead(register) { //Not quite SMB.
    this.#wordView.setUint16(0, register, false);
    this.#io.write(this.#wordBuffer);
  }

  #readByte(register) {
    this.#setupRead(register);
    this.#io.read(this.#byteBuffer);

    return this.#byteView.getUint8(0);
  }

  #readWord(register) {
    this.#setupRead(register);;
    this.#io.read(this.#wordBuffer);

    return this.#wordView.getUint16(0, false);
  }

  #writeByte(register, value) {
    let lowByte = register & 0xFF;
    let highByte = register >> 8;
    this.#io.write(Uint8Array.of(highByte, lowByte, value));
  }

  #writeWord(register, value) {
    let lowByte = register & 0xFF;
    let highByte = (register >> 8) & 0xFF;
    let lowValue = value & 0xFF;
    let highValue = (value >> 8) & 0xFF;

    this.#io.write(Uint8Array.of(highByte, lowByte, highValue, lowValue));
  }
}

export default VL6180;