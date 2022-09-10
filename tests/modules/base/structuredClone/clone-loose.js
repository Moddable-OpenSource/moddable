/*---
description:
flags: [module]
---*/

import structuredClone from "structuredClone";

{
  const s = Symbol('s');
  const o = Object.create(
    {
      p: {
        enumerable: true,
        value: 0
      }
    },
    {
      [s]: {
        enumerable: true,
        value: 0
      },
      cew: {
        value: 0
      },
      cEw: {
        enumerable: true,
        value: 0
      },
      CEw: {
        configurable: true,
        enumerable: true,
        value: 0
      },
      cEW: {
        enumerable: true,
        writable: true,
        value: 0
      },
      CEW: {
        configurable: true,
        enumerable: true,
        writable: true,
        value: 0
      },
      g: {
        enumerable: true,
        get: function() {
          return 0;
        }
      }
    }
  );
  const sc = structuredClone(o);
  assert.sameValue(Object.getPrototypeOf(sc), Object.prototype, "prototype");
  assert.sameValue("p" in o, true, "prototype property");
  assert.sameValue("p" in sc, false, "prototype property");
  assert.sameValue(s in o, true, "symbol property");
  assert.sameValue(s in sc, false, "symbol property");
  assert.sameValue("cew" in o, true, "non enumerable property");
  assert.sameValue("cew" in sc, false, "non enumerable property");

  const descriptors = Object.getOwnPropertyDescriptors(sc);
  for (let name in descriptors) {
    const descriptor = descriptors[name];
    assert.sameValue(descriptor.configurable, true, "configurable");
    assert.sameValue(descriptor.writable, true, "writable");
    assert.sameValue(descriptor.value, 0, "value");
  }
}

{
  class C {
    #x = 0;
    y = 0;
    get x() {
      return this.#x;
    }
  }
  const o = new C;
  const sc = structuredClone(o);
  assert.sameValue(Object.getPrototypeOf(sc), Object.prototype, "prototype");
  assert.sameValue(sc.x, undefined, "prototype getter");
  const descriptor = Object.getOwnPropertyDescriptor(sc, "y");
  assert.sameValue(descriptor.configurable, true, "configurable");
  assert.sameValue(descriptor.writable, true, "writable");
  assert.sameValue(descriptor.value, 0, "value");
}


function testClass(Class, ...args) {
  class SubClass extends Class {}
  const o = new SubClass(...args);
  const p = structuredClone(o);
  const s = Object.prototype.toString.call(o);
  assert(o instanceof SubClass, s);
  assert(p instanceof Class, s);
}
testClass(Array);
testClass(Boolean, false);
testClass(Number, 0);
testClass(String, "");
testClass(ArrayBuffer);
testClass(DataView, new ArrayBuffer());
testClass(Date);
testClass(Error);
testClass(Map);
testClass(RegExp);
testClass(Set);
testClass(Uint8Array, new ArrayBuffer());
