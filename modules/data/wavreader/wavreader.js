/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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

class WavReader {
	constructor(buffer) {
		this.wav = new DataView(buffer);
		this.position = 0;
		this.waveSize = buffer.byteLength;

		if ("RIFF" !== this.readFourCC())
			throw new Error("expected RIFF");
		this.seekBy(4);		// file size
		if ("WAVE" !== this.readFourCC())
			throw new Error("expected WAVE");

		if ("JUNK" === this.readFourCC())
			this.seekBy(this.readUint32())
		else
			this.seekBy(-4);

		if ("fmt " !== this.readFourCC())
			throw new Error("expected fmt");
		let next = this.readUint32();
		next += this.position;

		this.audioFormat = this.readUint16();
		this.numChannels = this.readUint16();
		this.sampleRate = this.readUint32();
		this.seekBy(4 + 2);
		this.bitsPerSample = this.readUint16();

		this.seekTo(next);

		while ("data" !== this.readFourCC())
			this.seekBy(this.readUint32());

		this.samples = Math.floor(this.readUint32() / ((this.numChannels * this.bitsPerSample) >> 3));
	}

	getSamples(buffer, count) {		// always returns signed 16-bit sample values
		this.samples -= count;
		if (this.samples < 0)
			throw Error("out of samples");

		count *= this.numChannels;
		let i = 0;
		if (16 === this.bitsPerSample) {
			while (count--)
				buffer[i++] = this.readInt16();
		}
		else
		if (8 == this.bitsPerSample) {
			while (count--) {
				let value = this.readInt8();
				buffer[i++] = value << 8;		// write Uint8 representation of value into low bits
			}
		}
	}

	readFourCC() {
		let result = String.fromCharCode(this.wav.getUint8(this.position), this.wav.getUint8(this.position + 1),
										 this.wav.getUint8(this.position + 2), this.wav.getUint8(this.position + 3));
		this.position += 4;
		if (this.position > this.waveSize)
			throw new Error("eof");
		return result;
	}
	readUint32() {
		let result = this.wav.getUint32(this.position, true);
		this.position += 4;
		if (this.position > this.waveSize)
			throw new Error("eof");
		return result;
	}
	readUint16() {
		let result = this.wav.getUint16(this.position, true);
		this.position += 2;
		if (this.position > this.waveSize)
			throw new Error("eof");
		return result;
	}
	readInt16() {
		let result = this.wav.getInt16(this.position, true);
		this.position += 2;
		if (this.position > this.waveSize)
			throw new Error("eof");
		return result;
	}
	readUint8() {
		let result = this.wav.getUint8(this.position, true);
		this.position += 1;
		if (this.position > this.waveSize)
			throw new Error("eof");
		return result;
	}
	readInt8() {
		let result = this.wav.getInt8(this.position, true);
		this.position += 1;
		if (this.position > this.waveSize)
			throw new Error("eof");
		return result;
	}
	seekBy(count) {
		this.position += count;
	}
	seekTo(position) {
		this.position = position;
	}
}

export default WavReader;
