/*---
description: 
flags: [module]
---*/

import {Mixer} from "pins/audioout"
import Resource from "Resource";
import {Digest} from "crypt";
import {MAUD, SampleFormat} from "./maud.js";

// ffmpeg -i $MODDABLE/examples/assets/sounds/bflatmajor.wav -ac 1 -ar 16000 -b:a 32k ~/bflatmajor-sbc.sbc
const sbc = new Resource("bflatmajor-sbc.sbc");

const bytesPerChunk = 32;
const samplesPerChunk = 128;

let buffers = Math.idiv(sbc.byteLength, bytesPerChunk);

const sampleRate = 16000;
const samples = new Uint8Array(new SharedArrayBuffer(MAUD.byteLength + sbc.byteLength));
samples.set(new Uint8Array(sbc), MAUD.byteLength)
const maud = new MAUD(samples.buffer);

maud.tag = "ma";
maud.version = 1;
maud.bitsPerSample = 16;
maud.sampleRate = sampleRate;
maud.numChannels = 1;
maud.bufferSamples = Math.idiv(sbc.byteLength, bytesPerChunk) * samplesPerChunk;
maud.sampleFormat = SampleFormat.SBC;

const mixer = new Mixer({streams: 1, sampleRate, numChannels: 1});

let digest = new Digest("MD5");

mixer.enqueue(0, Mixer.Samples, samples);
for (let i = 0; i < buffers + 2; i++)
	digest.write(mixer.mix(samplesPerChunk));
verifyChecksum("ee5e70299c0730db30d833c2a207496e");

mixer.enqueue(0, Mixer.Samples, samples);
for (let i = 0; i < buffers + 2; i++)
	digest.write(mixer.mix(samplesPerChunk));
verifyChecksum("3817f2551ce7fcf4de5596ed32ad1bbb");		// decoder keeps some state... so not quite identical second time!

mixer.enqueue(0, Mixer.Flush);
mixer.enqueue(0, Mixer.Samples, samples);
for (let i = 0; i < buffers + 2; i++)
	digest.write(mixer.mix(samplesPerChunk));
verifyChecksum("ee5e70299c0730db30d833c2a207496e");		// flush resets the decoder state

mixer.enqueue(0, Mixer.Samples, samples);
for (let i = 0; i < buffers + 2; i += 1) {
	digest.write(mixer.mix(3));
	digest.write(mixer.mix(samplesPerChunk - 3));
}
verifyChecksum("3817f2551ce7fcf4de5596ed32ad1bbb");

mixer.enqueue(0, Mixer.Samples, samples);
let remain = (buffers + 2) * samplesPerChunk;
while (remain > samplesPerChunk) {
	digest.write(mixer.mix(50));
	remain -= 50;
}
digest.write(mixer.mix(remain));
verifyChecksum("3817f2551ce7fcf4de5596ed32ad1bbb");

function verifyChecksum(expected) {
	const bytes = new Uint8Array(digest.close());
	let checksum = [];
	for (let i = 0, length = bytes.length; i < length; i++)
		checksum.push(bytes[i].toString(16).padStart(2, "0"));
	assert.sameValue(checksum.join(""), expected);
	
	digest.reset();
	$262.gc();
}
