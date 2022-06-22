/*---
description: 
flags: [module]
---*/

const Digital = device.io.Digital;

let input = new Digital({
   pin: $TESTMC.config.digital[0],
   mode: Digital.Input
});

let output = new Digital({
   pin: $TESTMC.config.digital[1],
   mode: Digital.Output
});

// Check that `read` works
output.write(1);
assert.sameValue(input.read(), 1, `Read value should be 1`);

// Check that `read` still works after closing and reopening connection to input pin
input.close();
input = new Digital({
   pin: $TESTMC.config.digital[0],
   mode: Digital.Input
});
assert.sameValue(input.read(), 1, `Read value should be 1`);

// Check that `write` still works after closing and reopening connection to input pin
output.close();
output = new Digital({
   pin: $TESTMC.config.digital[1],
   mode: Digital.Output
});
output.write(1);
assert.sameValue(input.read(), 1, `Read value should be 1`);

// Test that other values can be written/read
const values = [
   0,
   0.5,
   500,
   -0,
   -1,
   false,
   true,
   null,
   {},
   undefined,
   Promise,
   "0",
   "1",
   "error"
];
for (let value of values) {
    const expected = Number(value) ? 1 : 0;
    output.write(value);
    assert.sameValue(input.read(), expected, `Read value should be ${expected} after writing value ${value}`);
}
