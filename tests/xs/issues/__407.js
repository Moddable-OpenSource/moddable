/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/407
flags: [onlyStrict]
---*/

'use strict';

assert.throws(RangeError, () => JSON.stringify(( 42 ) . toString ( 0 ) ));
