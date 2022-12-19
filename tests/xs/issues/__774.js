/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/774
flags: [onlyStrict]
negative:
  type: TypeError
---*/

new ArrayBuffer(64).concat({})
