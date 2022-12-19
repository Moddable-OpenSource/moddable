/*---
description: 
flags: [module]
---*/

import AudioOut from "pins/audioout"
import {Mixer} from "pins/audioout"

const invalidHost = new $TESTMC.HostObject;

const mixer = new Mixer({streams: 2, sampleRate: 12000, numChannels: 1});
mixer.enqueue(0, Mixer.Silence, 12000);
assert.throws(SyntaxError, () => mixer.close.call(invalidHost), "close with invalid this");
mixer.close();
mixer.close();
assert.throws(SyntaxError, () => mixer.close.call(invalidHost), "close with invalid this too");

assert.throws(SyntaxError, () => mixer.enqueue(0, Mixer.Silence, 12000), "enqueue after close"); 
assert.throws(SyntaxError, () => mixer.mix(100), "mix after close"); 
assert.throws(SyntaxError, () => mixer.length(0), "length after close"); 

assert.throws(SyntaxError, () => mixer.enqueue.call(invalidHost, 0, Mixer.Silence, 12000), "enqueue with invalid this"); 
assert.throws(SyntaxError, () => mixer.mix.call(invalidHost, 100), "mix with invalid this"); 
assert.throws(SyntaxError, () => mixer.length.call(invalidHost, 0), "length with invalid this"); 

const out = new AudioOut({streams: 1, sampleRate: 12000, numChannels: 1});
out.enqueue(0, AudioOut.Silence, 12000);
assert.throws(SyntaxError, () => out.close.call(invalidHost), "close with invalid this");
out.close();
out.close();
assert.throws(SyntaxError, () => out.close.call(invalidHost), "close with invalid this too");

assert.throws(SyntaxError, () => out.start(), "start after close"); 
assert.throws(SyntaxError, () => out.stop(), "stop after close"); 

assert.throws(SyntaxError, () => out.start.call(invalidHost), "start with invalid this"); 
assert.throws(SyntaxError, () => out.stop.call(invalidHost), "stop with invalid this"); 
