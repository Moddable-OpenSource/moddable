/*---
description: 
flags: [module]
---*/

import TextDecoder from "text/decoder";

assert.throws(RangeError, () => new TextDecoder("ascii"), "utf-8 only");
assert.throws(RangeError, () => new TextDecoder("utf-16"), "utf-16 only");
assert.throws(RangeError, () => new TextDecoder("UTF-8"), "utf-16 only");
assert.throws(RangeError, () => new TextDecoder(1), "format is string");
assert.throws(RangeError, () => new TextDecoder({}), "format is string");

const decoder = new TextDecoder;

assert.sameValue("utf-8", (new TextDecoder).encoding);
assert.sameValue("utf-8", (new TextDecoder("utf-8")).encoding);

assert.sameValue(false, (new TextDecoder).ignoreBOM);
assert.sameValue(false, (new TextDecoder("utf-8", {})).ignoreBOM);
assert.sameValue(true, (new TextDecoder("utf-8", {ignoreBOM: true})).ignoreBOM);
assert.sameValue(true, (new TextDecoder("utf-8", {ignoreBOM: {}})).ignoreBOM);
assert.sameValue(false, (new TextDecoder("utf-8", {ignoreBOM: 0})).ignoreBOM);

assert.sameValue(false, (new TextDecoder).fatal);
assert.sameValue(false, (new TextDecoder("utf-8", {})).fatal);
assert.sameValue(true, (new TextDecoder("utf-8", {fatal: true})).fatal);
assert.sameValue(true, (new TextDecoder("utf-8", {fatal: {}})).fatal);
assert.sameValue(false, (new TextDecoder("utf-8", {fatal: 0})).fatal);

assert.throws(TypeError, () => decoder.encoding = "utf-16");
assert.throws(TypeError, () => decoder.fatal = true);
assert.throws(TypeError, () => decoder.ignoreBOM = false);
