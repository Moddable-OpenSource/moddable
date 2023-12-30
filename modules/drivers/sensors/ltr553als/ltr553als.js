/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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
    LTR553ALS  Light & Proximity Sensor
	https://optoelectronics.liteon.com/upload/download/DS86-2014-0007/LTR-553ALS-01_DS_V1.pdf
*/

const Register = Object.freeze({
  ALS_CONTR: 0x80,
  PS_CONTR: 0x81,
  PS_LED: 0x82,
  PS_N_PULSES: 0x83,
  PS_MEAS_RATE: 0x84,
  ALS_MEAS_RATE: 0x85,
  PART_ID: 0x86,
  MANUFAC_ID: 0x87,
  ALS_DATA_CH1_0: 0x88,
  ALS_DATA_CH1_1: 0x89,
  ALS_DATA_CH0_0: 0x8a,
  ALS_DATA_CH0_1: 0x8b,
  ALS_PS_STATUS: 0x8c,
  PS_DATA_LOW: 0x8d,
  PS_DATA_HIGH: 0x8e,
  INTERRUPT: 0x8f,
  PS_THRES_UP_0: 0x90,
  PS_THRES_UP_1: 0x91,
  PS_THRES_LOW_0: 0x92,
  PS_THRES_LOW_1: 0x93,
  ALS_THRES_UP_0: 0x97,
  ALS_THRES_UP_1: 0x98,
  ALS_THRES_LOW_0: 0x99,
  ALS_THRES_LOW_1: 0x9a,
  INTERRUPT_PERSIST: 0x9e,
});

const ALS_MODE_MASK = 0x01;
const ALS_MODE_SHIFT = 0;
const ALS_GAIN_MASK = 0x1c;
const ALS_GAIN_SHIFT = 2;
const ALS_INTEGRATION_TIME_MASK = 0x38;
const ALS_INTEGRATION_TIME_SHIFT = 3;
const ALS_MEASURE_RATE_MASK = 0x07;
const ALS_MEASURE_RATE_SHIFT = 0;
const ALS_DATA_LOW_MASK = 0xff;
const ALS_DATA_HIGH_MASK = 0xff;
const PS_PULSES_MASK = 0x0f;
const PS_MEAS_RATE_MASK = 0x0f;
const VALID_PS_DATA_MASK = 0x80;
const VALID_PS_DATA_SHIFT = 7;
const PS_MODE_MASK = 0x02;
const PS_MODE_SHIFT = 1;
const LED_PULSE_FREQ_MASK = 0xe0;
const LED_PULSE_FREQ_SHIFT = 5;
const LED_DUTY_CYCLE_MASK = 0x18;
const LED_DUTY_CYCLE_SHIFT = 3;
const LED_PEAK_CURRENT_MASK = 0x07;
const PS_DATA_LOW_MASK = 0xff;
const PS_DATA_HIGH_MASK = 0x07;
const ALS_INTERRUPT_STATUS_MASK = 0x08;
const ALS_INTERRUPT_STATUS_SHIFT = 3;
const PS_INTERRUPT_STATUS_MASK = 0x02;
const PS_INTERRUPT_STATUS_SHIFT = 1;
const INTERRUPT_POLARITY_MASK = 0x04;
const INTERRUPT_POLARITY_SHIFT = 2;
const INTERRUPT_MODE_MASK = 0x03;
const PS_THRES_UP_1_MASK = 0x07;
const PS_THRES_LOW_1_MASK = 0x07;
const PS_INTERRUPT_PERSIST_MASK = 0xf0;
const PS_INTERRUPT_PERSIST_SHIFT = 4;
const ALS_INTERRUPT_PERSIST_MASK = 0x0f;

export const AlsMode = {
  ALS_STAND_BY_MODE: 0,
  ALS_ACTIVE_MODE: 1,
};

export const PsMode = {
  PS_STAND_BY_MODE: 0,
  PS_ACTIVE_MODE: 1,
};

export const PsLedPulseFreq = {
  LED_PULSE_FREQ_30KHZ: 0x00,
  LED_PULSE_FREQ_40KHZ: 0x01,
  LED_PULSE_FREQ_50KHZ: 0x02,
  LED_PULSE_FREQ_60KHZ: 0x03,
  LED_PULSE_FREQ_70KHZ: 0x04,
  LED_PULSE_FREQ_80KHZ: 0x05,
  LED_PULSE_FREQ_90KHZ: 0x06,
  LED_PULSE_FREQ_100KHZ: 0x07,
};

