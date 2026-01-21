// ssd1306-io - driver for a SSD1306 display using embedded:io
// Copright © 2023 by Thorsten von Eicken
// Based on a driver by Moddable Tech Inc. (had no license, but presumably LGPL)

import Bitmap from "commodetto/Bitmap"
import I2C, { Options as I2COptions } from "embedded:io/i2c"

const SSD1306_SETCONTRAST = 0x81
const SSD1306_DISPLAYALLON_RESUME = 0xa4
const SSD1306_NORMALDISPLAY = 0xa6
const SSD1306_DISPLAYOFF = 0xae
const SSD1306_DISPLAYON = 0xaf
const SSD1306_SETDISPLAYOFFSET = 0xd3
const SSD1306_SETCOMPINS = 0xda
const SSD1306_SETVCOMDETECT = 0xdb
const SSD1306_SETDISPLAYCLOCKDIV = 0xd5
const SSD1306_SETPRECHARGE = 0xd9
const SSD1306_SETMULTIPLEX = 0xa8
const SSD1306_SETSTARTLINE = 0x40
const SSD1306_MEMORYMODE = 0x20
const SSD1306_COLUMNADDR = 0x21
const SSD1306_PAGEADDR = 0x22
const SSD1306_COMSCANDEC = 0xc8
const SSD1306_SEGREMAP = 0xa0
const SSD1306_CHARGEPUMP = 0x8d
const SSD1306_EXTERNALVCC = 0x1
const SSD1306_SWITCHCAPVCC = 0x2
const SSD1306_DEACTIVATE_SCROLL = 0x2e
const vccstate: number = SSD1306_SWITCHCAPVCC

const kBufferSlop = 1

function doCmd(ssd: I2C, cmd: number) {
  ssd.write(new Uint8Array([0, cmd]).buffer)
}

export type Options = Omit<I2COptions, "hz" | "address"> & {
  // we default hz&address
  io: typeof I2C
  hz?: number
  address?: number
  height?: number
  width?: number
}

export default class SSD1306 {
  ssd1306: I2C
  pixel = 0
  out?: Uint8Array
  #height = 64
  #width = 128

  constructor(options: Options) {
    let ssd = (this.ssd1306 = new options.io({
      hz: 1000000,
      address: 0x3c,
      ...options,
    }))

    if (options.height) this.#height = options.height
    if (options.width) this.#width = options.width

    let repeat = 3
    while (repeat--) {
      try {
        doCmd(ssd, SSD1306_DISPLAYOFF) // 0xAE
        break
      } catch (e) {
        trace("SSD1306 init failed:\n" + e + "\n")
        if (!repeat) throw e
      }
    }
    doCmd(ssd, SSD1306_SETDISPLAYCLOCKDIV) // 0xD5
    doCmd(ssd, 0x80) // the suggested ratio 0x80

    doCmd(ssd, SSD1306_SETMULTIPLEX) // 0xA8
    doCmd(ssd, this.#height - 1)

    doCmd(ssd, SSD1306_SETDISPLAYOFFSET) // 0xD3
    doCmd(ssd, 0x0) // no offset
    doCmd(ssd, SSD1306_SETSTARTLINE | 0x0) // line #0
    doCmd(ssd, SSD1306_CHARGEPUMP) // 0x8D
    if (vccstate == SSD1306_EXTERNALVCC) {
      doCmd(ssd, 0x10)
    } else {
      doCmd(ssd, 0x14)
    }
    doCmd(ssd, SSD1306_MEMORYMODE) // 0x20
    doCmd(ssd, 0x00) // 0x0 act like ks0108
    doCmd(ssd, SSD1306_SEGREMAP | 0x1)
    doCmd(ssd, SSD1306_COMSCANDEC)

    if (this.#width == 128 && this.#height == 32) {
      doCmd(ssd, SSD1306_SETCOMPINS) // 0xDA
      doCmd(ssd, 0x02)
      doCmd(ssd, SSD1306_SETCONTRAST) // 0x81
      doCmd(ssd, 0x8f)
    } else if (this.#width == 128 && this.#height == 64) {
      doCmd(ssd, SSD1306_SETCOMPINS) // 0xDA
      doCmd(ssd, 0x12)
      doCmd(ssd, SSD1306_SETCONTRAST) // 0x81
      doCmd(ssd, 0xcf)
    } else {
      throw Error("height/width not supported")
      return
    }

    doCmd(ssd, SSD1306_SETPRECHARGE) // 0xd9
    if (vccstate == SSD1306_EXTERNALVCC) {
      doCmd(ssd, 0x22)
    } else {
      doCmd(ssd, 0xf1)
    }
    doCmd(ssd, SSD1306_SETVCOMDETECT) // 0xDB
    doCmd(ssd, 0x40)
    doCmd(ssd, SSD1306_DISPLAYALLON_RESUME) // 0xA4
    doCmd(ssd, SSD1306_NORMALDISPLAY) // 0xA6

    doCmd(ssd, SSD1306_DEACTIVATE_SCROLL)

    doCmd(ssd, SSD1306_DISPLAYON) //--turn on oled panel
  }
  begin(x: number, y: number, width: number, height: number) {
    if (0 != x || 0 != y || this.#width != width || this.#height != height) {
      trace("partial updates unsupported\n")
      return
    } else {
      let ssd = this.ssd1306

      doCmd(ssd, SSD1306_COLUMNADDR)
      doCmd(ssd, 0) // Column start address (0 = reset)
      doCmd(ssd, this.#width - 1) // Column end address (127 = reset)

      doCmd(ssd, SSD1306_PAGEADDR)
      doCmd(ssd, 0) // Page start address (0 = reset)

      doCmd(ssd, (this.#height >> 3) - 1) // Page end address

      this.pixel = 1
      this.out = new Uint8Array(this.#width + kBufferSlop)
      this.out[kBufferSlop - 1] = 0x40
      this.out.fill(0, kBufferSlop)
    }
  }
  send(data: ArrayBuffer, offset: number, count: number) {
    if (undefined !== offset && offset > 0) {
      data = data.slice(offset)
      if (undefined === count) count = data.byteLength
      else count -= offset
    } else {
      offset = 0
      count = data.byteLength
    }
    let pixels = new Uint8Array(data)

    let ssd = this.ssd1306
    if (count < 0) count = -count
    let pixel = this.pixel

    let out = this.out
    if (!out) throw new Error("begin not called")

    let off = 0
    let width = this.#width
    while (count > 0) {
      let i = width
      while (i--) {
        if (pixels[off + i] & 128) out[kBufferSlop + i] |= pixel
      }
      off += width

      pixel <<= 1
      if (256 === pixel) {
        // flush this set of 8 lines
        ssd.write(out.buffer)
        // start new group of 8 rows
        out.fill(0, kBufferSlop)
        pixel = 1
      }

      count -= width
    }
    this.pixel = pixel
  }
  end() {}
  continue() {}
  pixelsToBytes(count: number) {
    return count
  }
  get pixelFormat() {
    return Bitmap.Gray256
  }
  get width() {
    return this.#width
  }
  get height() {
    return this.#height
  }
  get c_dispatch() {
    return undefined
  }
  adaptInvalid(r: { x: number; y: number; width: number; height: number }) {
    r.x = 0
    r.y = 0
    r.width = this.width
    r.height = this.height
  }
  get async() {
    return false
  }
  get clut() {
    return undefined
  }
}
