/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/743
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function shouldThrowStackOverflow(f) {
    let threw = false;
    const verbose = false;
//     try {
        f();
//     } catch (e) {
//         threw = e instanceof RangeError;
//     }
}
const emptyFunction = function () {
};
let seenStartObjects = new Map();
function makeLongProxyChain(startObject = emptyFunction) {
    if (seenStartObjects.has(startObject))
        return seenStartObjects.get(startObject);
    let p = new Proxy(startObject, {});
    for (let i = 0; i < 500000; i++)
        p = new Proxy(p, {});
    seenStartObjects.set(startObject, p);
    return p;
}
shouldThrowStackOverflow(function longProxyChain() {
    let f = makeLongProxyChain();
    f.name;
});
