/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import {} from "piu/MC";

const whiteSkin = new Skin({ fill:"white" });

const DOWN = 0;
const RIGHT = 1;
const UP = 2;
const LEFT = 3;
const PAUSE = 4;

const LoadingIcon = Port.template($ => ({
  height: $.squareSize*5, width: $.squareSize*5,
  Behavior: class extends Behavior {
    onCreate(port, data) {
      this.data = data;
      data.color1 = hsl(data.color[0], data.color[1], data.color[2]);
      data.color2 = hsl(data.color[0], data.color[1], data.color[2]+0.17);
      data.color3 = hsl(data.color[0], data.color[1], data.color[2]+0.34);
      data.color4 = hsl(data.color[0], data.color[1], data.color[2]+0.51);
      this.direction = DOWN;
      this.index = 0;
    }
    onDisplaying(port) {
      this.dxy = Math.round(port.height / 10);
      this.x = 0;
      this.y = 0;
      this.last6coords = new Int16Array(12);
      port.interval = this.data.frequency / 30;
      port.time = 0;
      port.start();
    }
    onDraw(port) {
      let dxy = this.dxy
      let last6coords = this.last6coords;
      port.fillColor(this.data.color4, last6coords[10], last6coords[11], this.data.squareSize, this.data.squareSize);
      port.fillColor(this.data.color3, last6coords[6], last6coords[7], this.data.squareSize, this.data.squareSize);
      port.fillColor(this.data.color2, last6coords[2], last6coords[3], this.data.squareSize, this.data.squareSize);
      switch (this.direction) {
        case DOWN:
          this.y += this.dxy;
          break;
        case RIGHT:
          this.x += this.dxy;
          break;
        case UP:
          this.y -= this.dxy;
          break;
        case LEFT:
          this.x -= this.dxy;
          break;
        case PAUSE:
          break;
      }
      port.fillColor(this.data.color1, this.x, this.y, this.data.squareSize, this.data.squareSize);

      last6coords.copyWithin(2, 0);
      last6coords[0] = this.x;
      last6coords[1] = this.y;
    }
    onTimeChanged(port) {
      this.index++;
      if (this.index == 8) {
        this.index = 0;
        this.direction++;
        if (this.direction > PAUSE) {
          this.x = 0;
          this.direction = DOWN;
        }
      }
      port.invalidate();
    }
  }
}))

export default new Application({}, {
  skin: whiteSkin, 
  contents: [
    new LoadingIcon({ squareSize: 20, frequency: 1000, color: [ 230, 0.74, 0.38 ] }) // ~#192eab
  ],
});


