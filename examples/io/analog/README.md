# ESP32 Analog Additions
Updated: November 13, 2024

## Constructor

Add `esp32` specific options: `calibrated` and `attenuation`.

```
const analogInput = new device.io.Analog({
    pin: 36,
    esp32: {
        calibrated: true,
        attenuation: 2.5
    }
});
```


### `calibrated`

If the `calibrated` option is included in the analog constructor, the return value of `sample()` will be a calibrated value returned from the ADC driver, with values between 0 and `((1 << analog.resolution) - 1)`. For the ESP32, the device has a resolution of 12 bits, so the range is 0 to 4095.

The [ADC Calibration Driver](https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32/api-reference/peripherals/adc_calibration.html) is used to provide a closer approximation to actual voltage. The voltage range measured can be adjusted with the `attenuation` option.

### `attenuation`

Using the `attenuation` function adjusts the measurement range of the ADC driver.

Vref is a reference voltage used internally by ESP32 ADCs for measuring the input voltage. The ESP32 ADCs can measure analog voltages from 0 V to Vref. Among different chips, the Vref varies, the median is 1.1 V. In order to convert voltages larger than Vref, input voltages can be attenuated before being input to the ADCs.

There are [four attenuation](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32/api-reference/peripherals/adc.html#adc-attenuation) options.

| Attenuation (dB) | Measureable input voltage range |
| :---: | :--- |
| 0 | 100 mV ~ 950 mV
| 2.5 | 100 mV ~ 1250 mV
| 6 | 150 mV ~ 1750 mV
| 12 | 150 mV ~ 2450 mV

With an attenuation of 6 dB, 1750 mV presented at the pin will return 4095.

### Example:

By default, an `analog` object will provide a calibrated output with an attenuation of 12 dB. So, at 2450 mV to the pin, the `sample()` value will be 4095.

If your voltage input range is lower, you can reduce the attenuation. This constructor sets the measurable voltage range from 150 mV to 1750 mV.

```
const analogInput = new device.io.Analog({ pin: 36, attenuation: 6 });
```

Setting the `calibrated` option to false will return the raw value read from the ADC driver.

```
const analogInput = new device.io.Analog({ pin: 36, calibrated: false });
```

