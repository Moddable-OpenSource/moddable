/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/404
flags: [onlyStrict]
---*/

assert.sameValue('"[object Object]"', JSON.stringify(Array.prototype.toString.call( { } )));
