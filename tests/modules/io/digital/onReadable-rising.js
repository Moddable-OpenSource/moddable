/*---
description: 
flags: [async]
---*/

const Digital = device.io.Digital;

const output = new Digital({
   pin: $TESTMC.config.digital[1],
   mode: Digital.Output
});
output.write(0);

const input = new Digital({
   pin: $TESTMC.config.digital[0],
   mode: Digital.Input,
   edge: Digital.Rising,
   onReadable() {
      $DONE();
   }
});

output.write(1);
$TESTMC.timeout(100, "`onReadable` should have been triggered");
