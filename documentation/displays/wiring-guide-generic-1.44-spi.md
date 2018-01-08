# Wiring Guides for Moddable supported SPI displays

Copyright 2018 Moddable Tech, Inc.  
Revised: January 2, 2018


## Generic 1.44" displays (no touch)
**Size:** 1.44", 320 x 240

**Type:** TFT LCD

**Drivers:** video [ST7735](../../documentation/drivers/st7735/st7735.md), no touch

**Availability:** [Generic 1.44" SPI Displays on eBay] (https://www.ebay.com/sch/i.html?_odkw=spi+display&_osacat=0&_from=R40&_trksid=p2045573.m570.l1313.TR0.TRC0.H0.Xspi+display+1.44%22.TRS1&_nkw=spi+display+1.44%22&_sacat=0)

**Description:** These inexpensive displays are available on eBay and other resources. 

![Generic SPI Display](images/generic-1.44-display.jpg)

**Moddable Sample code:** The Piu example [Transitions](../../examples/piu/transitions/) is good for testing this display. The build command below includes the -d, debug flag so requires XSBug to be launched to run.

```
cd $MODDABLE/examples/piu/transitions
mcconfig -d -m -r 0 -f rgb565le -p esp screen=ST7735 touch=""  
```
See [ST7735](../../documentation/drivers/st7735/st7735.md) documentation on adding ST7735 driver to your app.

**ESP8266 Pinout:**

| 1.44" Display | ESP8266 | ESP8266 Devboard label
| --- | --- | --- | 
| VCC | 3.3V |
| GND | GND | 
| CS | GPIO 15 | (D8)
| RESET | 3.3V | 
| AO | GPIO 2 | (D4)
| SDA | GPIO 13 | (D7) 
| SCK | GPIO 14 | (D5) 
| LED | 3.3V | 


![Generic 2.4"-2.8" wiring illustration](images/esp-generic-1.44-display.jpg)

