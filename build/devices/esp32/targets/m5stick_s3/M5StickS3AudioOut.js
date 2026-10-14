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

import AudioOut from "embedded:io/audio/out-original";
import M5PM1 from "m5pm1";
import ES8311 from "m5stick_s3/es8311";
import AudioResource from "m5stick_s3/audio-resource";

class M5StickS3AudioOut extends AudioOut {
	#amplifierEnabled = false;

	constructor(options) {
		AudioResource.checkAvailable();
		super(options);
		AudioResource.acquire(this);
	}

	start() {
		super.start();
		if (this.#amplifierEnabled)
			return;
		try {
			const codec = new ES8311({
				...device.i2c.internal,
				io: device.io.SMBus
			});
			try {
				codec.configureOutput();
			}
			finally {
				codec.close();
			}
			setAmplifier(true);
			this.#amplifierEnabled = true;
		}
		catch (e) {
			super.stop();
			throw e;
		}
	}

	stop() {
		if (this.#amplifierEnabled) {
			setAmplifier(false);
			this.#amplifierEnabled = false;
		}
		super.stop();
	}

	close() {
		try {
			if (this.#amplifierEnabled)
				setAmplifier(false);
		}
		finally {
			this.#amplifierEnabled = false;
			super.close();
			AudioResource.release(this);
		}
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

M5StickS3AudioOut.Async = class extends M5StickS3AudioOut {
	write(samples, callback) {
		return AudioOut.Async.prototype.write.call(this, samples, callback);
	}
};

function setAmplifier(enabled) {
	const pmic = new M5PM1({
		...device.i2c.internal,
		io: device.io.SMBus
	});
	try {
		pmic.speakerEnabled = enabled;
	}
	finally {
		pmic.close();
	}
}

export default M5StickS3AudioOut;
