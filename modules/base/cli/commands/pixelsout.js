/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

import CLI from "cli";

CLI.install(function(command, parts) {
	if ("pixelsout" !== command)
		return false;

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
			let x = 0, y = 0;
			let width = this.pixelsout.width;
			let height = this.pixelsout.height;
			if (parts.length >= 4) {
				x = parseInt(parts[0]);
				y = parseInt(parts[1]);
				width = parseInt(parts[2]);
				height = parseInt(parts[3]);
//@@ adjust
			}

			let buffer = new ArrayBuffer(this.pixelsout.pixelsToBytes(width));
			let pixels = new Uint16Array(buffer);
			pixels.fill(Math.random() * 65535);
			this.pixelsout.begin(x, y, width, height);
			for (let i = height; i > 0; i--)
				this.pixelsout.send(buffer);
			this.pixelsout.end();
			break;

		case "help":
			this.line("pixelsout - list loaded modules");
			break;

		default:
			this.line("unknown pixelsout option");
			break;

	}
	return true;
});
