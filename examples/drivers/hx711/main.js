/*
 * Copyright (c) 2020 Shinya Ishikawa
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

import HX711 from "hx711";
import Timer from "timer";
import { Application, Label } from "piu/MC";
import config from "mc/config";

let hx711 = new HX711({
  dat: config.dat,
  clk: config.clk,
});

let ap = new Application(null, {
  displayListLength: 1024,
  touchCount: 0,
  contents: [
    new Label(null, {
      name: "gram",
      left: 0,
      right: 0,
      top: 0,
      bottom: 0,
      style: new Style({
        font: "OpenSans-Regular-24",
        color: "white",
      }),
      skin: new Skin({
        fill: "black",
      }),
    }),
  ],
});

globalThis.button.b.onChanged = function () {
  if (this.read()) {
    hx711.resetOffset();
  }
};

let v = 0;
Timer.repeat(() => {
  v = hx711.value;
  let gramStr = `${v.toFixed(0)}g`;
  ap.content("gram").string = gramStr;
}, 500);
