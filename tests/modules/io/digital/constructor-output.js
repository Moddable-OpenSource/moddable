/*---
description: 
flags: [onlyStrict]
---*/

const Digital = device.io.Digital;

let output = new Digital({
   pin: $TESTMC.config.digital[1],
   mode: Digital.Output,
});

// Same pin can't be used by two instances of the `Digital` class configured as digital output
assert.throws(Error, () => {
	let duplicate = new Digital({
	   pin: $TESTMC.config.digital[1],
	   mode: Digital.Output,
	});
}, "Digital constructor should throw an error when pin specified is already in use");

// Verify that pins can be reused after calling `close`
output.close();
output = new Digital({
   pin: $TESTMC.config.digital[1],
   mode: Digital.Output
});
output.close();
