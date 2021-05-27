/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 * Copyright (c) 2019  Wilberforce
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Timer from "timer";
import HD44780 from "HD44780";

let lcd = new HD44780({
  sda: 12,
  scl: 13,
  address: 0x27,
});

// Degree symbol
lcd.createChar( 2,[ 0x0C,0x12,0x12,0x0C,0x00,0x00,0x00,0x00] );

lcd.setCursor(0, 0);
lcd.cursor(true);

// Moddable logo Left and Right parts, 6x8 matrix
lcd.createChar( 0, 
[ 
  0b00001110,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00001110,
] );
lcd.createChar( 1, 
  [ 
    0b00000000,
    0b00111100,
    0b00000010,
    0b00000010,
    0b00000010,
    0b00000010,
    0b00111100,
    0b00000000,
  ] );

lcd.blink(true);
lcd.blink(false);
lcd.blink(true);
lcd.cursor(false);
lcd.cursor(true);
lcd.setCursor(0, 0);
lcd.setCursor(5, 1);
lcd.setCursor(0, 0);

lcd.print('Moddable '+String.fromCharCode(0)+String.fromCharCode(1));

Timer.repeat(() => {
  let date = new Date();
  let hours = String(date.getHours());
  let minutes = String(date.getMinutes());
  let seconds = String(date.getSeconds());
  let ampm = (hours > 11 ? ' pm' : ' am');
  if (hours > 12)
    hours -= 12;
  if (1 == minutes.length)
    minutes = '0' + minutes;
  if (1 == seconds.length)
    seconds = '0' + seconds;
  let time = hours + ':' + minutes + ':' + seconds + ampm;
  lcd.setCursor(0,1).print(time);
  lcd.print( ' 20'+String.fromCharCode(2)+'c');
}, 1000);