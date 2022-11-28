/*---
description: 
flags: [module]
---*/

import {Mixer} from "pins/audioout"

let mixer;

mixer = new Mixer({streams: 1, sampleRate: 12000, numChannels: 1});
mixer.callback = function() {};

mixer.enqueue(0, Mixer.Silence, 1000);
mixer.enqueue(0, Mixer.Silence, 1000);
mixer.enqueue(0, Mixer.Silence, 1000);
mixer.enqueue(0, Mixer.Silence, 1000);
assert.throws(Error, () => mixer.enqueue(0, Mixer.Silence, 1000));

mixer.enqueue(0, Mixer.Flush);
mixer.enqueue(0, Mixer.Silence, 1000);
mixer.enqueue(0, Mixer.Silence, 1000);
mixer.enqueue(0, Mixer.Silence, 1000);
mixer.enqueue(0, Mixer.Silence, 1000);
assert.throws(Error, () => mixer.enqueue(0, Mixer.Silence, 1000));

mixer.enqueue(0, Mixer.Flush);
assert.throws(SyntaxError, () => mixer.enqueue());
assert.throws(RangeError, () => mixer.enqueue(-1, Mixer.Flush));
assert.throws(RangeError, () => mixer.enqueue(3, Mixer.Flush));

assert.throws(Error, () => mixer.enqueue(0, -1));
assert.throws(Error, () => mixer.enqueue(0, 100));

assert.throws(SyntaxError, () => mixer.enqueue(0, Mixer.Volume));
mixer.enqueue(0, Mixer.Volume, 5);
mixer.enqueue(0, Mixer.Volume, "55");
assert.throws(TypeError, () => mixer.enqueue(0, Mixer.Volume, Symbol()));

mixer.enqueue(0, Mixer.Flush);
assert.throws(SyntaxError, () => mixer.enqueue(0, Mixer.Callback));
assert.throws(TypeError, () => mixer.enqueue(0, Mixer.Callback, Symbol()));
mixer.enqueue(0, Mixer.Callback, 0);
mixer.enqueue(0, Mixer.Callback, -1);
mixer.enqueue(0, Mixer.Callback, 65537);
mixer.enqueue(0, Mixer.Callback, "5");
assert.throws(Error, () => mixer.enqueue(0, Mixer.Callback, "55"));

mixer.enqueue(0, Mixer.Flush);
assert.throws(SyntaxError, () => mixer.enqueue(0, Mixer.Tone));
assert.throws(TypeError, () => mixer.enqueue(0, Mixer.Tone, Symbol()));
assert.throws(TypeError, () => mixer.enqueue(0, Mixer.Tone, 880, Symbol()));
assert.throws(TypeError, () => mixer.enqueue(0, Mixer.Tone, 880, 1000, Symbol()));
mixer.enqueue(0, Mixer.Tone, 220);
mixer.enqueue(0, Mixer.Tone, 220, 1000);
mixer.enqueue(0, Mixer.Tone, 220, 1000, 255);

mixer.enqueue(0, Mixer.Flush);
assert.throws(SyntaxError, () => mixer.enqueue(0, Mixer.Silence));
assert.throws(TypeError, () => mixer.enqueue(0, Mixer.Silence, Symbol()));
assert.throws(Error, () => mixer.enqueue(0, Mixer.Silence, 0));
mixer.enqueue(0, Mixer.Silence, 100);
mixer.enqueue(0, Mixer.Silence, 200);
