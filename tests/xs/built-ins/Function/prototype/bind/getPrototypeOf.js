/*---
description:
flags: []
---*/

let s = "";
const p = new Proxy(function() {}, {
  getPrototypeOf(target)  {
    s = "trapped";
    return Reflect.getPrototypeOf(target);
  }
});

p.bind(this);
assert.sameValue(s, "trapped", "bind [[getPrototypeOf]]");
