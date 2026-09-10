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

/*
	GPS / GNSS over UART — embedded:sensor/Location/GPS-UART

	protocol is a constructor (data/location/nmea, data/location/ubx, or another
	parser with the same static surface: location, identification, optional setup).

	  constructor({ sensor, protocol, onSample, onError, target })
	  configure({ enableHighAccuracy, timeout, maximumAge })
	    enableHighAccuracy is accepted for Location API alignment; currently a no-op
	    (see modules/drivers/sensors/location/gps-uart/readme.md).
	  sample()
	  close()
	  get identification()
*/

import Timer from "timer";

const DEFAULT_BAUD = 9600;

function roundTo(value, factor) {
	return Math.round(value * factor) / factor;
}

class GPS {
	#io;
	#Protocol;
	#parser;
	#onSample;
	#onError;
	#timeout = 0;
	#timeoutTimer;
	#maximumAge = 0;
	#sample;
	#published = false;
	#latest = {
		latitude: undefined,
		longitude: undefined,
		altitude: undefined,
		accuracy: undefined,
		altitudeAccuracy: undefined,
		heading: undefined,
		speed: undefined,
		fix: false,
		updated: 0
	};

	constructor(options) {
		if ("target" in options)
			this.target = options.target;

		this.#onSample = options.onSample;
		this.#onError = options.onError;

		const Protocol = options.protocol;
		if (typeof Protocol !== "function")
			throw new Error("invalid GPS protocol");
		this.#Protocol = Protocol;
		this.#parser = new Protocol(parsed => {
			const update = Protocol.location(parsed);
			if (update)
				this.#merge(update);
		});

		try {
			this.#io = new options.sensor.io({
				baud: DEFAULT_BAUD,
				...options.sensor,
				format: "buffer",
				target: this,
				onReadable() {
					this.target.#onReadable();
				}
			});

			const frames = Protocol.setup?.();
			if (frames) {
				for (let i = 0; i < frames.length; i++) {
					const frame = frames[i];
					const bytes = frame instanceof ArrayBuffer
						? new Uint8Array(frame)
						: new Uint8Array(frame.buffer, frame.byteOffset, frame.byteLength);
					this.#io.write(bytes);
				}
			}
		}
		catch (e) {
			this.close();
			throw e;
		}
	}

	configure(options) {
		if (!this.#io)
			throw new Error("closed");

		if ("maximumAge" in options)
			this.#maximumAge = options.maximumAge;

		if ("timeout" in options) {
			Timer.clear(this.#timeoutTimer);
			this.#timeoutTimer = undefined;
			this.#timeout = options.timeout;
			if (this.#timeout > 0 && !this.#published) {
				this.#timeoutTimer = Timer.set(() => {
					this.#timeoutTimer = undefined;
					this.#fail(new Error("timeout"));
				}, this.#timeout);
			}
		}
	}

	sample() {
		if (!this.#io)
			throw new Error("closed");
		const sample = this.#sample;
		if (sample && this.#maximumAge > 0 && sample.timestamp &&
				(Date.now() - sample.timestamp) > this.#maximumAge) {
			this.#sample = undefined;
			return;
		}
		this.#sample = undefined;
		return sample;
	}

	get identification() {
		if (!this.#io)
			throw new Error("closed");
		return { ...this.#Protocol.identification };
	}

	close() {
		const io = this.#io;
		this.#io = undefined;
		Timer.clear(this.#timeoutTimer);
		this.#timeoutTimer = undefined;
		io?.close();
	}

	#onReadable() {
		try {
			const chunk = this.#io.read();
			if (chunk)
				this.#parser.push(chunk);
		}
		catch (e) {
			this.#fail(e);
		}
	}

	#merge(update) {
		const s = this.#latest;
		let changed = false;

		if ("fix" in update && !update.fix) {
			const hadFix = s.latitude != null;
			s.latitude = undefined;
			s.longitude = undefined;
			s.altitude = undefined;
			s.accuracy = undefined;
			s.altitudeAccuracy = undefined;
			s.heading = undefined;
			s.speed = undefined;
			s.fix = false;
			if (hadFix) {
				this.#sample = undefined;
				this.#onSample?.call(this);
			}
			return;
		}

		if (update.latitude != null && update.longitude != null) {
			const lat = roundTo(update.latitude, 1e6);
			const lon = roundTo(update.longitude, 1e6);
			if (lat !== s.latitude || lon !== s.longitude)
				changed = true;
			s.latitude = lat;
			s.longitude = lon;
			s.updated = Date.now();
			s.fix = true;
		}
		if (update.altitude != null) {
			const altitude = roundTo(update.altitude, 10);
			if (altitude !== s.altitude)
				changed = true;
			s.altitude = altitude;
		}
		if (update.accuracy != null) {
			const accuracy = roundTo(update.accuracy, 10);
			if (accuracy !== s.accuracy)
				changed = true;
			s.accuracy = accuracy;
		}
		if (update.altitudeAccuracy != null) {
			const altitudeAccuracy = roundTo(update.altitudeAccuracy, 10);
			if (altitudeAccuracy !== s.altitudeAccuracy)
				changed = true;
			s.altitudeAccuracy = altitudeAccuracy;
		}
		if (update.heading != null) {
			const heading = roundTo(update.heading, 10);
			if (heading !== s.heading)
				changed = true;
			s.heading = heading;
		}
		if (update.speed != null) {
			const speed = roundTo(update.speed, 100);
			if (speed !== s.speed)
				changed = true;
			s.speed = speed;
		}

		if (!changed || s.latitude == null || s.longitude == null)
			return;

		const sample = {
			latitude: s.latitude,
			longitude: s.longitude
		};
		if (s.updated > 0)
			sample.timestamp = s.updated;
		if (s.altitude != null)
			sample.altitude = s.altitude;
		if (s.altitudeAccuracy != null)
			sample.altitudeAccuracy = s.altitudeAccuracy;
		if (s.accuracy != null)
			sample.accuracy = s.accuracy;
		if (s.heading != null)
			sample.heading = s.heading;
		if (s.speed != null)
			sample.speed = s.speed;

		this.#sample = sample;
		this.#published = true;
		Timer.clear(this.#timeoutTimer);
		this.#timeoutTimer = undefined;
		this.#onSample?.call(this);
	}

	#fail(e) {
		if (!this.#io)
			return;
		const onError = this.#onError;
		this.close();
		onError?.(e);
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default GPS;
