/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/82
flags: [onlyStrict]
---*/

const result = [];

let a = {
  x: 0,
  y: 1,
};

for (let key in a) {
  result.push(key);
  delete a.y;
  if (key === 'y') {
    result.push('reached');
  }
}
assert.sameValue(`["x"]`, JSON.stringify(result));
