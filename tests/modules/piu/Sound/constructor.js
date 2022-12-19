/*---
description: 
flags: [module]
---*/

import Sound from "piu/Sound"

new Sound([{frequency: 440, samples: 4000}, {frequency: 262, samples: 4000}]);
new Sound({path: "bflatmajor-ima-12000.wav"});
new Sound({path: "bflatmajor-ima-16000.wav"});
new Sound({path: "bflatmajor-ima-16000.wav", offset: 1000, size: 1000});

assert.throws(TypeError, () => new Sound());
assert.throws(URIError, () => new Sound({}));
assert.throws(URIError, () => new Sound({path: "bflatmajor-ima-12000.ima"}));
assert.throws(URIError, () => new Sound({path: "missing-sound.wav"}));
