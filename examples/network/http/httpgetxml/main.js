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
import {XML} from "xml"

let APPID = "94de4cda19a2ba07d3fa6450eb80f091";
let location = "Cupertino";		//@@ encodeURIComponent

let request = new Request({	host: "api.openweathermap.org",
							path: `/data/2.5/weather?q=${location}&APPID=${APPID}&units=imperial&mode=xml`,
							response: String});

request.callback = function(message, value)
{
	if (5 == message) {
		let root = XML.parse(value);

		let cityElement = root.elements.find(element => "city" == element.name);
		let valueAttribute = cityElement.attributes.find(attribute => "name" == attribute.name);
		let city = valueAttribute.value;

		let temperatureElement = root.elements.find(element => "temperature" == element.name);
		valueAttribute = temperatureElement.attributes.find(attribute => "value" == attribute.name);
		let temperature = Number.parseFloat(valueAttribute.value);

		let weatherElement = root.elements.find(element => "weather" == element.name);
		valueAttribute = weatherElement.attributes.find(attribute => "value" == attribute.name);
		let weather = valueAttribute.value;

		trace(`The temperature in ${city} is ${temperature} F.\n`);
		trace(`The weather condition is ${weather}.\n`);
	}
}
