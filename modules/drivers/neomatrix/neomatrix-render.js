/* eslint-disable camelcase */

import Bitmap from 'commodetto/Bitmap'
import { NeoMatrix } from 'neomatrix'
import CONFIG from "mc/config";

export default class NeoMatrixRender {
  #matrix
  #x
  #y
  #w
  #h
  #idx
  #pin
  #width
  #height
  #pixelFormat
  constructor (dictionary = {}) {
    this.#width = dictionary.width ? dictionary.width : this.#getConfigureWidth()
    this.#height = dictionary.height ? dictionary.height : this.#getConfigureHeight()
    this.#pixelFormat = dictionary.pixelFormat
      ? dictionary.pixelFormat
      : Bitmap.RGB565LE
    this.#pin = dictionary.pin ? dictionary.pin : this.#getConfigurePin()
    this.#matrix = new NeoMatrix({
      width: this.#width,
      height: this.#height,
      pin: this.#pin,
      order: 'GRB',
      brightness: this.#getConfigureBrightness(),
      rotation: CONFIG.rotation
    })
    this.#idx = 0
  }

  begin (x, y, width, height) {
    this.#x = x
    this.#y = y
    this.#w = width
    this.#h = height
    this.#idx = 0
  }

  send (pixels, offset, count) {
    if (this.#x == null) {
      throw new Error('begin should be called first')
    }
    const m = this.#matrix
    const u16a = new Uint16Array(pixels)
    const len = u16a.length
    for (let i = this.#idx; i < this.#idx + len; i++) {
      const pixel = u16a[i]
      const r = (pixel & 0b1111100000000000) >> 8
      const g = (pixel & 0b0000011111100000) >> 3
      const b = (pixel & 0b0000000000011111) << 3
      const x = (i % this.#w) + this.#x
      const y = Math.floor(i / this.#w) + this.#y
      m.setPixel(x, y, m.makeRGB(r, g, b))
    }
    this.#idx += len
  }

  end () {
    this.#matrix.update()
    this.#x = this.#y = this.#w = this.#h = this.#idx = null
  }

  adaptInvalid (r) {
    r.x = 0
    r.y = 0
    r.w = this.#width
    r.h = this.#height
  }

  continue (x, y, width, height) {
    this.#x = x
    this.#y = y
    this.#w = width
    this.#h = height
    this.#idx = 0
  }

  pixelsToBytes (count) {
    const bytes = (count * Bitmap.depth(this.#pixelFormat)) >> 3
    return bytes
  }

  get async () {
    return false
  }

  get clut () {}

  set clut (clut) {}

  get c_dispatch () {}

  get width () {
    return this.#width
  }

  get height () {
    return this.#height
  }

  get pixelFormat () {
    return this.#pixelFormat
  }

  set brightness (b) {
    this.#matrix.brightness = b
  }

  #getConfigurePin() @ "xs_NeoMatrix_get_configure_pin";

  #getConfigureWidth() @ "xs_NeoMatrix_get_configure_width";

  #getConfigureHeight() @ "xs_NeoMatrix_get_configure_height";

  #getConfigureBrightness() @"xs_NeoMatrix_get_configure_brightness";
}

Object.freeze(NeoMatrixRender.prototype)
