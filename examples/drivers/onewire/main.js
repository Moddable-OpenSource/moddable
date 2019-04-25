/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

import OneWire from "onewire";
import Timer from "timer";

const ow = new OneWire({
  pin: 23
});

let devices = ow.search(); // search returns an array of device IDs

trace('found ',devices.length, '\n' );
trace( devices.map( x => ow.rom(x) + '\n' ) );

import DS18X20 from "DS18X20";

//let sensor = new DS18X20(ow, devices[0]);
let sensor = new DS18X20(ow);

// echo ROM code
trace( sensor.toString(),'\n');

// test callback on temperature conversion
function gotTemp( t ) {
  trace('Got temp!',t,'\n');
}
sensor.getTemp(gotTemp);

let res=sensor.resolution;
trace( "res Before:",res,'\n');

sensor.resolution=7;
res=sensor.resolution;
trace( "res - should be 9 :",res,'\n');

sensor.resolution=12;
res=sensor.resolution;
trace( "res - should be 12 :",res,'\n');

let alarms=sensor.searchAlarm();
trace( 'Alarms:',alarms.length,'\n')

sensor.setAlarm(10,21);

alarms=sensor.searchAlarm();
trace( 'Alarms > 21:',alarms.length,'\n');

var sensors = ow.search().map(function (device) {
  return new DS18X20(ow, device);
});

function readTemps() {

  sensors.forEach(function (sensor, index) {
    let t = sensor.temp;
    trace(index, ' ', sensors[index].toString(), ':', t, '\n');
    //t.toFixed(2);
  });
}

readTemps();

Timer.repeat(readTemps, 5000);