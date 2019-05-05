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

import Timer from "timer";
import config from "mc/config";
import OneWire from "onewire";
import DS18X20 from "DS18X20";

// ESP8266 NodemCU D6 = GPIO12

/*
import Digital from "pins/digital";
let count = 0;
Timer.repeat(() => {
	trace(`repeat ${++count} \n`);
	Digital.write(config.onewire.pin, ~count & 1);
}, 500);
*/

const ow = new OneWire({
  pin: config.onewire.pin
});

let devices = ow.search(); // search returns an array of device IDs

trace('found ',devices.length, '\n' );
trace( devices.map( x => OneWire.hex(x) + '\n' ) );

if ( devices.length ) {

let sensor = new DS18X20({bus:ow});

// echo ROM code
trace( 'Found first:',sensor.toString(),'\n');

sensor = new DS18X20({bus:ow, index: 1});

// echo ROM code
trace( 'Found 2nd:',sensor.toString(),'\n');

// test callback on temperature conversion
function gotTemp( t ) {
  trace('Got temp!',t,'\n');
}
sensor.getTemp(gotTemp);

let res=sensor.resolution;
trace( "res Before:",res,'\n');

sensor.resolution=9;
res=sensor.resolution;
trace( "res - should be 9 :",res,'\n');

sensor.resolution=12;
res=sensor.resolution;
trace( "res - should be 12 :",res,'\n');

var sensors = ow.search().map(function (device) {
  return new DS18X20({bus:ow, id:device, digits:1});
});

function readTemps() {

  sensors.forEach(function (sensor, index) {
    let t = sensor.temp;
    trace(index, ' ', sensors[index].toString(), ': ', t, '\n');
  });
}

readTemps();

Timer.repeat(readTemps, 5000);

}

