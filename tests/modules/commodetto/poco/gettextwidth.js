/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";
import parseBMF from "commodetto/parseBMF";
import Resource from "Resource";

const render = new Poco(screen);
const font = parseBMF(new Resource("OpenSans-Regular-16.bf4"));

assert.throws(SyntaxError, () => render.getTextWidth(), "getTextWidth requires 2 arguments");
assert.throws(SyntaxError, () => render.getTextWidth("a"), "getTextWidth requires 2 arguments");

const throwArgument = {
	[Symbol.toPrimitive]() {
		throw new Test262Error;
	}
};
assert.throws(Test262Error, () => render.getTextWidth(throwArgument, font), "coerces string");

assert.throws(SyntaxError, () => render.getTextWidth("12", {}), "invalid font");
assert.throws(SyntaxError, () => render.getTextWidth(new ArrayBuffer, {}), "invalid font");
assert.throws(SyntaxError, () => render.getTextWidth(Uint8Array.of(32), {}), "invalid font");

assert.sameValue(27, render.getTextWidth("123", font));
assert.sameValue(27, render.getTextWidth(123, font));
assert.sameValue(0, render.getTextWidth("", font));
assert.sameValue(27, render.getTextWidth("\u1234123\u1234", font));
assert.sameValue(56, render.getTextWidth(" 1 2 3 4 ", font));
