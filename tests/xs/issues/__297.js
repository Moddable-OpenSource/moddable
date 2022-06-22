/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/297
flags: [onlyStrict]
---*/

const p = Proxy.revocable(function(){}, {});
p.revoke();
assert.sameValue("function", typeof p.proxy);
