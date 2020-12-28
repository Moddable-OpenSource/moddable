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
