/*
 * Copyright (c) 2021  Moddable Tech, Inc.
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

import Timer from "timer";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import TextDecoder from "text/decoder";
import TextEncoder from "text/encoder";

class App {
	font = new Resource("8x8font.dat");
	render = new Poco(screen);
	black = this.render.makeColor(0, 0, 0);
	white = this.render.makeColor(255, 255, 255);

	sent = 0;
	received = 0;

	decoder = new TextDecoder;
	encoder = new TextEncoder;

	constructor() {
		this.lora = new device.peripheral.lora.Default({
			onReadable: () => this.receive(this.lora.read())
		});

		this.button = new device.peripheral.button.Flash({
			onPush: () => {
				if (this.button.pressed)
					this.send();
			}
		});
		
		this.update();
	}
	send() {
		this.sent += 1;
		this.messageSent = `hello #${this.sent}`
		this.lora.write(this.encoder.encode(this.messageSent));
		this.update();
	}
	receive(buffer) {
		this.received += 1;
		this.messageReceived = this.decoder.decode(buffer); 
		this.update();
	}
	update() {
		const {render, font, black, white} = this;
		render.begin();
			render.fillRectangle(black, 0, 0, render.width, render.height);

			render.drawText(`Sent ${this.sent}`, font, white, 0, 0);
			if (this.messageSent)
				render.drawText(`"${this.messageSent}"`, font, white, 0, 12);

			render.fillRectangle(white, 0, 25, render.width, 1);

			render.drawText(`Received ${this.received}`, font, white, 0, 29);
			if (this.messageReceived)
				render.drawText(`"${this.messageReceived}"`, font, white, 0, 41);
		render.end();
	}
}

globalThis.app = new App;
