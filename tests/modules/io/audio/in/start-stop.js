/*---
description: 
flags: [module]
---*/

import AudioIn from "embedded:io/audioin";

let input = new AudioIn({});

input.stop(); // stop before start
input.start()
assert.throws(Error, () => input.start(), "start while started");
input.stop();
input.stop(); // stop after stop

input.start();		// start again
input.stop();

input.start();

input.close();		// close while started
