/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

import {Request} from "http"

const APPID = "94de4cda19a2ba07d3fa6450eb80f091";
const zip = "94025";
const country = "us";

let request = new Request({	host: "api.openweathermap.org",
							path: `/data/2.5/weather?zip=${zip},${country}&appid=${APPID}&units=imperial`,
							response: String});

request.callback = function(message, value)
{
	if (Request.responseComplete === message) {
		value = JSON.parse(value, ["main", "name", "temp", "weather"]);
		trace(`The temperature in ${value.name} is ${value.main.temp} F.\n`);
		trace(`The weather condition is ${value.weather[0].main}.\n`);
	}
}
