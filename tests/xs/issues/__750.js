/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/750
flags: [onlyStrict]
---*/
function JSEtest(proxyTarget) {
  var {
    proxy,
    revoke
  } = Proxy.revocable(proxyTarget, new Proxy({}, {
    get(target, propertyKey, receiver) {
      revoke();
    }
  }));
  return proxy;
}

Object.getPrototypeOf(JSEtest({}));
