/*---
description: 
flags: [onlyStrict]
---*/

const DigitalBank = device.io.DigitalBank;

const INPUT_PIN = $TESTMC.config.digital[0];
const OUTPUT_PIN = $TESTMC.config.digital[1];

let input = new DigitalBank({
	pins: 1 << INPUT_PIN,
	mode: DigitalBank.Input
});

let output = new DigitalBank({
	pins: 1 << OUTPUT_PIN,
	mode: DigitalBank.Output
});

let value = (1 << OUTPUT_PIN);
output.write(value);
let read = input.read() << 1;
assert.sameValue(read, value, `Read value should be 0b${value.toString(2)}, but is 0b${read.toString(2)}`);

value = 0;
output.write(value);
read = input.read() << 1;
assert.sameValue(read, value, `Read value should be 0b${value.toString(2)}, but is 0b${read.toString(2)}`);

const values = [
	(1 << (OUTPUT_PIN-1)),
	(1 << (OUTPUT_PIN+1)),
	0.5,
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
	 output.write(value);
	 assert.sameValue(input.read(), 0, `Read value should be 0 after writing value ${value}`);
}
