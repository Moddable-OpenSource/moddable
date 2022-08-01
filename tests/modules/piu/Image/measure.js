/*---
description: 
flags: [onlyStrict]
---*/

const content = new Image(null, {
    top: 0, left: 0, height: 100, width: 100, path: "screen2.cs"
});

const content2 = new Image(null, {
    top: 0, bottom: 0, left: 0, right: 0, path: "screen2.cs"
});

new Application(null, {
    contents: [ content, content2 ]
});

assert.sameValue(content.measure().width, 100, "`content`'s measured width should be 100");
assert.sameValue(content.measure().height, 100, "`content`'s measured height should be 100");
assert.sameValue(content.width, 100, "`content`'s fitted width should be 100");
assert.sameValue(content.height, 100, "`content`'s fitted height should be 100");

// Image's measured width matches dimensions of image file
assert.sameValue(content2.measure().width, 320, "`content2`'s measured width should be 320");
assert.sameValue(content2.measure().height, 240, "`content2`'s measured height should be 240");
assert.sameValue(content2.width, 240, "`content2`'s fitted width should be 0");
assert.sameValue(content2.height, 320, "`content2`'s fitted height should be 0");