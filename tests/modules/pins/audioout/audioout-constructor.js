/*---
description: 
flags: [module]
---*/

import AudioOut from "pins/audioout"

let out;

out = new AudioOut({streams: 1, sampleRate: 11025, numChannels: 1}); out.close();

assert.throws(SyntaxError, () => new AudioOut, "audioout constructor requires 1 argument");
assert.throws(SyntaxError, () => new AudioOut(Symbol()), "audioout constructor rejects Symbol");
assert.throws(TypeError, () => AudioOut("testaudioout"), "audioout constructor called as function");

assert.throws(RangeError, () => new AudioOut({sampleRate: 0}), "invalid sample rate");
assert.throws(RangeError, () => new AudioOut({sampleRate: -1}), "invalid sample rate");
assert.throws(RangeError, () => new AudioOut({sampleRate: 65536}), "invalid sample rate");
assert.throws(TypeError, () => new AudioOut({sampleRate: Symbol()}), "invalid sample rate");
out = new AudioOut({sampleRate: "8000"}); out.close();
out = new AudioOut({sampleRate: 48000}); out.close();

assert.throws(RangeError, () => new AudioOut({numChannels: 0}), "invalid channels");
assert.throws(RangeError, () => new AudioOut({numChannels: 3}), "invalid channels");
assert.throws(RangeError, () => new AudioOut({numChannels: -1}), "invalid channels");
assert.throws(TypeError, () => new AudioOut({numChannels: Symbol()}), "invalid channels");
out = new AudioOut({numChannels: 1}); out.close();
out = new AudioOut({numChannels: "2"}); out.close();

assert.throws(RangeError, () => new AudioOut({bitsPerSample: 0}), "invalid bitsPerSample");
assert.throws(RangeError, () => new AudioOut({bitsPerSample: 17}), "invalid bitsPerSample");
assert.throws(RangeError, () => new AudioOut({bitsPerSample: 9}), "invalid bitsPerSample");
assert.throws(RangeError, () => new AudioOut({bitsPerSample: -1}), "invalid bitsPerSample");
assert.throws(TypeError, () => new AudioOut({bitsPerSample: Symbol()}), "invalid bitsPerSample");
out = new AudioOut({bitsPerSample: 8}); out.close();
out = new AudioOut({bitsPerSample: "16"}); out.close();

assert.throws(RangeError, () => new AudioOut({streams: 0}), "invalid streams");
assert.throws(RangeError, () => new AudioOut({streams: 3}), "invalid streams");
assert.throws(RangeError, () => new AudioOut({streams: -1}), "invalid streams");
assert.throws(TypeError, () => new AudioOut({streams: Symbol()}), "invalid streams");
out = new AudioOut({streams: 1}); out.close();
out = new AudioOut({streams: "2"}); out.close();

assert.sameValue(AudioOut.Samples, 1);
assert.sameValue(AudioOut.Flush, 2);
assert.sameValue(AudioOut.Callback, 3);
assert.sameValue(AudioOut.Volume, 4);
assert.sameValue(AudioOut.RawSamples, 5);
assert.sameValue(AudioOut.Tone, 6);
assert.sameValue(AudioOut.Silence, 7);
