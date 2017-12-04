# Connecting a SPI display to ESP8266

Copyright 2017 Moddable Tech, Inc.

Revised: October 12, 2017



## Overview

Many Moddable examples include screen and touch UI. These demos have been developed with inexpensive resistive SPI touch displays readly available on eBay and other sources. The displays are 320 x 240 resoution and come in 2.4" and 2.8" sizes. 

Any SPI based screen should work. We have brought up samples and products on many screens including LCD, OLED, eInk. There are a number of display and touch drivers in the open source repository. See: modules/drivers/

These inexpensive red resistive displays use the ili9341 display IC/driver and the xpt2046 IC/driver.

<img src="assets/spi-touch-display.jpg" height="311"/> 


## Wiring

When wired to an ESP8266 development board as shown below any of the Moddable examples with screen UI will utilize the screen. 

<img src="assets/esp-display-wiring.png" height="720"/> 