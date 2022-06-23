/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/258
flags: [onlyStrict]
---*/

var canonical = Symbol.for('s');
assert.sameValue("s", Symbol.keyFor(canonical));
