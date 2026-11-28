Avia HX711 24-bit ADC for weight scales
=======================================

The HX711 chip is an ADC designed for weight scales or load cells.
It has a 24-bit ADC and a programmable gain amplifier (PGA) with gain up to 128.
The interface requires a clock wire and a data wire: the clock is pulsed 25-27 times and
24 bits of data are read from the data line. A number of extra clock pulses are required and
tell the chip what gain to use.

This driver is very simple and basically provides a single function to read the ADC. This
function is implemented in C using modGPIO in order to perform the read as quickly as possible
and meet the timing requirements of the chip. A read takes a couple of milliseconds.

This driver is intended to conform to ECMA-419. An example is provided in the examples directory.
