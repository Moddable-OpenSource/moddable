// ssd1306-io exammple
// Copright © 2023 by Thorsten von Eicken
//
// run using a commandline like mcconfig -d -m -p esp32/nodemcu -f gray256

import Poco from "commodetto/Poco"
import SSD1306 from "embedded:display/ssd1306"
import parseBMF from "commodetto/parseBMF"
import Resource from "Resource"

//const font = parseBMF(new Resource("OpenSans-Regular-24.bf4"))
const font = parseBMF(new Resource("OpenSans-Semibold-28.bf4"))

const screen = new SSD1306({
  io: device.io.I2C,
  clock: 10,
  data: 8,
  height: 32,
})

let poco = new Poco(screen)
const white = poco.makeColor(255, 255, 255)
const black = poco.makeColor(0, 0, 0)

poco.begin()
poco.fillRectangle(white, 0, 0, poco.width, poco.height)
poco.fillRectangle(black, 1, 1, poco.width - 2, poco.height - 2)
poco.fillRectangle(white, poco.height / 2, poco.height / 2, poco.height / 2, poco.height / 2)
//poco.fillRectangle(poco.makeColor(gray, gray, gray), 4, 4, poco.width - 8, poco.height - 8)
poco.drawText("220W 35%", font, white, 0, -7)
poco.end()
