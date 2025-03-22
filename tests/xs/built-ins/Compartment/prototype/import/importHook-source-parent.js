/*---
description: 
flags: [onlyStrict,async]
---*/

const parentCompartment = new Compartment({
  importHook(specifier) {
    if (specifier === 'bar') {
      return {
        source: new ModuleSource('export const meaning = 42;'),
      };
    }
    throw new Error(`Unexpected specifier in parent ${specifier}`);
  },
});

const childCompartment = new parentCompartment.globalThis.Compartment({
  importHook(specifier) {
    if (specifier === 'foo') {
      return {
        source: 'bar',
      };
    }
    throw new Error(`Unexpected specifier in child ${specifier}`);
  },
});

Promise.all([
	childCompartment.import('foo'),
	parentCompartment.import('bar'),
])
.then(([ child, parent ]) => {
	assert.sameValue(child.meaning, 42, "child export");
	assert.sameValue(parent.meaning, 42, "parent export");
	assert(child !== parent, "separate namepaces");
})
.then($DONE, $DONE);
