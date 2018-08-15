/*
 * Copyright (c) 2018  Moddable Tech, Inc. 
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
import AudioIn from "audioin"
import {Server} from "http"

class WavServer extends Server {
	callback(message, value) {
		if (2 == message) {
			trace(`New connection\n`);
			this.sentWavHeader = false;
		}
		else if (8 == message)
			return {headers: ["Content-type", "audio/wav"], body: true};
		else if (9 === message) {
			if (!this.audioIn) {
				this.audioIn = new AudioIn;
				this.samplesRemaining = 6 * this.audioIn.sampleRate;
				trace(`created audioin\n`);
			}

			if (!this.sentWavHeader) {
				const header = new DataView(new ArrayBuffer(44));
				header.setUint8(0, 'R'.charCodeAt());
				header.setUint8(1, 'I'.charCodeAt());
				header.setUint8(2, 'F'.charCodeAt());
				header.setUint8(3, 'F'.charCodeAt());
				header.setUint32(4, 36 + (this.samplesRemaining * this.audioIn.numChannels * (this.audioIn.bitsPerSample >> 3)), true);
				header.setUint8(8, 'W'.charCodeAt());
				header.setUint8(9, 'A'.charCodeAt());
				header.setUint8(10, 'V'.charCodeAt());
				header.setUint8(11, 'E'.charCodeAt());
				header.setUint8(12, 'f'.charCodeAt());
				header.setUint8(13, 'm'.charCodeAt());
				header.setUint8(14, 't'.charCodeAt());
				header.setUint8(15, ' '.charCodeAt());
				header.setUint32(16, 16, true);																		// fmt chunk size
				header.setUint16(20, 1, true);																		// pcm
				header.setUint16(22, this.audioIn.numChannels, true);												// mono
				header.setUint32(24, this.audioIn.sampleRate, true);												// sample rate
				header.setUint32(28, this.audioIn.sampleRate * this.audioIn.numChannels * (this.audioIn.bitsPerSample >> 3), true);		// byte rate: SampleRate * NumChannels * BitsPerSample/8
				header.setUint16(32, (1 * this.audioIn.bitsPerSample) >> 3, true);									// block align: NumChannels * BitsPerSample/8
				header.setUint16(34, this.audioIn.bitsPerSample, true);												// bits per sample
				header.setUint8(36, 'd'.charCodeAt());
				header.setUint8(37, 'a'.charCodeAt());
				header.setUint8(38, 't'.charCodeAt());
				header.setUint8(39, 'a'.charCodeAt());
				header.setUint32(40, this.samplesRemaining * this.audioIn.numChannels * (this.audioIn.bitsPerSample >> 3), true);

				this.sentWavHeader = true;
				return header.buffer;
			}

			if (!this.samplesRemaining)
				return;

			value = Math.min(value, 1400);
			let samplesToRead = Math.min(this.samplesRemaining, value / (this.audioIn.numChannels * (this.audioIn.bitsPerSample >> 3)))
			this.samplesRemaining -= samplesToRead;
			trace(`${this.samplesRemaining} samplesRemaining\n`);
			return this.audioIn.read(samplesToRead);
		}
		else if ((message < 0) || (10 === message)) {
			trace(`CLOSE\n`);
			if (this.audioIn)
				this.audioIn.close();
		}
	}
}

export default function () {
	new WavServer({});
}

