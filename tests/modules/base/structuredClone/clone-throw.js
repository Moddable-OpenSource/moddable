/*---
description:
flags: [module]
---*/

import structuredClone from "structuredClone";
import * as ns from "./clone_FIXTURE.js"

function test() {
  const tests = [
    arguments,
    globalThis,
    new Compartment(),
    new FinalizationRegistry(() => {}),
    function () {},
    (function*() {})(),
    ns,
    new Promise((resolve, reject) => {}),
    new Proxy({}, {}),
    new WeakMap(),
    new WeakRef({}),
    new WeakSet(),
  ];
  for (let o of tests) {
    const s = Object.prototype.toString.call(o);
    assert.throws(TypeError, () => structuredClone(o), s);
  }
  assert.throws(TypeError, () => structuredClone(), "no value");
}
test("oops");


