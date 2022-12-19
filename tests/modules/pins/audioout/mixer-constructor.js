/*---
description: 
flags: [module]
---*/

import {Mixer} from "pins/audioout"

let mixer;

mixer = new Mixer({streams: 1, sampleRate: 11025, numChannels: 1}); mixer.close();

assert.throws(SyntaxError, () => new Mixer, "mixer constructor requires 1 argument");
assert.throws(SyntaxError, () => new Mixer(Symbol()), "mixer constructor rejects Symbol");
assert.throws(TypeError, () => Mixer("testmixer"), "mixer constructor called as function");

assert.throws(RangeError, () => new Mixer({sampleRate: 0}), "invalid sample rate");
assert.throws(RangeError, () => new Mixer({sampleRate: -1}), "invalid sample rate");
assert.throws(RangeError, () => new Mixer({sampleRate: 65536}), "invalid sample rate");
assert.throws(TypeError, () => new Mixer({sampleRate: Symbol()}), "invalid sample rate");
mixer = new Mixer({sampleRate: "8000"}); mixer.close();
mixer = new Mixer({sampleRate: 48000}); mixer.close();

assert.throws(RangeError, () => new Mixer({numChannels: 0}), "invalid channels");
assert.throws(RangeError, () => new Mixer({numChannels: 3}), "invalid channels");
assert.throws(RangeError, () => new Mixer({numChannels: -1}), "invalid channels");
assert.throws(TypeError, () => new Mixer({numChannels: Symbol()}), "invalid channels");
mixer = new Mixer({numChannels: 1}); mixer.close();
mixer = new Mixer({numChannels: "2"}); mixer.close();

assert.throws(RangeError, () => new Mixer({bitsPerSample: 0}), "invalid bitsPerSample");
assert.throws(RangeError, () => new Mixer({bitsPerSample: 17}), "invalid bitsPerSample");
assert.throws(RangeError, () => new Mixer({bitsPerSample: 9}), "invalid bitsPerSample");
assert.throws(RangeError, () => new Mixer({bitsPerSample: -1}), "invalid bitsPerSample");
assert.throws(TypeError, () => new Mixer({bitsPerSample: Symbol()}), "invalid bitsPerSample");
mixer = new Mixer({bitsPerSample: 8}); mixer.close();
mixer = new Mixer({bitsPerSample: "16"}); mixer.close();

assert.throws(RangeError, () => new Mixer({streams: 0}), "invalid streams");
assert.throws(RangeError, () => new Mixer({streams: 3}), "invalid streams");
assert.throws(RangeError, () => new Mixer({streams: -1}), "invalid streams");
assert.throws(TypeError, () => new Mixer({streams: Symbol()}), "invalid streams");
mixer = new Mixer({streams: 1}); mixer.close();
mixer = new Mixer({streams: "2"}); mixer.close();

assert.sameValue(Mixer.Samples, 1);
assert.sameValue(Mixer.Flush, 2);
assert.sameValue(Mixer.Callback, 3);
assert.sameValue(Mixer.Volume, 4);
assert.sameValue(Mixer.RawSamples, 5);
assert.sameValue(Mixer.Tone, 6);
assert.sameValue(Mixer.Silence, 7);
