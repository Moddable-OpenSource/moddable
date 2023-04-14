/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";

const render = new Poco(screen);

function testBegin(...args) {
	render.begin.apply(render, args);
	render.end();

	assert.sameValue("ac3553226808462a69794df4390b20d5", screen.checksum, "image mismatch");
}

testBegin();
testBegin({x: 0, y: 0, w: render.width, h: render.height});
testBegin(0, 0);
testBegin(0, 0, render.width);
testBegin(0, 0, render.width, render.height);
testBegin(String(0), String(0), String(render.width), String(render.height));
testBegin({x: String(0), y: String(0), w: String(render.width), h: String(render.height)});

assert.throws(TypeError, () => render.begin(0, 0, Symbol()), "fails on symbol");
assert.throws(TypeError, () => render.begin(0, 0, render.width, Symbol()), "fails on symbol");
assert.throws(TypeError, () => render.begin({x: 0, y: 0, w: render.width, h: Symbol()}), "fails on symbol");

assert.throws(SyntaxError, () => render.begin.call(new $TESTMC.HostObject), "begin with non-poco this");
