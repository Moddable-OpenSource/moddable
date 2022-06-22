/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/414
flags: [onlyStrict]
---*/

const source = `
( { ... { } } = 0 ) ;
`;

assert.throws(SyntaxError, () => eval(source));
