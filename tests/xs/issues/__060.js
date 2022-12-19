/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/60
flags: [onlyStrict]
---*/

let x = {};
x.a = x;

try {
  JSON.stringify(x);
} catch(e) {}

delete x.a;

assert.sameValue(Object.keys(x).length, 0, 'Number of keys');

JSON.stringify(x); //throws here
