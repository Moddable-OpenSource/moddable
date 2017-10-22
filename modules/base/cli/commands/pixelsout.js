/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

import CLI from "cli";

CLI.install(function(command, parts) {
	if ("pixelsout" !== command)
		return false;

	if (!parts.length)
		throw new Error("missing open/close/fill");

	switch (parts.shift().toLowerCase()) {
		case "open":
			if (this.pixelsout)
				throw new Error("already open");
			let module = require(parts[0]);
			this.pixelsout = new module({});
			this.line(`width ${this.pixelsout.width}, height ${this.pixelsout.height}`);
			break;

		case "close":
			if (!this.pixelsout)
				throw new Error("none open");
//			this.pixelsout.close();
			delete this.pixelsout;
			break;

		case "fill":
			if (!this.pixelsout)
				throw new Error("none open");
			let r = parseInt(parts[0]), g = parseInt(parts[1]), b = parseInt(parts[2])
			let x = 0, y = 0;
			let width = this.pixelsout.width;
			let height = this.pixelsout.height;
			if (parts.length >= 7) {
				x = parseInt(parts[3]);
				y = parseInt(parts[4]);
				width = parseInt(parts[5]);
				height = parseInt(parts[6]);
//@@			pixelsOut.adaptInvalid()
			}

			let buffer = new ArrayBuffer(this.pixelsout.pixelsToBytes(width));
			let pixels = new Uint16Array(buffer);	//@@ consider pixelFormat to make color and array type
			pixels.fill(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
			this.pixelsout.begin(x, y, width, height);
			for (let i = height; i > 0; i--)
				this.pixelsout.send(buffer);
			this.pixelsout.end();
			break;

		case "help":
			this.line("pixelsout open name - opens PixelsOut module of 'name' and displays dimensions");
			this.line("pixelsout close - closes active PixelsOut");
			this.line("pixelsout fill r g b [x y width height] - uses specified color to fill full screen or specified area");
			break;

		default:
			this.line("unknown pixelsout option");
			break;

	}
	return true;
});
