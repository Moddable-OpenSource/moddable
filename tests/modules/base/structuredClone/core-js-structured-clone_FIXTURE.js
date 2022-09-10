import deepEqual from "deepEqual";
import structuredClone from "structuredClone";

const GLOBAL = globalThis;
const NODE = null;

const { from } = Array;
const { assign, getPrototypeOf, keys } = Object;

function fromSource(s) {
  try {
    return eval(s);
  }
  catch {
  }
}

const QUnit = {
  test(title, f) {
    const assertion = {
      arrayEqual(actual, expected, message) {
        assert.compareArray(actual, expected, message);
      },
      arity(f, expected) {
        assert.sameValue(f.length, expected, "arity");
      },
      deepEqual(actual, expected, message) {
        assert(deepEqual(actual, expected), message);
      },
      isFunction(f, message) {
        assert(Object(f) instanceof Function, message);
      },
      looksNative(f, message) {
        //??
      },
      name(f, expected) {
         assert.sameValue(f.name, expected, "name");
      },
      notSame(actual, expected, message) {
        assert.notSameValue(actual, expected, message);
      },
      same(actual, expected, message) {
        assert.sameValue(actual, expected, message);
      },
      throws(f, message) {
        assert.throws(TypeError, f, message);
      },
      true(value, message) {
        assert(value, message);
      },
    }
    f(assertion);
  }
};

function cloneTest(value, verifyFunc) {
  verifyFunc(value, structuredClone(value));
}

// Specialization of cloneTest() for objects, with common asserts.
function cloneObjectTest(assert, value, verifyFunc) {
  cloneTest(value, (orig, clone) => {
    assert.notSame(orig, clone, 'clone should have different reference');
    assert.same(typeof clone, 'object', 'clone should be an object');
    // https://github.com/qunitjs/node-qunit/issues/146
    assert.true(getPrototypeOf(orig) === getPrototypeOf(clone), 'clone should have same prototype');
    verifyFunc(orig, clone);
  });
}

export { structuredClone, GLOBAL, NODE, from, assign, getPrototypeOf, keys, fromSource, QUnit, cloneTest, cloneObjectTest };


