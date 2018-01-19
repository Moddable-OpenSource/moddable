# Wiring Guides for Moddable supported SPI displays

Copyright 2018 Moddable Tech, Inc.  
Revised: January 2, 2018


## Sharp Memory Screen 
**Part:** LS027B7DH01A

**Size:** 2.7", 240 x 400

**Type:** Sharp Memory TFT-LCD, Monochrome

**Interface:** SPI

**Drivers:** video [ls013b4dn04](../../documentation/drivers/ls013b4dn04/ls013b4dn04.md), no touch

**Availability:** [Sharp Memory 2,7" LCD on Digi-Key] (https://www.digikey.com/product-detail/en/sharp-microelectronics/LS027B7DH01A/425-2908-ND/5054067?utm_adgroup=Optoelectronics&gclid=Cj0KCQiAvrfSBRC2ARIsAFumcm-L2iz88RlcYf9Z1MU0J1ZW97VgAa0oPoDBgqYSIIRUyZnhGNURyY4aAjIgEALw_wcB)

**Description:** The Sharp Memory display is a blend of an eInk (e-paper) display and an LCD. It has the very-low power draw of eInk and fast-refresh rates of an LCD.

Moddable used the [Kuzyatech Sharp Memory display breakout board] (https://www.tindie.com/products/kuzyatech/sharp-memory-lcd-breakout-a2/) to interface with the display FFC and boost the ESP8266 power to the 5v needed to drive the display.


![Generic SPI Display](images/Sharp_Memory_LCD_LS027B7DH01.jpg)

**Moddable Sample code:** The Piu example [Balls](../../examples/piu/balls/) is good for testing this display. The build command below includes the -d, debug flag.

```
cd $MODDABLE/examples/piu/balls
mcconfig -d -m -r 0 -f rgb565le -p esp screen=ili9341 touch=xpt2046  
```

**ESP8266 Pinout:**

| 2.7" Memory Display | ESP8266 | ESP8266 Devboard label
| --- | --- | --- |
| VIN | 3.3V |  
| GND | GND | 
| EXTMODE |  |  
| DISP | 3.3V | 
| EXTCOMM |  | 
| SCS | GPIO 4 | (D2) 
| SI | GPIO 13 | (D7) 
| SCK | GPIO 14 | (D5) 

![Generic SPI Display](images/wiring-Kuzyatech-sharp-2.7.jpg)


