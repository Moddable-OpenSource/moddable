/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/463
flags: [onlyStrict]
---*/

const source = `
const handler = {}
function testArray(arr) {
    let proxy = new Proxy(arr, handler)
    proxy.sort((x, ()) => 1 * x - y);
    arr.sort((x, y) => 1 * x - y);
}
testArray([5, (this), 2, 99]);
`;

assert.throws(SyntaxError, () => eval(source));
