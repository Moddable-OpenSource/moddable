/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/567
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function test() {
	let stuff = '1234';
	for (;;) {
	  stuff += stuff;
	}
}

test();
