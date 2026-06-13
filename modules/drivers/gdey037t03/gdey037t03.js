/*
 * Copyright (c) 2026 Joshua Jun
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
	Datasheet: https://v4.cecdn.yun300.cn/100001_1909185148/GDEY037T03-new.pdf

	The panel's pixel array is 240 x 416 with a reversed gate scan. This driver
	presents the GDEY037T03's advertised orientation (416 x 240 landscape, upright)
	so software uses it at rotation 0 with no flip: the framebuffer is transposed
	into the native array on output, which also absorbs the gate-scan reversal.
*/

import Timer from "timer";
import Bitmap from "commodetto/Bitmap";
import Dither from "commodetto/Dither";

const WIDTH = 416;
const HEIGHT = 240;
const STRIDE = WIDTH >> 3;
const NATIVE_W = HEIGHT;
const NATIVE_H = WIDTH;
const NATIVE_STRIDE = NATIVE_W >> 3;
const FRAME_BYTES = STRIDE * HEIGHT;

// Native transpose (gdey037t03.c): native(nx, ny) = logical(ny, nx). Done in C
// because the per-pixel bit loop over the whole frame is far too slow in script.
function transpose(logical, out) @ "xs_gdey037t03_transpose";

class EPD {
  constructor() {
    const Digital = device.io.Digital;
    const SPI = device.io.SPI;

    this.select = new Digital({
      pin: device.pin.epdSelect,
      mode: Digital.Output,
      initialValue: 1,
    });
    this.dc = new Digital({
      pin: device.pin.epdDC,
      mode: Digital.Output,
      initialValue: 1,
    });
    this.reset = new Digital({
      pin: device.pin.epdReset,
      mode: Digital.Output,
      initialValue: 1,
    });
    this.busy = new Digital({ pin: device.pin.epdBusy, mode: Digital.Input });

    this.spi = new SPI({ ...device.SPI.default, hz: 10_000_000, mode: 0 });
    this.spi.byte = new Uint8Array(1);
  }
  close() {
    this.select?.close();
    this.dc?.close();
    this.reset?.close();
    this.busy?.close();
    this.spi?.close();
    this.select = this.dc = this.reset = this.busy = this.spi = undefined;
  }

  writeCMD(cmd) {
    this.select.write(0);
    this.dc.write(0);
    this.spi.byte[0] = cmd;
    this.spi.write(this.spi.byte);
    this.select.write(1);
  }
  writeData(value) {
    this.select.write(0);
    this.dc.write(1);
    this.spi.byte[0] = value;
    this.spi.write(this.spi.byte);
    this.select.write(1);
  }
  writeBuffer(buffer) {
    this.select.write(0);
    this.dc.write(1);
    this.spi.write(buffer);
    this.select.write(1);
  }

  isBusy() {
    return !this.busy.read(); // LOW (0) = busy
  }
  waitBusy(ms = 5000) {
    const deadline = Date.now() + ms;
    do {
      if (!this.isBusy()) return;
      Timer.delay(1);
    } while (Date.now() < deadline);
    throw new Error("UC8253 BUSY timeout");
  }

  reset_() {
    this.reset.write(0);
    Timer.delay(10);
    this.reset.write(1);
    Timer.delay(10);
  }

  initFull() {
    // EPD_Init — cleanest, flashes
    this.reset_();
    this.writeCMD(0x04);
    this.waitBusy();
    this.writeCMD(0x50);
    this.writeData(0x97);
  }
  initFast() {
    // EPD_Init_Fast — ~1.5 s
    this.reset_();
    this.writeCMD(0x04);
    this.waitBusy();
    this.writeCMD(0xe0);
    this.writeData(0x02);
    this.writeCMD(0xe5);
    this.writeData(0x5f);
  }
  initPart() {
    // EPD_Init_Part — no flash
    this.reset_();
    this.writeCMD(0x04);
    this.waitBusy();
    this.writeCMD(0xe0);
    this.writeData(0x02);
    this.writeCMD(0xe5);
    this.writeData(0x6e);
    this.writeCMD(0x50);
    this.writeData(0xd7);
  }

