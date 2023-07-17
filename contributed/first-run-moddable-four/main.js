/*
 * Copyright (c) 2016-2020 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import {} from "piu/MC";
import model from "model";
import Controller from "Controller";
import Power from "power";

Power.setup();
globalThis.importNow = function(specifier) @ "xs_importNow";
globalThis.localize = function(it) {
	if (it) {
		let result = locals.get(it);
		if (result === undefined) {
			trace("Missing local: \"", it, "\"\n");
			result = it;
		}
		return result;
    }
    return it;
}

export default function() {
// 	globalThis.locals = new Locals;
	globalThis.controller = new Controller;
	globalThis.application = new Application(
		{ 
			model
		}, 
		{
			commandListLength: 2048,
			displayListLength: 3072,
			pixels: 128 * 8,
			touchCount: 0,
			behavior: controller
		}
	);
}
