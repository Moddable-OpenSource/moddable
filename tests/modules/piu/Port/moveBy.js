/*---
description: 
flags: [onlyStrict]
---*/

const unconstrainedContent = new Port(null, { 
    top: 0, left: 0, height: 100, width: 100
});

const constrainedContent = new Port(null, { 
    top: 0, left: 0, bottom: 140, right: 220
});

new Application(null, {
    contents: [ unconstrainedContent, constrainedContent ]
});

assert.sameValue(unconstrainedContent.x, 0, "unconstrainedContent should have x coordinate 0");
assert.sameValue(unconstrainedContent.y, 0, "unconstrainedContent should have y coordinate 0");
assert.sameValue(constrainedContent.x, 0, "constrainedContent should have x coordinate 0");
assert.sameValue(constrainedContent.y, 0, "constrainedContent should have y coordinate 0");

unconstrainedContent.moveBy(100,100);
constrainedContent.moveBy(100,100);	// Does nothing

assert.sameValue(unconstrainedContent.x, 100, "unconstrainedContent should have x coordinate 100");
assert.sameValue(unconstrainedContent.y, 100, "unconstrainedContent should have y coordinate 100");
assert.sameValue(constrainedContent.x, 0, "constrainedContent should have x coordinate 0");
assert.sameValue(constrainedContent.y, 0, "constrainedContent should have y coordinate 0");