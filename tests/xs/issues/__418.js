/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/418
flags: [onlyStrict]
---*/

assert.throws(TypeError, () => ( [ undefined ] = 0 ));
