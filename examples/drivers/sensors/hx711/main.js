// Copyright © 2023 by Thorsten von Eicken.
import HX711 from "embedded:sensor/ADC/HX711"
import Timer from "timer"
import Time from "time"

trace("===== HX711 TEST =====\n")

let hx711 = new HX711({
  sensor: {
    io: device.io,
    din: 7,
    clk: 6,
  },
  gain: 1, // 128x
})

let v = 0
Timer.repeat(() => {
  const t0 = Time.ticks
  const raw = hx711.read()
  const dt = Time.ticks - t0
  trace(`Raw: ${raw} in ${dt}ms\n`)
}, 1000)
