/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/526
flags: [onlyStrict]
---*/

let source = `
var NISLFuzzingFunc = function() {
    print("hello");
};
};
`;

assert.throws(SyntaxError, () => eval(source));
