/*---
description: 
flags: [module]
---*/

import AudioOut from "embedded:io/audio/out";

let out = new AudioOut({});

out.stop(); // stop before start
out.start();
out.start(); // start after start
out.stop();
out.stop(); // stop after stop

out.start();		// start again
out.stop();

out.start();

out.close();		// close while started
