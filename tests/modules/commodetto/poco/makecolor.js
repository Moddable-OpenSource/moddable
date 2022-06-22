/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";

const render = new Poco(screen);

assert.throws(SyntaxError, () => render.makeColor(), "makeColor requires 3 arguments");
assert.throws(SyntaxError, () => render.makeColor(0), "makeColor requires 3 arguments");
assert.throws(SyntaxError, () => render.makeColor(0, 0), "makeColor requires 3 arguments");

assert.sameValue("number", typeof render.makeColor(0, 0, 0), "makeColor returns number");

let count = 0;
const value = {
	[Symbol.toPrimitive]() {
		count +=  1;
		return 0;
	}
};
render.makeColor(value, value, value);
assert.sameValue(3, count, "makeColor coerces to number");

// implementation does not use "this", so this test is not currently used
// assert.throws(SyntaxError, () => render.makeColor.call(new $TESTMC.HostObject, 0, 0, 0), "makeColor with non-poco this");
