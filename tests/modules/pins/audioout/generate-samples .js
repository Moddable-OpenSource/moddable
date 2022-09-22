/*---
description: 
flags: [module]
---*/

import {Mixer} from "pins/audioout"
import {MAUD, SampleFormat} from "./maud.js"

const bufferSamples = 137;
const sampleRate = 12000;
const samples = new Int16Array(new SharedArrayBuffer(MAUD.byteLength + (bufferSamples * 2)), MAUD.byteLength);
for (let i = 0; i < samples.length; i++)
	samples[i] = 32767 - Math.round(65535 * Math.random());
const maud = new MAUD(samples.buffer);

maud.tag = "ma";
maud.version = 1;
maud.bitsPerSample = 16;
maud.sampleRate = sampleRate;
maud.numChannels = 1;
maud.bufferSamples = bufferSamples;
maud.sampleFormat = SampleFormat.Uncompressed;

const mixer = new Mixer({streams: 1, sampleRate: sampleRate, numChannels: 1});

// confirm initial MAUD header is valid
mixer.enqueue(0, Mixer.Samples, samples.buffer);
mixer.enqueue(0, Mixer.Flush);

// tag
maud.tag = "  ";
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))
maud.tag = "MA";
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))
maud.tag = "mb";
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))
maud.tag = "ma";

// version
maud.version = 3;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))
maud.version = 0;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))
maud.version = 1;

// bitsPerSample
maud.bitsPerSample = 0;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))
maud.bitsPerSample = 8;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))
maud.bitsPerSample = 16;

// sampleRate
maud.sampleRate = 1;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))

maud.sampleRate = sampleRate + 1;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))
maud.sampleRate = sampleRate;

// numChannels
maud.numChannels = 3;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))

maud.numChannels = -1;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))

maud.numChannels = 0;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))
maud.numChannels = 1;

// sampleFormat
maud.sampleFormat = -1;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))

maud.sampleFormat = SampleFormat.Tone + 1;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))
maud.sampleFormat = SampleFormat.Uncompressed;

// bufferSamples
maud.bufferSamples = -1;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))

maud.bufferSamples = 0;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))

maud.bufferSamples = bufferSamples + 1;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))

maud.bufferSamples = 32768;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))

maud.bufferSamples = 65536;
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, samples.buffer))

// mixing (adatped from generate-rawsamples)

assert.throws(TypeError, () => mixer.enqueue(0, Mixer.Samples, new Int16Array(16), 1, 0, 16));
assert.throws(TypeError, () => mixer.enqueue(0, Mixer.Samples, new Int16Array(new SharedArrayBuffer(16 * 2)), 1, 0, 16));
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, new ArrayBuffer(16), 1, 0, 16));
assert.throws(Error, () => mixer.enqueue(0, Mixer.Samples, new Int8Array(16), 1, 0, 16));

mixer.enqueue(0, Mixer.Samples, samples.buffer, 1, 0, samples.length);
verifyOutput(samples);

mixer.enqueue(0, Mixer.Samples, samples.buffer, 1, 0, samples.length);
mixer.enqueue(0, Mixer.Samples, samples.buffer, 1, 0, samples.length);
verifyOutput(samples);
verifyOutput(samples);

mixer.enqueue(0, Mixer.Samples, samples.buffer, 2, 0, 50);
mixer.enqueue(0, Mixer.Samples, samples.buffer, 1, samples.length - 50, 50);
verifyOutput(samples.slice(0, 50));
verifyOutput(samples.slice(0, 50));
verifyOutput(samples.slice(samples.length - 50));

mixer.enqueue(0, Mixer.Samples, samples.buffer, Infinity, 50, 39);
for (let i = 0, expected = samples.slice(50, 50 + 39); i < 100; i++)
	verifyOutput(expected);
mixer.enqueue(0, Mixer.Flush);

mixer.enqueue(0, Mixer.Volume, 128);
mixer.enqueue(0, Mixer.Samples, samples.buffer, 1, 0, samples.length);
mixer.enqueue(0, Mixer.Volume, 64);
mixer.enqueue(0, Mixer.Samples, samples.buffer, 1, 0, samples.length);
let expected = samples.slice();
for (let i = 0; i < expected.length; i++)
	expected[i] = expected[i] >> 1;
verifyOutput(expected);
expected = samples.slice();
for (let i = 0; i < expected.length; i++)
	expected[i] = expected[i] >> 2;
verifyOutput(expected);

mixer.enqueue(0, Mixer.Volume, 4 * 256);		// overdrive
mixer.enqueue(0, Mixer.Samples, samples.buffer, 1, 0, samples.length);
for (let i = 0; i < expected.length; i++) {
	let v = expected[i] << 2;
	expected[i] = Math.min(Math.max(-32768, v), 32767);
}

function verifyOutput(expected) {
	const result = mixer.mix(expected.length);
	const actual = new Int16Array(result);
	assert.sameValue(actual.length, expected.length);
	for (let i = 0, count = expected.length; i < count; i++)
		assert.sameValue(actual[i], expected[i]);
}

