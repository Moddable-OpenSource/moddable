TI INA233 Power Monitor
=======================

The INA233 is a power monitor chip that measures voltage and current and calculates power.
A special feature of this chip is to accumulate power internally allowing accurate
energy consumption measurements with relatively infrequent polling.

TI product information: https://www.ti.com/product/INA233

This driver is intended to conform with ECMA419.

The samples returned contain voltage, current, power, and energy measurements.
The energy is accumulated in the chip and must be cleared periodically (or can
be cleared automatically at every reading).

Alerts/interrupts are not supported.

### Power accumulator roll-over

The INA233 has a 24-bit accumulator for power measurements and a 24-bit sample count register.
The `sample()` method must be called frequently enough to prevent the accumulator from
overflowing.
This interval can be calculated as follows:
- power = voltage * current (expected max value)
- current_LSB = current / 32768 (current resolution)
- adc_interval = voltage_ADC_conversion_time + current_ADC_conversion_time
- power_interval = adc_interval * number_of_averages
- power_increment = power / (25 * current_LSB)
- samples_to_overflow = 2^24 / power_increment
- time_to_overflow = samples_to_overflow * power_interval

The `maxInterval()` method returns this value given the expected power consumption.
