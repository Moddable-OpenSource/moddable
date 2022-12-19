/*---
description: 
flags: [async, module]
---*/

import Timer from "timer";
const Digital = device.io.Digital;

const output = new Digital({
   pin: $TESTMC.config.digital[1],
   mode: Digital.Output
});
output.write(1);

const input = new Digital({
   pin: $TESTMC.config.digital[0],
   mode: Digital.Input,
   edge: Digital.Rising,
   onReadable() {
      $DONE("`onReadable` should not have been triggered");
   }
});

output.write(0);
Timer.set(() => $DONE(), 100);
