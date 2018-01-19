# Wiring Guides for Moddable supported SPI displays

Copyright 2018 Moddable Tech, Inc.  
Revised: January 2, 2018


## Adafruit 1.8" ST7735
**Part:** Adafruit Product ID: 2088 

**Size:** 1.8", 128 × 160

**Type:** TFT LCD

**Interface:** SPI

**Drivers:** video [ST7735](../../documentation/drivers/st7735/st7735.md), No touch

**Availability:**  
[Adafruit 1.8" Color TFT LCD Display] (https://www.adafruit.com/product/358)  
[HiLetgo 1.8" ST7735R](https://www.amazon.com/gp/product/B00LSG51MM/)


**Description:** Colorful, bright display wtih 4-wire SPI.

![Generic SPI Display](images/adafruit-st7735-1.8.jpg)

**Moddable Sample code:** The Piu example [balls](../../examples/piu/balls/) is good for testing this display. The build command below includes the -d, debug flag.

```
cd $MODDABLE/examples/piu/balls
mcconfig -d -m -r 0 -f rgb332 -p esp screen=st7735 touch="" 
```


**ESP8266 Pinout:**

| Adafruit 1.8" TFT | ESP8266 | ESP8266 Devboard label
| --- | --- | --- |
| LITE | 3.3v | 
| MISO | GPIO 12 | (D6)
| SCK | GPIO 14 | (D5)
| MOSI | GPIO 13 | (D7)
| TFT_CS | GPIO 15| (D8)
| DC | GPIO 2 | (D4)
| RESET | 3.3v  | 
| VCC| 3.3v | 
| GND | GND | 

![Generic 2.4"-2.8" wiring illustration](images/adafruit-st7735-1.8-wiring.png)

