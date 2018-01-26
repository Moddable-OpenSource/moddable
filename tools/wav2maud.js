import {TOOL, FILE} from "tool";
import Resampler from "resampler";

export default class extends TOOL {
	constructor(argv) {
		super(argv);

		this.inputPath = "";
		this.outputPath = "";

		this.output = {
			sampleRate: 11025,
			numChannels: 1,
			bitsPerSample: 8
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

		let parts = this.splitPath(this.inputPath);
		parts.extension = ".maud";
		parts.directory = this.outputPath;
		this.outputPath = this.joinPath(parts);

		this.wav = new WavReader(this.readFileBuffer(this.inputPath));

		this.wavSamples = new Float32Array(1024 * this.wav.numChannels);

		this.resampler = new Resampler(this.wav.sampleRate, this.output.sampleRate, this.wav.numChannels, this.wavSamples);

		if (this.output.numChannels > this.wav.numChannels)
			throw Error("no mono to stereo conversion");
	}

	run() {
		let outputFile = new FILE(this.outputPath, "wb");

		let header = new DataView(new ArrayBuffer(12));
		header.setUint8(0, 'm'.charCodeAt(0));
		header.setUint8(1, 'a'.charCodeAt(0));
		header.setUint8(2, 1);		// version
		header.setUint8(3, this.output.bitsPerSample);
		header.setUint16(4, this.output.sampleRate, true);		// little-endian
		header.setUint8(6, this.output.numChannels);
		header.setUint8(7, 0);		// unused
		header.setUint32(8, Math.floor((this.wav.samples / this.wav.sampleRate) * this.output.sampleRate), true)
		outputFile.writeBuffer(header.buffer);

		let resampled = this.resampler.outputBuffer;
		let finalSamples = (8 === this.output.bitsPerSample) ? new Int8Array(this.wavSamples.length) : new Int16Array(this.wavSamples.length);

		while (this.wav.samples) {
			// get next buffer of samples
			let use = this.wavSamples.length / this.wav.numChannels;
			if (use > this.wav.samples)
				use = this.wav.samples;
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
			let byteLength = use * ((this.output.bitsPerSample * this.output.numChannels) >> 3);
			outputFile.writeBuffer(finalSamples.buffer.slice(0, byteLength));
		}

		outputFile.close();
	}
}

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

		if ("fmt " !== this.readFourCC())
			throw new Error("expected fmt");
		let next = this.readUint32();
		next += this.position;

		this.audioFormat = this.readUint16();
		if (1 !== this.audioFormat)
			throw new Error("unsupported format");

		this.numChannels = this.readUint16();
		if ((1 !== this.numChannels) && (2 !== this.numChannels))
			throw new Error("unsupported channels");

		this.sampleRate = this.readUint32();
		this.seekBy(4 + 2);
		this.bitsPerSample = this.readUint16();
		if ((8 !== this.bitsPerSample) && (16 !== this.bitsPerSample))
			throw new Error("unsupported bitsPerSample");

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
