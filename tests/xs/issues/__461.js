/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/461
flags: [onlyStrict]
---*/

const source = `
function f(x) {
    delete (());
    arguments[0] !== undefined;
}
f(1, x = [f.ArrayBuffer, undefined], this, this, this) ;
`;

assert.throws(SyntaxError, () => eval(source));
