import NeoPixel from 'neopixel'

const TIMING_WS2812B = {
  mark: { level0: 1, duration0: 900, level1: 0, duration1: 350 },
  space: { level0: 1, duration0: 350, level1: 0, duration1: 900 },
  reset: { level0: 0, duration0: 100, level1: 0, duration1: 100 }
}
Object.freeze(TIMING_WS2812B)
Object.freeze(TIMING_WS2812B.mark)
Object.freeze(TIMING_WS2812B.space)
Object.freeze(TIMING_WS2812B.reset)

export class NeoMatrix {
  constructor ({ height, width, pin, timing, order, brightness }) {
    this.length = height * width
    this.height = height
    this.width = width
    this.timing = timing || TIMING_WS2812B
    this.neoPixel = new NeoPixel({
      length: this.length,
      pin,
      timing: this.timing,
      order
    })
    this.neoPixel.brightness = brightness || 32
  }
  setPixel (x, y, color) {
    const a = x * this.width
    const b = x & 1 ? this.height - y - 1 : y
    const i = a + b
    this.neoPixel.setPixel(i, color)
  }
  fill (color, index, count) {
    if (index == null) {
      this.neoPixel.fill(color)
    } else if (count == null) {
      this.neoPixel.fill(color, index)
    } else {
      this.neoPixel.fill(color, index, count)
    }
  }
  update () {
    this.neoPixel.update()
  }
  set brightness (b) {
    this.neoPixel.brightness = b
  }
  makeRGB (r, g, b, w) {
    return this.neoPixel.makeRGB(r, g, b, w)
  }
  makeHSB (h, s, b, w) {
    return this.neoPixel.makeHSB(h, s, b, w)
  }
  close () {
    this.neoPixel.close()
  }
}

Object.freeze(NeoMatrix.prototype)
