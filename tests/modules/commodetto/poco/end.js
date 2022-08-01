/*---
description: 
flags: [module]
---*/

import Poco from "commodetto/Poco";

const render = new Poco(screen);

assert.throws(Error, () => render.end(), "unpaired end");

render.begin();
for (let i = 0; i < 1000; i++)
	render.fillRectangle(0, 0, 0, 10, 10);
assert.throws(Error, () => render.end(), "end with display list overflow");

render.begin();
for (let i = 0; i < 1000; i++)
	render.clip(0, 0, 10, 10);
assert.throws(Error, () => render.end(), "end with clip stack overflow");

render.begin();
for (let i = 0; i < 1000; i++)
	render.origin(0, 0, 10, 10);
assert.throws(Error, () => render.end(), "end with origin stack overflow");

render.begin();
assert.throws(SyntaxError, () => render.end.call(new $TESTMC.HostObject), "end with non-poco this");
render.end();
assert.throws(Error, () => render.end(), "duplicate end");
