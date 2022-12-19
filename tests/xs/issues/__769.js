/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/769
flags: [onlyStrict]
---*/

var arr = [];
for (var i = 0; i < 28000; i++) {
    arr.push(new RegExp("ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA" + i.toString(3)));
}