export const PsLedPeakCurrent = {
  LED_PEAK_CURRENT_5MA: 0x00,
  LED_PEAK_CURRENT_10MA: 0x01,
  LED_PEAK_CURRENT_20MA: 0x02,
  LED_PEAK_CURRENT_50MA: 0x03,
  LED_PEAK_CURRENT_100MA: 0x04,
};

export const PsLedCurrentDuty = {
  CURRENT_DUTY_PER25: 0x00,
  CURRENT_DUTY_PER50: 0x01,
  CURRENT_DUTY_PER75: 0x02,
  CURRENT_DUTY_PER100: 0x03,
};

export const PsMeasurementRate = {
  PS_MEASUREMENT_RATE_10MS: 0x08,
  PS_MEASUREMENT_RATE_50MS: 0x00,
  PS_MEASUREMENT_RATE_70MS: 0x01,
  PS_MEASUREMENT_RATE_100MS: 0x02,
  PS_MEASUREMENT_RATE_200MS: 0x03,
  PS_MEASUREMENT_RATE_500MS: 0x04,
  PS_MEASUREMENT_RATE_1000MS: 0x05,
  PS_MEASUREMENT_RATE_2000MS: 0x06,
};

export const AlsGain = {
  ALS_GAIN_1X: 0x00,
  ALS_GAIN_2X: 0x01,
  ALS_GAIN_4X: 0x02,
  ALS_GAIN_8X: 0x03,
  ALS_GAIN_48X: 0x06,
  ALS_GAIN_96X: 0x07,
};

export const AlsIntegrationTiime = {
  ALS_INTEGRATION_TIME_50MS: 0x01,
  ALS_INTEGRATION_TIME_100MS: 0x00,
  ALS_INTEGRATION_TIME_150MS: 0x04,
  ALS_INTEGRATION_TIME_200MS: 0x02,
  ALS_INTEGRATION_TIME_250MS: 0x05,
  ALS_INTEGRATION_TIME_300MS: 0x06,
  ALS_INTEGRATION_TIME_350MS: 0x07,
  ALS_INTEGRATION_TIME_400MS: 0x03,
};

export const AlsMeasureRate = {
  ALS_MEASUREMENT_RATE_50MS: 0x00,
  ALS_MEASUREMENT_RATE_100MS: 0x01,
  ALS_MEASUREMENT_RATE_200MS: 0x02,
  ALS_MEASUREMENT_RATE_500MS: 0x03,
  ALS_MEASUREMENT_RATE_1000MS: 0x04,
  ALS_MEASUREMENT_RATE_2000MS: 0x05,
};

class LTR553ALS {
  #io;

  #onError;

