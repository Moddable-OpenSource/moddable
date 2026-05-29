/*
 * Copyright (c) 2024-2025 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import {} from "piu/MC";
import model from "model" with { type:"json" };
import assets from "assets";
import Controller from "Controller";

globalThis.importNow = function(specifier) { return native("xs_importNow").call(this, specifier); };
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
			commandListLength: 6000,
			displayListLength: 10000,
			touchCount: 1,
			behavior: controller
		}
	);
	screen.corner ??= 0;
}
