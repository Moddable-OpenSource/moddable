/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";

const render = new Poco(screen);

function testBegin(...args) {
	render.begin.apply(render, args);
	render.end();

	assert.sameValue("d41d8cd98f00b204e9800998ecf8427e", screen.checksum, "image mismatch");
}

testBegin();
testBegin({x: 0, y: 0, width: render.width, height: render.height});
testBegin(0, 0);
testBegin(0, 0, render.width);
testBegin(0, 0, render.width, render.height);
testBegin(String(0), String(0), String(render.width), String(render.height));

assert.throws(TypeError, () => render.begin(0, 0, Symbol()), "fails on symbol");
assert.throws(TypeError, () => render.begin(0, 0, render.width, Symbol()), "fails on symbol");

assert.throws(SyntaxError, () => render.begin.call(new $TESTMC.HostObject), "begin with non-poco this");
