/*---
description: 
flags: [onlyStrict]
---*/

const parentCompartment = new Compartment({
  importNowHook(specifier) {
    if (specifier === 'bar') {
      return {
        source: new ModuleSource('export const meaning = 42;'),
      };
    }
    throw new Error(`Unexpected specifier in parent ${specifier}`);
  },
});

const childCompartment = new parentCompartment.globalThis.Compartment({
  importNowHook(specifier) {
    if (specifier === 'foo') {
      return {
        source: 'bar',
      };
    }
    throw new Error(`Unexpected specifier in child ${specifier}`);
  },
});

const child = childCompartment.importNow('foo');
const parent = parentCompartment.importNow('bar');

assert.sameValue(child.meaning, 42, "child export");
assert.sameValue(parent.meaning, 42, "parent export");
assert(child !== parent, "separate namepaces");
