/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/59
flags: [onlyStrict]
---*/

const source = `
884e
`;

assert.throws(SyntaxError, () => eval(source));

