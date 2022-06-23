/*---
description: 
flags: [onlyStrict]
---*/

const content = new Image(null, {
    active: true, top: 0, left: 0, height: 100, width: 100, path: "screen2.cs"
});

new Application(null, {
    contents: [ content ]
});

assert.sameValue(application.hit(10, 10), content, "hit should return `content`");
assert.sameValue(application.hit(200, 200), undefined, "hit should return `undefined`");
assert.sameValue(content.hit(10, 10), content, "hit should return `content`");
assert.sameValue(content.hit(200, 200), undefined, "hit should return `undefined`");