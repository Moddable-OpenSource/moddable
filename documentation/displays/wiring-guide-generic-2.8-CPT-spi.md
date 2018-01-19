# Wiring Guides for Moddable supported SPI displays

Copyright 2018 Moddable Tech, Inc.  
Revised: January 2, 2018


## BuyDisplay 2.8" CTP display
**Part:** ER-TFTM028-4 LCD w/Capacitive Touch Screen Module  
[Datasheet](http://www.buydisplay.com/download/manual/ER-TFTM028-4_Datasheet.pdf)

**Size:** 2.8", 320 x 240

**Type:** TFT LCD

**Interface:** SPI

**Drivers:** video [ILI9341](../../documentation/drivers/ili9341/ili9341.md), touch FT6206

**Availability:** [2.8" BuyDisplay CTP display] (http://www.buydisplay.com/default/2-8-inch-tft-touch-shield-for-arduino-w-capacitive-touch-screen-module)

**Description:** This BuyDisplay screen is highly configurable and can be ordered with varying touch modules, main power voltage and connection types. This sample is for the 3.3v CTP version.

This display supports many configurations. For this sample Moddable configured the display  IMO settings to support 4 wire SPI. See the ER-TFTM028-4 [datasheet](http://www.buydisplay.com/download/manual/ER-TFTM028-4_Datasheet.pdf). The jumper settings below are open or closed drag solder pads on the PCB.

| IMO Mode | Jumper settings
| --- | --- |
| 4-wire SPI Interface | J2,J3,J4,J5 Short and J1, J6, J7, J8 Open.  R1-R10=0R, R19=0R, R21-R28=0R. R17, R18, R20 not soldered.


![Generic SPI Display](images/spi_serial_2.8_inch_320x240_tft_lcd_display_module_ili9341_arduino_stm32_1.jpg)

**Moddable Sample code:** The Piu example [Drag](../../examples/piu/drag/) is good for testing this display. The build command below includes the -d, debug flag.

```
cd $MODDABLE/examples/piu/drag
mcconfig -d -m -r 0 -f rgb565le -p esp screen=ili9341 touch=ft6206  
```


**ESP8266 Pinout:**

| BuyDisplay CTP Display | ESP8266 | ESP8266 Devboard label
| --- | --- | --- |
| 1 - VSS | GND | 
| 2 - VDD | 3.3V | 
| 21 - Reset | 3.3v  |  
| 23 - CS | GPIO 15 | (D8) 
| 24 - SCK | GPIO 14 | (D5)
| 25 - DC | GPIO 2 | (D4)  
| 27 - SDI | GPIO 13 | (D7)
| 28 - CPT SDL | GPIO 4 | (D2)
| 31  -CPT SDA | GPIO 5 | (D1) 

![Generic 2.4"-2.8" wiring illustration](images/buydisplay+esp-wiring.png)

