/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";

const render = new Poco(screen);

render.begin();

assert.throws(SyntaxError, () => render.clip(0), "clip requires 0 or 4 arguments");
assert.throws(SyntaxError, () => render.clip(0, 0), "clip requires 0 or 4 arguments");
assert.throws(SyntaxError, () => render.clip(0, 0, 0), "clip requires 0 or 4 arguments");

let count = 0;
const value = {
	[Symbol.toPrimitive]() {
		count +=  1;
		return 0;
	}
};
render.clip(value, value, value, value);
assert.sameValue(4, count, "clip coerces to number");
render.clip();

assert.sameValue(true, render.clip(0, 0, 10, 10), "clip push returns true if non-empty");
assert.sameValue(true, render.clip("2", "2", "20", "20"), "clip push returns true if non-empty");
assert.sameValue(undefined, render.clip(10, 10, 10, 10), "clip push returns undefined if empty");
assert.sameValue("undefined", typeof render.clip(), "clip pop has no return value");
assert.sameValue("undefined", typeof render.clip(), "clip pop has no return value");
assert.sameValue("undefined", typeof render.clip(), "clip pop has no return value");

for (let i = 0; i < 1000; i++)
	render.clip(0, 0, 10, 10);
assert.throws(Error, () => render.end(), "clip stack overflow causes end to throw");

assert.throws(SyntaxError, () => render.clip.call(new $TESTMC.HostObject), "clip with non-poco this");
