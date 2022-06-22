/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/739
flags: [onlyStrict]
negative:
  type: RangeError
---*/

function assertThrows(code, type_opt, cause_opt) {
    if (typeof code === 'function')
        return code();
    if (typeof code === 'string')
        return eval(code);
}
let proxy = new Proxy(function () {
}, {});
for (let i = 0; i < 100000; i++) {
    proxy = new Proxy(proxy, {});
}
try {
    Reflect.apply(proxy, {}, []);
} catch (_) {
}
try {
    Reflect.construct(proxy, []);
} catch (_) {
}
try {
    Reflect.defineProperty(proxy, 'x', {});
} catch (_) {
}
try {
    Reflect.deleteProperty(proxy, 'x');
} catch (_) {
}
try {
    Reflect.get(proxy, 'x');
} catch (_) {
}
try {
    Reflect.getOwnPropertyDescriptor(proxy, 'x');
} catch (_) {
}
try {
    Reflect.getPrototypeOf(proxy);
} catch (_) {
}
try {
    Reflect.has(proxy, 'x');
} catch (_) {
}
try {
    Reflect.isExtensible(proxy);
} catch (_) {
}
try {
    Reflect.ownKeys(proxy);
} catch (_) {
}
try {
    Reflect.preventExtensions(proxy);
} catch (_) {
}
try {
    Reflect.setPrototypeOf(proxy, {});
} catch (_) {
}
try {
    Reflect.set(proxy, 'x', {});
} catch (_) {
}
function run(trap, ...args) {
    let handler = {};
    const proxy = new Proxy(function () {
    }, handler);
    handler[trap] = (target, ...args) => Reflect[trap](proxy, ...args);
    return Reflect[trap](proxy, ...args);
}
assertThrows(() => run('apply', {}, []), RangeError);
assertThrows(() => run('construct', []), RangeError);
assertThrows(() => run('defineProperty', 'x', {}), RangeError);
assertThrows(() => run('deleteProperty', 'x'), RangeError);
assertThrows(() => run('get', 'x'), RangeError);
assertThrows(() => run('getOwnPropertyDescriptor', 'x'), RangeError);
assertThrows(() => run('has', 'x'), RangeError);
assertThrows(() => run('isExtensible'), RangeError);
assertThrows(() => run('ownKeys'), RangeError);
assertThrows(() => run('preventExtensions'), RangeError);
assertThrows(() => run('setPrototypeOf', {}), RangeError);
assertThrows(() => run('set', 'x', {}), RangeError);
