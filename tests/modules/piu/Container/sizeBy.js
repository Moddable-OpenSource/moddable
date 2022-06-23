/*---
description: 
flags: [onlyStrict]
---*/

const unconstrainedContent = new Container(null, { 
    top: 0, left: 0, height: 100, width: 100
});

const constrainedContent = new Container(null, { 
    top: 100, left: 100, bottom: 120, right: 40
});

new Application(null, {
    contents: [ unconstrainedContent, constrainedContent ]
});

assert.sameValue(unconstrainedContent.width, 100, "unconstrainedContent should have width 100");
assert.sameValue(unconstrainedContent.height, 100, "unconstrainedContent should have height 100");
assert.sameValue(constrainedContent.width, 100, "constrainedContent should have width 100");
assert.sameValue(constrainedContent.height, 100, "constrainedContent should have height 100");

unconstrainedContent.sizeBy(100,100);
constrainedContent.sizeBy(100,100);	// Does nothing

assert.sameValue(unconstrainedContent.width, 200, "unconstrainedContent should have width 200");
assert.sameValue(unconstrainedContent.height, 200, "unconstrainedContent should have height 200");
assert.sameValue(constrainedContent.width, 100, "constrainedContent should have width 100");
assert.sameValue(constrainedContent.height, 100, "constrainedContent should have height 100");