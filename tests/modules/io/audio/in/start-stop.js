/*---
description: 
flags: [module]
---*/

import AudioIn from "embedded:io/audio/in";

let input = new AudioIn({});

input.stop(); // stop before start
input.start();
input.start(); // start after start
input.stop();
input.stop(); // stop after stop

input.start();		// start again
input.stop();

input.start();

input.close();		// close while started
