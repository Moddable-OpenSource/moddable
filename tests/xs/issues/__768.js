/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/768
flags: [onlyStrict]
negative:
  type: TypeError
---*/

function JSEtest() {
    return { __proto__: setInterval(/a/g, "") } instanceof Array
        && !({ __proto__: null } instanceof Object);
}

assert.sameValue(JSEtest(), false, "");

