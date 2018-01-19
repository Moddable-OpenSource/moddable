# Wiring Guides for Moddable supported SPI displays

Copyright 2018 Moddable Tech, Inc.  
Revised: January 2, 2018


## Sparkfun TeensyView
**Part:** Sparkfun - LCD-14048 

**Size:** 128 x 32

**Type:** OLED, Monochrome

**Interface:** SPI

**Drivers:** video [SSD1306](../../documentation/drivers/ssd1306/ssd1306.md), no touch

**Availability:** [TeensyView on Sparkfun] (https://www.sparkfun.com/products/14048)

**Description:** Very small monochrome, OLED display. Moddable used the TeensyView configured in [standard] (https://learn.sparkfun.com/tutorials/teensyview-hookup-guide) mode.


![Generic SPI Display](images/teensyview.jpg)

**Moddable Sample code:** The Piu example [Balls](../../examples/piu/balls/) is good for testing this display. The build command below includes the -d, debug flag.

```
cd $MODDABLE/examples/piu/balls
mcconfig -d -m -r 0 -f rgb565le -p esp screen=ili9341 touch=xpt2046  
```

**ESP8266 Pinout:**

| TeensyView Display | ESP8266 | ESP8266 Devboard label
| --- | --- | --- |
| GND | GND | 
| 5 | GPIO 2 | (D4)
| 10 | GPIO 4 | (D2)
| 11 | GPIO 13 | (D7) 
| 13 | GPIO 14 | (D5) 
| 15 | GPIO 0 | (D3) 
| 3v | 3.3V |

![Generic SPI Display](images/teensyview-wiring.jpg)


