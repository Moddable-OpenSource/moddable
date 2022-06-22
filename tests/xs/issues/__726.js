/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/726
flags: [onlyStrict]
---*/

assert.throws(SyntaxError, () => eval(`void (Date,);`), "void (Date,);");
assert.throws(SyntaxError, () => eval(`(Date,) = null;`), "(Date,) = null;");
