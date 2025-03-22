/*
 * Copyright (c) 2016-2024  Moddable Tech, Inc.
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
import Bitmap from "commodetto/Bitmap";
import Camera from "embedded:x-io/imagein/camera";
import Net from "net";

let width = 176, height = 144;
let frame;

const camera = new Camera({
	width,
	height,
	imageType: Bitmap.RGB565LE,
	format: "buffer/disposable",
	onReadable: () => {
		if (frame?.inUse) {
			const f = camera.read();		// ignore frame
			f?.close();
			return;
		}
		frame?.close();
		frame = camera.read();
		frame.inUse = 0;
	}
});
width = camera.width;
height = camera.height;
camera.start();

const port = 8080;
const server = new Server({port});
server.callback = function(message, value)
{
	if (Server.prepareResponse == message) {
		if (!frame)
			return;

		frame.bitmap ??= new Bitmap(width, height, camera.imageType, frame, 0);
		frame.inUse += 1;
		this.spooler = new BMPSpooler(width, height, (render, width, height) => {
			render.drawBitmap(frame.bitmap, 0, 0);
		})
		return {headers: ["Content-Type", "image/bmp", "Content-Length", this.spooler.byteLength], body: true};
	}
	else if (Server.responseFragment == message)
		return this.spooler.get(value);
	else if ((Server.responseComplete == message) || (Server.error == message)) {
		if (frame)
			frame.inUse -= 1;
	}
}

trace(`Server ready at: http://${Net.get("IP")}:${port}\n`);
