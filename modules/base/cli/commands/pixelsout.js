/*
 *     Copyright (C) 2016-2019 Moddable Tech, Inc.
 *     All rights reserved.
 */

import CLI from "cli";

CLI.install(function(command, parts) {
	if ("help" === command) {
		this.line("screen - print information");
		this.line("screen rotate degrees - rotate display");
		this.line("screen fill r g b [x y width height] - uses specified color to fill full screen or specified area");
		return;
	}

	if ("screen" !== command)
		return false;

	if (!global.screen) {
		this.line("no screen found");
		return true;
	}

	if (!parts.length) {
		this.line(`width ${screen.width}`);
		this.line(`height ${screen.height}`);
		this.line(`pixelFormat ${screen.pixelFormat}`);
		this.line(`async ${screen.async ? true : false}`);
		if (undefined === screen.rotation)
			this.line(`rotation unsupported`);
		else
			this.line(`rotation ${screen.rotation}`);
		return true;
	}

	switch (parts.shift().toLowerCase()) {
		case "rotate":
			if (undefined === screen.rotation)
				this.line(`rotation unsupported`);
			else
				screen.rotation = parseInt(parts[0]);
			break;

		case "fill":
			let r = parseInt(parts[0]), g = parseInt(parts[1]), b = parseInt(parts[2])
			let x = 0, y = 0;
			let width = screen.width;
			let height = screen.height;
			if (parts.length >= 7) {
				x = parseInt(parts[3]);
				y = parseInt(parts[4]);
				width = parseInt(parts[5]);
				height = parseInt(parts[6]);
//@@			screen.adaptInvalid()
			}

			let buffer = new ArrayBuffer(screen.pixelsToBytes(width));
			let pixels = new Uint16Array(buffer);	//@@ consider pixelFormat to make color and array type
			pixels.fill(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
			screen.begin(x, y, width, height);
			for (let i = height; i > 0; i--)
				screen.send(buffer);
			screen.end();
			break;

		default:
			this.line("unknown screen option");
			break;

	}
	return true;
});
