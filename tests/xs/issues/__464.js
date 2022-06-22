/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/464
flags: [onlyStrict]
---*/

let count = 0;
const it = {
  [Symbol.iterator]() {
    return this;
  },
  next() {
    count += 1;
    return {
      value: 42,
      done: true,
    };
  },
};

const [a, b, ...c] = it;

assert.sameValue(count, 1);
