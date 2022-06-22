/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/462
flags: [onlyStrict]
---*/

const source = `
var x;
var counter = 0;
for ([...{1: ()}] of [[
            1,
            2,
            3
        ]]) {
    counter += 1;
}
`;

assert.throws(SyntaxError, () => eval(source));
