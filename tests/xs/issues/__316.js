/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/316
flags: [onlyStrict]
---*/

const source = `
(() => { await 0; });
`;

assert.throws(SyntaxError, () => eval(source));
