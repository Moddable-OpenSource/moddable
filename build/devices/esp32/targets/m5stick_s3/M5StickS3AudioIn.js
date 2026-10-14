/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

import AudioIn from "embedded:io/audio/in-original";
import ES8311 from "m5stick_s3/es8311";
import AudioResource from "m5stick_s3/audio-resource";

export default class M5StickS3AudioIn extends AudioIn {
	#started = false;

	constructor(options) {
		AudioResource.checkAvailable();
		super(options);
		AudioResource.acquire(this);
	}

	start() {
		super.start();
		if (this.#started)
			return;
		try {
			const codec = new ES8311({
				...device.i2c.internal,
				io: device.io.SMBus
			});
			try {
				codec.configureInput();
			}
			finally {
				codec.close();
			}
			this.#started = true;
		}
		catch (e) {
			super.stop();
			throw e;
		}
	}

	stop() {
		super.stop();
		this.#started = false;
	}

	close() {
		super.close();
		this.#started = false;
		AudioResource.release(this);
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}
