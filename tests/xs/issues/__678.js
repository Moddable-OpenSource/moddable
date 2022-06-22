/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/678
flags: [onlyStrict]
---*/

assert.throws(TypeError, () => new BigInt64Array( new Int32Array(0) ));

assert.throws(TypeError, () => new Int32Array(new BigInt64Array(0))); 
