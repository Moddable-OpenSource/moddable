/*---
description: 
flags: [onlyStrict]
---*/

const content = new Text(null, {
    top: 0, left: 0, height: 100, width: 100
});

const content2 = new Text(null, {
    top: 0, bottom: 0, left: 0, right: 0
});

new Application(null, {
    contents: [ content, content2 ]
});

assert.sameValue(content.measure().width, 100, "`content`'s measured width should be 100");
assert.sameValue(content.measure().height, 100, "`content`'s measured height should be 100");
assert.sameValue(content.width, 100, "`content`'s fitted width should be 100");
assert.sameValue(content.height, 100, "`content`'s fitted height should be 100");

assert.sameValue(content2.measure().width, 0, "`content2`'s measured width should be 0");
assert.sameValue(content2.measure().height, 0, "`content2`'s measured height should be 0");
assert.sameValue(content2.width, 240, "`content2`'s fitted width should be 0");
assert.sameValue(content2.height, 320, "`content2`'s fitted height should be 0");