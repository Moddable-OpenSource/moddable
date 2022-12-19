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