  #refresh() {
    this.writeCMD(0x12);
    Timer.delay(1); // >= 200us before polling BUSY
    this.waitBusy();
  }
  #powerOff() {
    this.writeCMD(0x02);
    this.waitBusy();
  }

  display(oldFrame, newFrame) {
    this.writeCMD(0x10);
    this.writeBuffer(oldFrame);
    this.writeCMD(0x13);
    this.writeBuffer(newFrame);
    this.#refresh();
    this.#powerOff();
  }

  displayPartial(x, y, w, h, oldWin, newWin) {
    const xEnd = x + w - 1,
      yEnd = y + h - 1;
    this.writeCMD(0x91); // partial in
    this.writeCMD(0x90); // window setting
    this.writeData(x);
    this.writeData(xEnd);
    this.writeData(y >> 8);
    this.writeData(y & 0xff);
    this.writeData(yEnd >> 8);
    this.writeData(yEnd & 0xff);
    this.writeData(0x01);
    this.writeCMD(0x10);
    this.writeBuffer(oldWin);
    this.writeCMD(0x13);
    this.writeBuffer(newWin);
    this.#refresh();
    this.writeCMD(0x92); // partial out
    this.#powerOff();
  }

  deepSleep() {
    this.writeCMD(0x02);
    this.waitBusy();
    this.writeCMD(0x07);
    this.writeData(0xa5);
  }
}

class Display {
  // Commodetto PixelsOut
  #epd;
  #current = new Uint8Array(FRAME_BYTES).fill(0xff); // logical frame being drawn (0xFF = white)
  #previous = new Uint8Array(FRAME_BYTES).fill(0xff); // last displayed native frame (0x10 "old data")
  #dither = new Dither({ width: WIDTH });
  #mode = "full"; // full-refresh waveform: "full" (clean) or "fast"
  #wantFull = true; // next update is a full (de-ghosting) refresh

  constructor(options = {}) {
    this.#epd = new EPD();
    if (options.mode) this.#mode = options.mode;
  }
  close() {
    if (this.#epd) {
      this.#epd.deepSleep();
      this.#epd.close();
      this.#epd = undefined;
    }
    this.#dither?.close?.();
    this.#dither = this.#current = this.#previous = undefined;
  }
  configure(options) {
    if (undefined !== options?.refresh) this.#wantFull = options.refresh;
    if (undefined !== options?.mode) this.#mode = options.mode;
    if (undefined !== options?.dither) {
      this.#dither.close();
      let algorithm = options.dither;
      if (false === algorithm) algorithm = "none";
      else if (true === algorithm) algorithm = undefined;
      this.#dither = new Dither({ width: WIDTH, algorithm });
    }
  }

  begin(x, y, width, height) {
    this.#current.position = 0;
    this.#dither.reset();
  }
  send(pixels, offset = 0, byteLength = pixels.byteLength) {
    const buffer = this.#current;
    this.#dither.send(
      Math.idiv(byteLength, WIDTH),
      pixels,
      offset,
      buffer,
      buffer.position,
    );
    buffer.position += byteLength >> 3;
  }
  end() {
    const epd = this.#epd;
    const next = this.#toNative(this.#current);
    if (this.#wantFull) {
      // full refresh: redraws every pixel, clears ghosting, flashes
      "fast" === this.#mode ? epd.initFast() : epd.initFull();
      epd.display(this.#previous, next);
      this.#wantFull = false;
    } else {
      // partial waveform over the whole screen: no flash
      epd.initPart();
      epd.displayPartial(0, 0, NATIVE_W, NATIVE_H, this.#previous, next);
    }
    this.#previous = next;
  }
  continue() {
    return this.end();
  }
  adaptInvalid(area) {
    area.x = 0;
    area.y = 0;
    area.w = WIDTH;
    area.h = HEIGHT;
  }

  // Force a full (de-ghosting) refresh of the current image right now.
  refresh() {
    const epd = this.#epd;
    "fast" === this.#mode ? epd.initFast() : epd.initFull();
    epd.display(this.#previous, this.#previous);
    this.#wantFull = false;
  }

  get width() {
    return WIDTH;
  }
  get height() {
    return HEIGHT;
  }
  get pixelFormat() {
    return Bitmap.Gray256;
  }
  get async() {
    return false;
  }
  pixelsToBytes(count) {
    return count;
  }

  // Transpose a full logical (W x H) frame into the native (H x W) panel array,
  // absorbing the panel's reversed gate scan so the advertised landscape
  // orientation comes out upright. The work is done natively (see gdey037t03.c).
  #toNative(logical) {
    const out = new Uint8Array(NATIVE_STRIDE * NATIVE_H);
    transpose(logical.buffer, out.buffer);
    return out;
  }
}

export default Display;
