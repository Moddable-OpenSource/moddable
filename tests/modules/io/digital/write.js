/*---
description: 
flags: [onlyStrict]
---*/

const Digital = device.io.Digital;

// Digital IO instance configured as an input does NOT implement `write`
const input = new Digital({
   pin: $TESTMC.config.digital[0],
   mode: Digital.Input
});
assert.throws(Error, () => {
   input.write(0);
}, "`write` should not be implemented for digital input");
input.close();


// Digital IO instance configured as an output does implement `write`
const output = new Digital({
   pin: $TESTMC.config.digital[1],
   mode: Digital.Output
});
output.write(1);

// `write` should only accept input that can be coerced to numbers
let invalidInputs = [
   Symbol(),
   // BigInt("1")
]
for (let invalidInput of invalidInputs) {
   assert.throws(TypeError, () => {
      output.write(invalidInput);
   }, `Digital write should throw an error when input that can't be coerced to a number is given`);
}

// `write` should not be callable after calling `close`
output.close();
assert.throws(SyntaxError, () => {
   output.write(1);
}, `Cannot write after pin is closed`);
