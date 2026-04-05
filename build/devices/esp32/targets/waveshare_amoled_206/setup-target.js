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

import AXP2101 from 'embedded:peripheral/Power/axp2101'
import Digital from 'embedded:io/digital'
import Timer from 'timer'

globalThis.Host = {
  Backlight: class {
    constructor(brightness = 100) {
      this.write(brightness)
    }
    write(value) {
      if (globalThis.screen) globalThis.screen.brightness = value
    }
    close() {}
  }
}

export default function (done) {
  globalThis.power = new AXP2101({
    sensor: {
      ...device.I2C.default,
      io: device.io.SMBus
    },
    address: 0x34
  })

  globalThis.power.writeByte(0x30, 0x0f)

  // ES8311 codec (speaker DAC) initialization
  const es8311 = new device.io.SMBus({
    ...device.I2C.default,
    hz: 400_000,
    address: 0x18
  })
  es8311.writeUint8(0x00, 0x1f) // reset
  Timer.delay(20)
  es8311.writeUint8(0x00, 0x00)
  es8311.writeUint8(0x00, 0x80) // power on
  es8311.writeUint8(0x01, 0x3f) // use MCLK from pin, enable all clocks
  es8311.writeUint8(0x02, 0x00) // pre_div=1, no multiplier (MCLK = sampleRate*256)
  es8311.writeUint8(0x03, 0x20) // ADC osr=32 (for 256x MCLK ratio)
  es8311.writeUint8(0x04, 0x20) // DAC osr=32
  es8311.writeUint8(0x05, 0x00) // adc_div=1, dac_div=1
  es8311.readUint8(0x06)
  es8311.writeUint8(0x06, 0x07) // bclk_div=8 (MCLK/BCLK = 256/32 = 8)
  es8311.readUint8(0x07)
  es8311.writeUint8(0x07, 0x00) // lrck_h
  es8311.writeUint8(0x08, 0xff) // lrck_l
  let reg00 = es8311.readUint8(0x00)
  es8311.writeUint8(0x00, reg00 & 0xbf) // slave mode
  es8311.writeUint8(0x09, 0x0c) // SDP in: 16-bit, I2S Philips format
  es8311.writeUint8(0x0a, 0x0c) // SDP out: 16-bit, I2S Philips format
  es8311.writeUint8(0x0d, 0x01) // power up analog
  es8311.writeUint8(0x0e, 0x02) // enable PGA, ADC modulator
  es8311.writeUint8(0x12, 0x00) // power up DAC
  es8311.writeUint8(0x13, 0x10) // enable HP drive output
  es8311.writeUint8(0x14, 0x1a) // enable analog MIC, max PGA gain
  es8311.writeUint8(0x1c, 0x6a) // ADC EQ bypass, cancel DC offset
  es8311.writeUint8(0x37, 0x08) // DAC EQ bypass
  es8311.writeUint8(0x32, 0xd0) // DAC volume ~80%
  es8311.close()

  // ES7210 codec (microphone ADC) initialization
  const es7210 = new device.io.SMBus({
    ...device.I2C.default,
    hz: 400_000,
    address: 0x40
  })
  es7210.writeUint8(0x00, 0xff) // reset
  const micInit = [
    [0x00, 0x41],
    [0x01, 0x1f], // clock on
    [0x06, 0x00],
    [0x07, 0x20], // ADC OSR
    [0x08, 0x10],
    [0x09, 0x30],
    [0x0a, 0x30],
    [0x20, 0x0a], // HPF
    [0x21, 0x2a],
    [0x22, 0x0a],
    [0x23, 0x2a],
    [0x02, 0xc1],
    [0x04, 0x01],
    [0x05, 0x00],
    [0x11, 0x60],
    [0x40, 0x42], // analog sys
    [0x41, 0x70], // mic bias
    [0x42, 0x70],
    [0x43, 0x1b], // mic1 gain
    [0x44, 0x1b], // mic2 gain
    [0x45, 0x00],
    [0x46, 0x00],
    [0x47, 0x00],
    [0x48, 0x00],
    [0x49, 0x00],
    [0x4a, 0x00],
    [0x4b, 0x00], // mic1/2 power on
    [0x4c, 0xff], // mic3/4 power down
    [0x01, 0x14] // clock
  ]
  for (const [reg, value] of micInit) es7210.writeUint8(reg, value)
  es7210.close()

  // Enable speaker power amplifier
  const paEnable = new Digital({
    pin: 46,
    mode: Digital.Output
  })
  paEnable.write(1)

  done?.()
}
