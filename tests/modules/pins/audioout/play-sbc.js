/*---
description: 
flags: [module,async]
---*/

import AudioOut from "pins/audioout"
import Resource from "Resource";
import {MAUD, SampleFormat} from "./maud.js"
import Timer from "timer"

// ffmpeg -i $MODDABLE/examples/assets/sounds/bflatmajor.wav -ac 1 -ar 16000 -b:a 32k ~/bflatmajor-sbc.sbc
const sbc = new Resource("bflatmajor-sbc.sbc");

const bytesPerChunk = 32;
const samplesPerChunk = 128;

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

const out = new AudioOut({streams: 1, sampleRate, numChannels: 1});

out.enqueue(0, AudioOut.Samples, samples);
out.start();

Timer.set(() => $DONE(), 2000)
