/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/413
flags: [onlyStrict]
---*/

function * x ( ) { new . target } ;
x ( ) ;
