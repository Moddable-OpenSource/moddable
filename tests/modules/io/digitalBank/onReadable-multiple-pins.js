/*---
description: 
flags: [async]
---*/

const DigitalBank = device.io.DigitalBank;

// Note: pin numbers can be changed, but the tests below assume that 
// input pin numbers are 1 less than corresponding output pin numbers

const INPUT_PIN_1 = $TESTMC.config.digital[0];
const OUTPUT_PIN_1 = $TESTMC.config.digital[1];

const INPUT_PIN_2 = $TESTMC.config.digital[2];
const OUTPUT_PIN_2 = $TESTMC.config.digital[3];

const output = new DigitalBank({
   pins: (1 << OUTPUT_PIN_1) | (1 << OUTPUT_PIN_2),
   mode: DigitalBank.Output
});
output.write(0);

const input = new DigitalBank({
   pins: (1 << INPUT_PIN_1) | (1 << INPUT_PIN_2),
   rises: (1 << INPUT_PIN_1) | (1 << INPUT_PIN_2),
   mode: DigitalBank.Input,
   onReadable(triggers) {
      if (triggers == (1 << INPUT_PIN_2))
         $DONE();
      else
         $DONE(`Pin ${INPUT_PIN_2} should have been triggered`);
   }
});

output.write(1 << OUTPUT_PIN_2);
$TESTMC.timeout(100, "`onReadable` should have been triggered");     
