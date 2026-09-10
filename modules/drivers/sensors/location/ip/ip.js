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
	IP geolocation — embedded:sensor/Location/IP

	Looks up the public IP over HTTP(S). No GNSS hardware.
	Application must establish network connectivity first.
	Produces a single sample for the lifetime of the instance.

	  constructor({ http?, onSample, onError, target })
	  configure(options)
	  sample()
	  close()
	  get identification()
*/

import Timer from "timer";
import TextDecoder from "text/decoder";

const DEFAULT_ACCURACY = 25_000;
const DEFAULT_PATH = "/?fields=success,latitude,longitude,accuracy,altitude,timezone";
const PARSE_KEYS = Object.freeze(["success", "latitude", "longitude", "accuracy", "altitude", "timezone", "offset", "is_dst"]);

const identification = Object.freeze({
	model: "IP Geolocation",
	classification: "Location",
	description: "Location from public IP (GeoIP); city-scale accuracy"
});

function parseSample(json) {
	if (!json || json.success === false)
		return;

	const latitude = +json.latitude;
	const longitude = +json.longitude;
	if (!Number.isFinite(latitude) || !Number.isFinite(longitude))
		return;

	const sample = {latitude, longitude};
	const accuracy = json.accuracy != null ? +json.accuracy : DEFAULT_ACCURACY;
	if (Number.isFinite(accuracy))
		sample.accuracy = accuracy;
	if (json.altitude != null) {
		const altitude = +json.altitude;
		if (Number.isFinite(altitude))
			sample.altitude = altitude;
	}
	const zone = json.timezone;
	if (zone && typeof zone === "object") {
		if (zone.offset != null) {
			const timezone = +zone.offset;
			if (Number.isFinite(timezone))
				sample.timezone = timezone;
		}
		if (zone.is_dst != null)
			sample.dst = !!zone.is_dst;
	}
	return sample;
}

function defaultHTTP() {
	const client = globalThis.device?.network?.https?.client;
	if (!client)
		return;
	return {
		...client,
		host: "ipwho.is",
		path: DEFAULT_PATH
	};
}

class IP {
	#onSample;
	#onError;
	#http;
	#path;
	#client;
	#request;
	#start;
	#sample;
	#decoder;
	#body;

	constructor(options) {
		if ("target" in options)
			this.target = options.target;

		this.#onSample = options.onSample;
		this.#onError = options.onError;

		try {
			const http = options.http ?? defaultHTTP();
			if (!http?.io)
				throw new Error("http client not available");
			this.#http = http;
			this.#path = http.path ?? DEFAULT_PATH;

			this.#start = Timer.set(() => {
				this.#start = undefined;
				this.#fetch();
			});
		}
		catch (e) {
			this.close();
			throw e;
		}
	}

	configure(options) {
		if (!this.#http)
			throw new Error("closed");
	}

	sample() {
		if (!this.#http)
			throw new Error("closed");
		const sample = this.#sample;
		this.#sample = undefined;
		return sample;
	}

	get identification() {
		if (!this.#http)
			throw new Error("closed");
		return { ...identification };
	}

	close() {
		const client = this.#client;
		this.#client = undefined;
		this.#request = undefined;
		this.#http = undefined;
		Timer.clear(this.#start);
		this.#start = undefined;
		client?.close();
	}

	#fetch() {
		if (!this.#http)
			return;

		this.#body = undefined;
		this.#decoder = new TextDecoder;

		try {
			this.#client?.close();
			this.#client = new this.#http.io({
				...this.#http,
				onError: e => this.#fail(e)
			});

			this.#request = this.#client.request({
				path: this.#path,
				method: "GET",
				headers: new Map([
					["user-agent", "moddable-location-ip"]
				]),
				onHeaders: (status) => {
					if (status < 200 || status >= 300)
						this.#fail(new Error(`HTTP ${status}`));
				},
				onReadable: (count) => {
					if (!this.#request)
						return;
					const chunk = this.#request.read(count);
					if (!chunk)
						return;
					this.#body = (this.#body ?? "") + this.#decoder.decode(chunk, {stream: true});
				},
				onDone: (error) => {
					if (!this.#http)
						return;
					this.#request = undefined;
					if (error) {
						this.#fail(error);
						return;
					}
					try {
						const json = JSON.parse((this.#body ?? "") + this.#decoder.decode(), PARSE_KEYS);
						const sample = parseSample(json);
						if (!sample) {
							this.#fail(new Error("unrecognized geolocation response"));
							return;
						}
						sample.timestamp = Date.now();
						this.#sample = sample;
						this.#client?.close();
						this.#client = undefined;
						this.#onSample?.call(this);
					}
					catch (e) {
						this.#fail(e);
					}
				}
			});
		}
		catch (e) {
			this.#fail(e);
		}
	}

	#fail(e) {
		if (!this.#http)
			return;
		const onError = this.#onError;
		this.close();
		onError?.call(this, e);
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default IP;
