/*---
description: 
flags: [module]
---*/

import {Mixer} from "pins/audioout"
import Resource from "Resource";
import {Digest} from "crypt";

const maud = new Resource("bflatmajor-ima-12000.maud");
const imaSamplesPerChunk = 129;
const imaChunkSize = 68;
const buffers = Math.idiv(maud.byteLength - 12, imaChunkSize);
assert(!(buffers % 2));

const mixer = new Mixer({streams: 1, sampleRate: 12000, numChannels: 1});

let digest = new Digest("MD5");

mixer.enqueue(0, Mixer.Samples, maud);
for (let i = 0; i < buffers + 2; i++) {
	const result = mixer.mix(imaSamplesPerChunk);
	digest.write(result);
}
verifyChecksum("5e8738767e264280beb45ed6ceb2d037");

mixer.enqueue(0, Mixer.Samples, maud);
for (let i = 0; i < buffers + 2; i += 1) {
	const result = mixer.mix(imaSamplesPerChunk);
	digest.write(result);
}
verifyChecksum("5e8738767e264280beb45ed6ceb2d037");

mixer.enqueue(0, Mixer.Samples, maud);
for (let i = 0; i < buffers + 2; i += 2) {
	const result = mixer.mix(imaSamplesPerChunk * 2);
	digest.write(result);
}
verifyChecksum("5e8738767e264280beb45ed6ceb2d037");

mixer.enqueue(0, Mixer.Samples, maud);
for (let i = 0; i < buffers + 2; i += 1) {
	digest.write(mixer.mix(1));
	digest.write(mixer.mix(imaSamplesPerChunk - 1));
}
verifyChecksum("5e8738767e264280beb45ed6ceb2d037");

mixer.enqueue(0, Mixer.Samples, maud);
let remain = (buffers + 2) * imaSamplesPerChunk;
while (remain > imaSamplesPerChunk) {
	digest.write(mixer.mix(50));
	remain -= 50;
}
digest.write(mixer.mix(remain));
verifyChecksum("5e8738767e264280beb45ed6ceb2d037");

mixer.enqueue(0, Mixer.Silence, imaSamplesPerChunk * 2);
mixer.enqueue(0, Mixer.Samples, maud, 1, imaSamplesPerChunk * 2);		// first few chunks are silence
for (let i = 0; i < buffers + 2; i += 1)
	digest.write(mixer.mix(imaSamplesPerChunk));
verifyChecksum("5e8738767e264280beb45ed6ceb2d037");

mixer.enqueue(0, Mixer.Silence, imaSamplesPerChunk * 10);
mixer.enqueue(0, Mixer.Samples, maud, 1, imaSamplesPerChunk * 10);		// first few chunks are silence (but not first 10)
for (let i = 0; i < buffers + 2; i += 1)
	digest.write(mixer.mix(imaSamplesPerChunk));
verifyChecksum("de22d0ececf71284e6f17368cf695c86");

function verifyChecksum(expected) {
	const bytes = new Uint8Array(digest.close());
	let checksum = [];
	for (let i = 0, length = bytes.length; i < length; i++)
		checksum.push(bytes[i].toString(16).padStart(2, "0"));
	assert.sameValue(checksum.join(""), expected);
	
	digest.reset();
	$262.gc();
}
