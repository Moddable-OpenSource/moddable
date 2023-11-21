/*
 * Copyright (c) 2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Poco from "commodetto/Poco";
import parseBMF from "commodetto/parseBMF";
import Resource from "Resource";

let render = new Poco(screen, { displayListLength: 2048 });
let black = render.makeColor(0, 0, 0);
let white = render.makeColor(255, 255, 255);
let font = parseBMF(new Resource("OpenSans-Regular-72.bf4"));

function drawCount(count) {
  render.begin();
    render.fillRectangle(black, 0, 0, render.width, render.height);
    render.drawText(
      count,
      font,
      white,
      (render.width - render.getTextWidth(count, font)) >> 1,
      (render.height - font.height) >> 1
    );
  render.end();
}

let current = 0;
drawCount(current);

const buzzer = new device.peripheral.tone.Default();
const button = new device.peripheral.button.A({
  onPush() {
    const pressed = this.pressed;
    trace(`pressed: ${pressed}\n`);
    if (pressed) {
      rotaryEncoder.write(0);
      current = 0;
      buzzer.tone(2000, 20);
      drawCount(current.toString());
    }
  },
});

const rotaryEncoder = new device.peripheral.RotaryEncoder({
  onReadable() {
    const count = this.read();
    trace(`count: ${count}\n`);
    drawCount(count.toString());
    if (count > current) {
      buzzer.tone(7000, 20);
    } else {
      buzzer.tone(6000, 20);
    }
    current = count;
  },
});
