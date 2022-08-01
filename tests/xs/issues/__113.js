/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/113
flags: [onlyStrict]
---*/

const array = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
let first = true;
array.sort((a, b) => {
  if (first) {
    first = false;
    for (let i = 0; i < array.length; i += 1) {
      delete array[i];
    }
  }
  return a - b;
});