  constructor(options) {
    const io = (this.#io = new options.sensor.io({
      hz: 400000,
      address: 0x23,
      ...options.sensor,
    }));

    try {
      if (
        0x92 !== io.readUint8(Register.PART_ID) ||
        0x05 !== io.readUint8(Register.MANUFAC_ID)
      )
        throw new Error("unexpected sensor");

      this.#io = io;
    } catch (e) {
      io.close();
      throw e;
    }
    this.configure({
      psLedPulseFreq: PsLedPulseFreq.LED_PULSE_FREQ_40KHZ,
      psLedDutyCycle: PsLedCurrentDuty.CURRENT_DUTY_PER100,
      psMeasRate: PsMeasurementRate.PS_MEASUREMENT_RATE_50MS,
      psLedPeakCurrent: PsLedPeakCurrent.LED_PEAK_CURRENT_100MA,
      psNPulses: 1,
      alsGain: AlsGain.ALS_GAIN_48X,
      alsIntegrationTime: AlsIntegrationTiime.ALS_INTEGRATION_TIME_100MS,
      alsMeasureRate: AlsMeasureRate.ALS_MEASUREMENT_RATE_500MS,
    });

    this.#onError = options.onError;
  }

  #setPsMode(mode) {
    const io = this.#io;
    let value = io.readUint8(Register.PS_CONTR);
    io.writeUint8(
      Register.PS_CONTR,
      (value & ~PS_MODE_MASK) | ((mode << PS_MODE_SHIFT) & PS_MODE_MASK)
    );
  }

  #setAlsMode(mode) {
    const io = this.#io;
    let value = io.readUint8(Register.ALS_CONTR);
    io.writeUint8(
      Register.ALS_CONTR,
      (value & ~ALS_MODE_MASK) | ((mode << ALS_MODE_SHIFT) & ALS_MODE_MASK)
    );
  }

  configure(options) {
    const io = this.#io;

    this.#setAlsMode(AlsMode.ALS_STAND_BY_MODE);
    this.#setPsMode(PsMode.PS_STAND_BY_MODE);

    if (undefined != options.psLedPulseFreq) {
      let value = io.readUint8(Register.PS_LED);
      io.writeUint8(
        Register.PS_LED,
        (value & ~LED_PULSE_FREQ_MASK) |
          ((options.psLedPulseFreq << LED_PULSE_FREQ_SHIFT) &
            LED_PULSE_FREQ_MASK)
      );
    }

    if (undefined != options.psLedDutyCycle) {
      let value = io.readUint8(Register.PS_LED);
      io.writeUint8(
        Register.PS_LED,
        (value & ~LED_DUTY_CYCLE_MASK) |
          ((options.psLedDutyCycle << LED_DUTY_CYCLE_SHIFT) &
            LED_DUTY_CYCLE_MASK)
      );
    }

    if (undefined != options.psMeasRate) {
      io.writeUint8(
        Register.PS_MEAS_RATE,
        options.psMeasRate & PS_MEAS_RATE_MASK
      );
    }

    if (undefined != options.psLedPeakCurrent) {
      let value = io.readUint8(Register.PS_LED);
      io.writeUint8(
        Register.PS_LED,
        (value & ~LED_PEAK_CURRENT_MASK) |
          (options.psLedPeakCurrent & LED_PEAK_CURRENT_MASK)
      );
    }

    if (undefined != options.psNPulses) {
      io.writeUint8(Register.PS_N_PULSES, options.psNPulses & PS_PULSES_MASK);
    }

    if (undefined != options.alsGain) {
      let value = io.readUint8(Register.ALS_CONTR);
      io.writeUint8(
        Register.ALS_CONTR,
        (value & ~ALS_GAIN_MASK) |
          ((options.alsGain << ALS_GAIN_SHIFT) & ALS_GAIN_MASK)
      );
    }

    if (undefined != options.alsIntegrationTime) {
      let value = io.readUint8(Register.ALS_MEAS_RATE);
      io.writeUint8(
        Register.ALS_MEAS_RATE,
        (value & ~ALS_INTEGRATION_TIME_MASK) |
          ((options.alsIntegrationTime << ALS_INTEGRATION_TIME_SHIFT) &
            ALS_INTEGRATION_TIME_MASK)
      );
    }

    if (undefined != options.alsMeasureRate) {
      let value = io.readUint8(Register.ALS_MEAS_RATE);
      io.writeUint8(
        Register.ALS_MEAS_RATE,
        (value & ~ALS_MEASURE_RATE_MASK) |
          ((options.alsMeasureRate << ALS_MEASURE_RATE_SHIFT) &
            ALS_MEASURE_RATE_MASK)
      );
    }

    if (undefined != options.psMeasRate) {
      let value = io.readUint8(Register.ALS_MEAS_PATE_REG);
      io.writeUint8(Register.ALS_MEAS_PATE_REG, value & PS_MEAS_RATE_MASK);
    }

    this.#setPsMode(PsMode.PS_ACTIVE_MODE);
    this.#setAlsMode(AlsMode.ALS_ACTIVE_MODE);
  }
  close() {
    this.#io.close();
    this.#io = undefined;
  }

  get psValue() {
    const buffer = new Uint8Array(2);
    this.#io.readBuffer(Register.PS_DATA_LOW, buffer);
    return (
      ((buffer[1] & PS_DATA_HIGH_MASK) << 8) | (buffer[0] & PS_DATA_LOW_MASK)
    );
  }

  get alsCh0Value() {
    const buffer = new Uint8Array(2);
    this.#io.readBuffer(Register.ALS_DATA_CH0_0, buffer);
    return (buffer[1] << 8) | buffer[0];
  }

  get alsCh1Value() {
    const buffer = new Uint8Array(2);
    this.#io.readBuffer(Register.ALS_DATA_CH1_0, buffer);
    return (buffer[1] << 8) | buffer[0];
  }

  sample() {
    const ps = this.psValue;
    const alsCh1 = this.alsCh1Value;
    const alsCh0 = this.alsCh0Value;
    const als = (alsCh0 + alsCh1) >> 1;

    return {
      ps,
      als,
    };
  }
}

export default LTR553ALS;
