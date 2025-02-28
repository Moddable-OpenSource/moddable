/*---
description: 
flags: [module]
---*/

import AudioOut from "embedded:io/audioout";

let out = new AudioOut({});

out.stop(); // stop before start
out.start()
assert.throws(Error, () => out.start(), "start while started");
out.stop();
out.stop(); // stop after stop

out.start();		// start again
out.stop();

out.start();

out.close();		// close while started
