/*---
description: 
flags: [onlyStrict]
---*/

new Application(null, { active: true });

assert.sameValue(application.hit(10, 10), application, "hit should return `application`");
assert.sameValue(application.hit(500, 500), undefined, "hit should return `undefined`");