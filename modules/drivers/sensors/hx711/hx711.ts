// Avia HX711 24-bit ADC for weight scales
// Copyright © 2023 by Thorsten von Eicken.

import Time from "time"
import HX711c from "embedded:sensor/ADC/HX711c"

export interface Options {
  sensor: {
    clk: number
    din: number
  }
  onError?: (error: string) => void

  // gain selects the gain of the ADC as well as the analog input channel
  gain?: number // 1:128chA, 2:32chB, 3:64chA (default: 1)
}

export default class HX711 {
  #hx711c: HX711c

  constructor(options) {
    this.configure(options)
  }

  close() {
    this.#hx711c = undefined
  }

  // To "configure" the HX711 we have to perform a read, which ends up setting up the gain
  // for the next read
  configure(options: Options) {
    this.#hx711c = new HX711c(options.sensor.clk, options.sensor.din, options.gain || 1)
    this.#hx711c.read()
  }

  get format() {
    return "number"
  }

  readable() {
    //return this.#din.read() == 0
  }

  read() {
    return this.#hx711c.read()
  }

  /* read() has to be implemented in C to meet the timing requirements (clk high < 50us)

  // read the 24-bit signed value from the sensor and perform additional pulses to set-up the gain
  read() {
    const clk_w = this.#clk.write.bind(this.#clk)
    const din_r = this.#din.read.bind(this.#din)
    if (din_r() != 0) return undefined
    // read 24 bits: din is stable 100ns after clk rising edge until next rising edge, so we read
    // after producing the falling edge
    // ugh: clk high must be less than 50us or the chip enters power-down mode
    let val = 0
    clk_w(0) // preload caches...
    for (let i = 24; i > 0; i--) {
      clk_w(1)
      clk_w(0)
      val = (val << 1) | (din_r() & 1)
    }
    // sign extend
    val = (val << 8) >> 8
    if (val == -1) return undefined // chip entered power-down mode
    // pulse the clock to set the gain
    for (let i = this.#gain; i > 0; i--) {
      clk_w(1)
      clk_w(0)
    }
    return val
  }
*/
}
