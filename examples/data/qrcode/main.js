/*
 * Copyright (c) 2018-2021  Moddable Tech, Inc.
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
import qrCode from "qrcode";
import Poco from "commodetto/Poco";

// generate QR code
let qr = qrCode({input: "Welcome to the QRCode example in the Moddable SDK. The JavaScript date is now " + (new Date())});
let size = qr.size;

// render QR code to screen
let render = new Poco(screen);

let margin = 10;
let available = Math.min(render.width - (margin * 2), render.height - (margin * 2));
let pixels = Math.floor(available / size);
margin += (available - (pixels * size)) >> 1;

render.begin();
render.fillRectangle(render.makeColor(255, 255, 255), 0, 0, render.width, render.height);
render.drawQRCode(qr, margin, margin, pixels, render.makeColor(0, 0, 255));
render.end();

// trace QR code to console
qr = new Uint8Array(qr);

for (let y = 0; y < size; y++) {
	for (let x = 0; x < size; x++) {
		if (qr[(y * size) + x])
			trace("X");
		else
			trace(".");
	}
	trace("\n");
}
