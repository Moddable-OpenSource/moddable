/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import {TOOL, FILE} from "tool";
import Resampler from "resampler";
import WavReader from "wavreader"

export default class extends TOOL {
	constructor(argv) {
		super(argv);

		this.inputPath = "";
		this.outputPath = "";

		this.output = {
			sampleRate: 11025,
			numChannels: 1,
			bitsPerSample: 8,
			format: "uncompressed"
		};

		let argc = argv.length;
		for (let argi = 1; argi < argc; argi++) {
			let option = argv[argi], name, path;
			switch (option) {
			case "-o":
				argi++;
				if (argi >= argc)
					throw new Error("-o: no directory!");
				name = argv[argi];
				if (this.outputPath)
					throw new Error("-o '" + name + "': too many directories!");
				path = this.resolveDirectoryPath(name);
				if (!path)
					throw new Error("-o '" + name + "': directory not found!");
				this.outputPath = path;
				break;

			case "-r":
				argi++;
				if (argi >= argc)
					throw new Error("-o: no sample rate!");
				this.output.sampleRate = parseInt(argv[argi]);
				if ((this.output.sampleRate < 8000) || (this.output.sampleRate > 48000))
					throw new Error("sample rate must be from 8000 to 48000");
				break;

			case "-s":
				argi++;
				if (argi >= argc)
					throw new Error("-o: no bitsPerSample!");
				this.output.bitsPerSample = parseInt(argv[argi]);
				if ((8 != this.output.bitsPerSample) && (16 != this.output.bitsPerSample))
					throw new Error("bitsPerSample must be 8 or 16");
				break;

			case "-c":
				argi++;
				if (argi >= argc)
					throw new Error("-o: no channels!");
				this.output.numChannels = parseInt(argv[argi]);
				if ((1 != this.output.numChannels) && (2 != this.output.numChannels))
					throw new Error("channels must be 1 or 2");
				break;

			case "-f":
				argi++;
				if (argi >= argc)
					throw new Error("-o: no format!");
				if (("uncompressed" !== argv[argi]) && ("ima" !== argv[argi]))
					throw new Error("unknown format!");
				this.output.format = argv[argi];
				break;

			default:
				name = argv[argi];
				if (this.inputPath)
					throw new Error("'" + name + "': too many files!");
				path = this.resolveFilePath(name);
				if (!path)
					throw new Error("'" + name + "': file not found!");
				this.inputPath = path;
				break;
			}
		}

		if (!this.inputPath)
			throw new Error("no file!");

		if (!this.outputPath)
			this.outputPath = this.splitPath(this.inputPath).directory;

		if ("ima" === this.output.format) {
			if (1 !== this.output.numChannels)
				throw new Error("ima must be mono");
			this.output.bitsPerSample = 16;
		}

		let parts = this.splitPath(this.inputPath);
		parts.extension = ".maud";
		parts.directory = this.outputPath;
		this.outputPath = this.joinPath(parts);

		this.wav = new WavReader(this.readFileBuffer(this.inputPath));
		if (1 !== this.wav.audioFormat)
			throw new Error("unsupported format");
		if ((8 !== this.wav.bitsPerSample) && (16 !== this.wav.bitsPerSample))
			throw new Error("unsupported bitsPerSample");
		if ((1 !== this.wav.numChannels) && (2 !== this.wav.numChannels))
			throw new Error("unsupported channels");

		this.wavSamples = new Float32Array(8192 * this.wav.numChannels);

		this.resampler = new Resampler(this.wav.sampleRate, this.output.sampleRate, this.wav.numChannels, this.wavSamples);

		if (this.output.numChannels > this.wav.numChannels)
			throw Error("no mono to stereo conversion");
	}

