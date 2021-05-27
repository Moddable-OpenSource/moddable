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

import {Server} from "http"
import BMPSpooler from "bmpSpooler"

const width = 120, height = 160;

let server = new Server({port: 80});
server.callback = function(message, value)
{
	if (Server.prepareResponse == message) {
		this.spooler = new BMPSpooler(width, height, (render, width, height) => {
			render.fillRectangle(render.makeColor(255, 0, 0), 0, 0, width, height);
			render.fillRectangle(render.makeColor(0, 255, 0), 16, 16, width - 32, height - 32);
			render.fillRectangle(render.makeColor(0, 0, 255), 32, 32, width - 64, height - 64);
		})
		return {headers: ["Content-Type", "image/bmp", "Content-Length", this.spooler.byteLength], body: true};
	}

	if (Server.responseFragment == message)
		return this.spooler.get(value);
}
