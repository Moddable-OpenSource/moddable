/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/115
flags: [onlyStrict]
---*/

function f1(x = 0, read = () => x) {
  var x = 1;

  assert.sameValue(0, read());
  assert.sameValue(1, x);
}
f1();

function f2(x = 0, read = () => x, write = val => { x = val; }) {
  var x;

  write(1);
  assert.sameValue(1, read());
  assert.sameValue(0, x);
}
f2();
