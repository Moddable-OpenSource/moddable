This provider.js for esp32c3 assumes:

A switch is connected to GPIO-0
An LED is connected from GPIO-10 to ground (through a resistor).

I2C is set up as DATA: GPIO-2 and CLK: GPIO-3

Serial is set up over UART0 with pins TX: GPIO-4, RX: GPIO-5

Analog is connected to GPIO-1 (ADC1_0)
