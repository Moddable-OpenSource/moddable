/*---
description: 
flags: [onlyStrict]
---*/

const Digital = device.io.Digital;

// Digital IO instance configured as an output does NOT implement `read`
const output = new Digital({
   pin: $TESTMC.config.digital[1],
   mode: Digital.Output
});
assert.throws(Error, () => {
   output.read();
}, "`read` should not be implemented for digital output");
output.close();

// Digital IO instance configured as an input does implement `read`
let input = new Digital({
   pin: $TESTMC.config.digital[0],
   mode: Digital.Input
});
input.read();

// `read` should not be callable after calling `close`
input.close();
assert.throws(SyntaxError, () => {
   input.read();
}, `Cannot read after pin is closed`);
