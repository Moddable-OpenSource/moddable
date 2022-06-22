/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/642
flags: [onlyStrict]
---*/

let x = 5n ** 5n ** 6n;
x.toString();
