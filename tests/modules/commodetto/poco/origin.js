/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";

const render = new Poco(screen);

render.begin();

assert.throws(SyntaxError, () => render.origin(0), "origin requires 0 or 2 arguments");

let count = 0;
const value = {
	[Symbol.toPrimitive]() {
		count += 1;
		return 0;
	}
};
render.origin(value, value);
assert.sameValue(2, count, "origin coerces to number");
render.origin();

assert.sameValue(undefined, render.origin(0, 0), "origin push returns undefined");
assert.sameValue(undefined, render.origin(), "origin pop returns undefined");

for (let i = 0; i < 1000; i++)
	render.origin(1, 1);
assert.throws(Error, () => render.end(), "origin stack overflow causes end to throw");

assert.throws(SyntaxError, () => render.origin.call(new $TESTMC.HostObject), "origin with non-poco this");
