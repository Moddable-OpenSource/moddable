/*---
description: 
flags: [async]
---*/

const Digital = device.io.Digital;

const output = new Digital({
   pin: $TESTMC.config.digital[1],
   mode: Digital.Output
});
output.write(1);

const input = new Digital({
   pin: $TESTMC.config.digital[0],
   mode: Digital.Input,
   edge: Digital.Falling,
   onReadable() {
      $DONE();
   }
});

output.write(0);
$TESTMC.timeout(100, "`onReadable` should have been triggered");