	run() {
		const imaSamplesPerChunk = 129;
		const imaOutput = "ima" === this.output.format;

		let outputFile = new FILE(this.outputPath, "wb");

		let header = new DataView(new ArrayBuffer(12));
		header.setUint8(0, 'm'.charCodeAt(0));
		header.setUint8(1, 'a'.charCodeAt(0));
		header.setUint8(2, 1);		// version
		header.setUint8(3, this.output.bitsPerSample);
		header.setUint16(4, this.output.sampleRate, true);		// little-endian
		header.setUint8(6, this.output.numChannels);
		header.setUint8(7, imaOutput ? 1 : 0);		// sample format
		let sampleCount = Math.floor((this.wav.samples / this.wav.sampleRate) * this.output.sampleRate);
		if (imaOutput)
			sampleCount = Math.idiv(sampleCount, imaSamplesPerChunk) * imaSamplesPerChunk;		// quantize
		header.setUint32(8, sampleCount, true)
		outputFile.writeBuffer(header.buffer);

		let resampled = this.resampler.outputBuffer;
		let finalSamples = (8 === this.output.bitsPerSample) ? new Int8Array(this.wavSamples.length) : new Int16Array(this.wavSamples.length);

		let imaBuffer = new Int16Array(imaSamplesPerChunk);
		imaBuffer.samplesInBuffer = 0;

		let totalOut = 0;
		while (this.wav.samples) {
			// get next buffer of samples
			let use = this.wavSamples.length / this.wav.numChannels;
			if (use > this.wav.samples)
				use = this.wav.samples;
			use = Math.min(1024, use);
			this.wav.getSamples(this.wavSamples, use);

			// apply output sample rate
			use = this.resampler.resampler(use * this.wav.numChannels) / this.wav.numChannels;

			// apply output numChannels (stereo to mono)
			if ((2 == this.wav.numChannels) && (1 == this.output.numChannels)) {
				for (let i = 0, j = 0; i < use; i++, j += 2)
					resampled[i] = (resampled[j] + resampled[j + 1]) / 2;
			}

			// apply output bitsPerSample to generate finalSamples
			if (16 == this.output.bitsPerSample) {			// -32768 to 32767
				for (let i = 0, length = use * this.wav.numChannels; i < length; i++)
					finalSamples[i] = resampled[i];
			}
			else if (8 == this.output.bitsPerSample) {		// -128 to 127
				for (let i = 0, length = use * this.wav.numChannels; i < length; i++)
					finalSamples[i] = resampled[i] >> 8;
			}

			// output samples
			if ("uncompressed" === this.output.format) {
				let byteLength = use * ((this.output.bitsPerSample * this.output.numChannels) >> 3);
				outputFile.writeBuffer(finalSamples.buffer.slice(0, byteLength));
				totalOut += use;
			}
			else if (imaOutput) {
				let pos = 0;
				while (pos < use) {
					let needed = imaSamplesPerChunk - imaBuffer.samplesInBuffer;
					if (needed > (use - pos))
						needed = (use - pos);

					imaBuffer.set(new Int16Array(finalSamples.buffer, pos << 1, needed), imaBuffer.samplesInBuffer);

					imaBuffer.samplesInBuffer += needed;
					if (imaBuffer.samplesInBuffer < imaSamplesPerChunk)
						break;

					outputFile.writeBuffer(this.compressIMA(imaBuffer.buffer, imaSamplesPerChunk));
					imaBuffer.samplesInBuffer = 0;
					pos += needed;
					totalOut += imaSamplesPerChunk;
				}
			}
		}
		
		if (totalOut > sampleCount)
			trace("too much output!\n");
		else if (totalOut < sampleCount) {
			if ("uncompressed" === this.output.format) {
				const needed = (sampleCount - totalOut) * ((this.output.bitsPerSample * this.output.numChannels) >> 3);
				outputFile.writeBuffer(new ArrayBuffer(needed));
			}
			else
				trace("IMA padding needed!\n");
		}

		// if IMA, may be a partial buffer pending
		outputFile.close();
	}
	compressIMA(samples) @ "Tool_compressIMA";
}
