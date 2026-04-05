/*
 * Copyright (c) 2024-2026  Moddable Tech, Inc.
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
	QMI8658 6-axis IMU (Accelerometer + Gyroscope)
	Datasheet: https://www.qstcorp.com/upload/pdf/202301/13-52-25%20QMI8658A%20Datasheet%20Rev%20A1.pdf
*/

import Timer from 'timer'

const REGISTERS = Object.freeze({
  WHO_AM_I: 0x00,
  REVISION: 0x01,
  CTRL1: 0x02,
  CTRL2: 0x03,
  CTRL3: 0x04,
  CTRL4: 0x05,
  CTRL5: 0x06,
  CTRL6: 0x07,
  CTRL7: 0x08,
  CTRL8: 0x09,
  CTRL9: 0x0a,
  STATUS0: 0x46,
  STATUS1: 0x47,
  TEMP_L: 0x33,
  TEMP_H: 0x34,
  AX_L: 0x35,
  AX_H: 0x36,
  AY_L: 0x37,
  AY_H: 0x38,
  AZ_L: 0x39,
  AZ_H: 0x3a,
  GX_L: 0x3b,
  GX_H: 0x3c,
  GY_L: 0x3d,
  GY_H: 0x3e,
  GZ_L: 0x3f,
  GZ_H: 0x40,
  RESET: 0x60
})

const EXPECTED_WHO_AM_I = 0x05

const ACCEL_SCALE = Object.freeze({
  AFS_2G: 2.0 / 32768.0,
  AFS_4G: 4.0 / 32768.0,
  AFS_8G: 8.0 / 32768.0,
  AFS_16G: 16.0 / 32768.0
})

const GYRO_SCALE = Object.freeze({
  GFS_16DPS: 16.0 / 32768.0,
  GFS_32DPS: 32.0 / 32768.0,
  GFS_64DPS: 64.0 / 32768.0,
  GFS_128DPS: 128.0 / 32768.0,
  GFS_256DPS: 256.0 / 32768.0,
  GFS_512DPS: 512.0 / 32768.0,
  GFS_1024DPS: 1024.0 / 32768.0,
  GFS_2048DPS: 2048.0 / 32768.0
})

class QMI8658 {
  #io
  #view
  #accelScale = ACCEL_SCALE.AFS_8G
  #gyroScale = GYRO_SCALE.GFS_512DPS

  constructor(options) {
    const io = (this.#io = new options.sensor.io({
      hz: 400_000,
      address: 0x6b,
      ...options.sensor
    }))

    this.#view = new DataView(new ArrayBuffer(14))

    const id = io.readUint8(REGISTERS.WHO_AM_I)
    if (id !== EXPECTED_WHO_AM_I) throw new Error('unexpected QMI8658 id: ' + id)

    io.writeUint8(REGISTERS.RESET, 0xb0)
    Timer.delay(20)

    io.writeUint8(REGISTERS.CTRL1, 0x40)
    io.writeUint8(REGISTERS.CTRL2, 0x15)
    io.writeUint8(REGISTERS.CTRL3, 0x55)
    io.writeUint8(REGISTERS.CTRL5, 0x00)
    io.writeUint8(REGISTERS.CTRL7, 0x03)

    Timer.delay(50)
  }

  close() {
    this.#io?.close()
    this.#io = undefined
  }

  configure(dictionary) {
    for (let property in dictionary) {
      switch (property) {
        case 'ACCEL_SCALE':
          this.#accelScale = dictionary.ACCEL_SCALE
          break
        case 'GYRO_SCALE':
          this.#gyroScale = dictionary.GYRO_SCALE
          break
      }
    }
  }

  sample() {
    const io = this.#io
    const view = this.#view

    io.readBuffer(REGISTERS.TEMP_L, view)

    const rawTemp = view.getInt16(0, true)

    return {
      thermometer: {
        temperature: rawTemp / 256.0
      },
      accelerometer: {
        x: view.getInt16(2, true) * this.#accelScale,
        y: view.getInt16(4, true) * this.#accelScale,
        z: view.getInt16(6, true) * this.#accelScale
      },
      gyroscope: {
        x: view.getInt16(8, true) * this.#gyroScale,
        y: view.getInt16(10, true) * this.#gyroScale,
        z: view.getInt16(12, true) * this.#gyroScale
      }
    }
  }
}

QMI8658.ACCEL_SCALE = ACCEL_SCALE
QMI8658.GYRO_SCALE = GYRO_SCALE

export default QMI8658
