// TI INA233 power monitor usage example
// Copyright © 2023 by Thorsten von Eicken.

import Timer from "timer"
import Time from "time"
import INA233, { ADC, POLARITY } from "embedded:sensor/Power-Management/INA233"

const sensor = new INA233({
  sensor: {
    ...device.I2C.default,
    io: device.io.SMBus,
  },
  onError: err => trace(`INA233: ${err}\n`),

  shuntOhms: 0.003,
  maxCurrent: 30, // max current in amps (default: 10A)
  averaging: 64, // number of samples to average (1,4,16,64,...1024), default:1
  vTime: ADC.TIME_588, // voltage ADC conversion time (ADC.TIME_*), default:1.1ms
  aTime: ADC.TIME_588, // current ADC conversion time (ADC.TIME_*), default:1.1ms
  polarity: POLARITY.BOTH, // polarity (POLARITY.*), default:POLARITY.BOTH
})

trace(`INA233: sampleIval=${sensor.sampleInterval}us maxIval=${sensor.maxInterval(600)}ms\n`)

const ticker = Timer.repeat(() => {
  const sample = sensor.sample()
  trace(`Sample: ${JSON.stringify(sample)}\n`)
}, 1000)

Timer.set(() => {
  Timer.clear(ticker)
  trace("=== The END ===\n")
}, 20000)
