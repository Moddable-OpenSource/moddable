/*---
description: 
flags: [async]
---*/

const Digital = device.io.Digital;

let count = 0, value = 1;

const output = new Digital({
   pin: $TESTMC.config.digital[1],
   mode: Digital.Output
});
output.write(0);

const input = new Digital({
   pin: $TESTMC.config.digital[0],
   mode: Digital.Input,
   edge: Digital.Rising + Digital.Falling,
   onReadable() {
      count++;
      if (count < 20) {
         value = !value;
         output.write(value);
      } else {
         $DONE();
      }
   }
});

output.write(value);
$TESTMC.timeout(100, "`onReadable` should have been triggered");
