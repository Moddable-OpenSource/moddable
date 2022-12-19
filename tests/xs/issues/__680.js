/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/680
flags: [onlyStrict]
negative:
  type: RangeError
---*/

let evil1 = new RegExp;
evil1.exec = () => ({ 0: '1234567', length: 1, index: 0 });
'abc'.replace(evil1, `$'`);

let evil2 = new RegExp;
evil2.exec = () => ({ 0: 'x', length: 1, index: 3 });
'abc'.replace(evil2, `$'`);
