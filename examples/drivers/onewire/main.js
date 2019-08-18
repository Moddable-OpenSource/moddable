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

const bus = new OneWire({
  pin: config.onewire.pin
});

let devices = bus.search(); // search returns an array of device IDs

trace('found ', devices.length, '\n');
trace(devices.map(x => OneWire.hex(x) + '\n'));

if (devices.length) {

  let sensor = new DS18X20({
    bus
  });

  // echo ROM code
  trace('Found first:', sensor.toString(), '\n');

  try {
    sensor = new DS18X20({
      bus,
      index: 1
    });
    trace('Found 2nd:', sensor.toString(), '\n');
  } catch {
    trace('No 2nd!\n');
  }

  trace(`Sensor present: ${sensor.present}\n`);
  trace(`NULL Sensor present: ${bus.isPresent(new ArrayBuffer(8))}\n`);

  // echo ROM code

  // test callback on temperature conversion
  function gotTemp(t) {
    trace('Got temp!', t, '\n');
  }
  sensor.getTemperature(gotTemp);

  let res = sensor.resolution;
  trace("res Before:", res, '\n');

  sensor.resolution = 9;
  res = sensor.resolution;
  trace("res - should be 9 :", res, '\n');

  sensor.resolution = 12;
  res = sensor.resolution;
  trace("res - should be 12 :", res, '\n');

  var sensors = bus.search().map(function (id) {
    return new DS18X20({
      bus,
      id
    });
  });

  function readTemps() {

    sensors.forEach(function (sensor, index) {
      let t = sensor.temperature;
      trace(index, ' ');
      trace(sensors[index]);
      //     trace( ': ', t.toFixed(2), '\n');
      trace( ': ', t, '\n');
    });
  }

  readTemps();

  Timer.repeat(readTemps, 1000);

} 