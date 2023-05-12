/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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

import MP3Streamer from "mp3streamer";

// https://docs.elevenlabs.io/api-reference/text-to-speech

export default class {
	constructor(options) {
		const {key, voice, latency, text, model, ...o} = options;
		const body = ArrayBuffer.fromString(JSON.stringify({
			text,
			model_id: model ?? "eleven_monolingual_v1",
			voice_settings: {
				stability: 0,
				similarity_boost: 0
			}
		}));
	
		return new MP3Streamer({
			...o,
			http: device.network.https,
			request: {
				method: 'POST',
				headers: new Map([
					["content-type", "application/json"],
					["accept", "audio/mpeg"],
					["xi-api-key", key],
					['content-length', body.byteLength.toString()],
				]),
				onWritable(count) {
					this.position ??= 0;
					this.write(body.slice(this.position, this.position + count))
					this.position += count
				},
			},
			port: 443,
			host: "api.elevenlabs.io",
			path: `/v1/text-to-speech/${voice}/stream?optimize_streaming_latency=${latency ?? 0}`,
		})
	}
}
