/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/87
flags: [onlyStrict]
---*/

let obj = {
  [2**24 - 2**20]: 0,
};

let obj2 = {
  [2**24 - 2**20 + 1]: 0,
};


function isEmptyKey(thing) {
  let keys = Reflect.ownKeys(thing);
  if (keys.length !== 1) throw 'too many keys';
  return (keys[0] === '');
}

assert.sameValue(false, isEmptyKey(obj));
assert.sameValue(false, isEmptyKey(obj2));
