/*---
description: 
flags: [module,async]
---*/

import AudioOut from "pins/audioout"
import Resource from "Resource";
import {MAUD, SampleFormat} from "./maud.js"
import Timer from "timer";

const ima = new Resource("bflatmajor-ima-16000.maud");
const sbcResource = new Resource("bflatmajor-sbc.sbc");

const bytesPerChunk = 32;
const samplesPerChunk = 128;

const sampleRate = 16000;
const samples = new Uint8Array(new SharedArrayBuffer(MAUD.byteLength + sbcResource.byteLength));
samples.set(new Uint8Array(sbcResource), MAUD.byteLength)
const sbc = new MAUD(samples.buffer);

sbc.tag = "ma";
sbc.version = 1;
sbc.bitsPerSample = 16;
sbc.sampleRate = sampleRate;
sbc.numChannels = 1;
sbc.bufferSamples = Math.idiv(sbcResource.byteLength, bytesPerChunk) * samplesPerChunk;
sbc.sampleFormat = SampleFormat.SBC;

const out = new AudioOut({streams: 1, sampleRate, numChannels: 1});

out.enqueue(0, AudioOut.Samples, ima, 2);
out.enqueue(0, AudioOut.Samples, sbc.buffer, 2);
out.enqueue(0, AudioOut.Samples, ima, 1);
out.enqueue(0, AudioOut.Tone, 466.16, sampleRate >> 1);
out.start();

Timer.set(() => $DONE(), 5000)
