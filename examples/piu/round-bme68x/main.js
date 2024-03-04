/*
 * Copyright (c) 2023-2024 Moddable Tech, Inc.
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

import * as SensorNamespace from "embedded:sensor/Barometer-Humidity-Temperature/BME68x";
import * as MCNamespace from "piu/MC";
import * as TimelineNamespace from "piu/Timeline";

import {} from "piu/shape";
import {Outline} from "commodetto/outline";

const globals = {
	device,
	screen,
	Date, Application, Behavior, CLUT, Column, Container, Content, DeferLink, Die, Label, Layout, Link,
	Locals, Port, Row, Scroller, Skin, Style, template, Template, Text, Texture, TouchLink,
	Transition, blendColors, hsl, hsla, rgb, rgba,
	Shape, Outline,
};
const modules = {
	"embedded:sensor/Barometer-Humidity-Temperature/BME68x": { namespace:SensorNamespace },
	"piu/MC": { namespace:MCNamespace },
	"piu/Timeline": { namespace:TimelineNamespace },
};

let ErrorApplication = Application.template($ => ({
	displayListLength:8192,
	skin:{ fill:"#192eab" },
	contents: [
		Column($, { 
			left:0, right:0,
			contents: [
				Content($, { skin:{ texture:{ path:"main.png" }, color:"white", width:64, height:64 } }),
				Text($, { left:10, right:10, style:{ font:"semibold 18px Open Sans", color:"white" }, string:$ } ),
			]
		}),
	]
}));

export default function () {
	const archive = globalThis.archive;
	let application;
	if (archive) {
		const compartment = new Compartment({
			globals: { 
				...globals, 
				archive
			},
			modules: {
				...modules, 
				mod: { archive, path:"mod" }
			},
			resolveHook(specifier) {
				return specifier;
			}
		});
		application = compartment.importNow("mod").default;
		if (application instanceof Function)
			application = application();
	}
	else 
		application = new ErrorApplication("No mod installed");
	return application;
}
