/*
 * Copyright (c) 2023 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */
 
import Flash from "flash";
import parseBMF from "commodetto/parseBMF";
import Poco from "commodetto/Poco";
import Resource from "Resource";

notify("Started");

try {
	const flash = new Flash("ble-peers");
	for (let offset = 0; offset < flash.byteLength; offset += flash.blockSize)
		flash.erase(Math.idiv(offset, flash.blockSize));
	notify("Cleared");
}
catch (e) {
	notify("Error");
}

function notify(msg) {
	trace(msg, "\n");

	if (!globalThis.screen) return;

	if (!globalThis.render) {
		globalThis.render = new Poco(screen);

		render.black = render.makeColor(0, 0, 0);
		render.white = render.makeColor(255, 255, 255);
		render.font = parseBMF(new Resource("OpenSans-Bold-32.bf4"));
	}

	render.begin();
	render.fillRectangle(render.white, 0, 0, render.width, render.height);
	render.drawText(msg, render.font, render.black,
		(render.width - render.getTextWidth(msg, render.font)) >> 1,
		((render.height - render.font.height) >> 1));
	render.end();
}
