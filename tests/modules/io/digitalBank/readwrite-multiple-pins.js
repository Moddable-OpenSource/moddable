/*---
description: 
flags: [onlyStrict]
---*/

const DigitalBank = device.io.DigitalBank;

// Note: pin numbers can be changed, but the tests below assume that 
// input pin numbers are 1 less than corresponding output pin numbers

const INPUT_PIN_1 = $TESTMC.config.digital[0];
const OUTPUT_PIN_1 = $TESTMC.config.digital[1];

const INPUT_PIN_2 = $TESTMC.config.digital[2];
const OUTPUT_PIN_2 = $TESTMC.config.digital[3];

const INPUT_PIN_3 = $TESTMC.config.digital[4];
const OUTPUT_PIN_3 = $TESTMC.config.digital[5];

const input = new DigitalBank({
   pins: (1 << INPUT_PIN_1) | (1 << INPUT_PIN_2 ) | (1 << INPUT_PIN_3),
   mode: DigitalBank.Input
});

const output = new DigitalBank({
   pins: (1 << OUTPUT_PIN_1) | (1 << OUTPUT_PIN_2 ) | (1 << OUTPUT_PIN_3),
   mode: DigitalBank.Output
});

const values = [
   // Verify that writing just one of the pins works
   (1 << OUTPUT_PIN_1),
   (1 << OUTPUT_PIN_2),
   (1 << OUTPUT_PIN_3),
   // Verify that writing two of the pins works
   (1 << OUTPUT_PIN_1) | (1 << OUTPUT_PIN_2),
   (1 << OUTPUT_PIN_1) | (1 << OUTPUT_PIN_3),
   (1 << OUTPUT_PIN_2) | (1 << OUTPUT_PIN_3),
   // Verify that writing all three pins works
   (1 << OUTPUT_PIN_1) | (1 << OUTPUT_PIN_2) | (1 << OUTPUT_PIN_3),
   0
]

for (let value of values) {
   output.write(value);
   let read = input.read() << 1;
   assert.sameValue(read, value, `Read value should be 0b${value.toString(2)}, but is 0b${read.toString(2)}`);
}
