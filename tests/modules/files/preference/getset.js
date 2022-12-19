/*---
description: 
flags: [module]
---*/

import Preference from "preference";

assert.throws(SyntaxError, () => Preference.set(), "preference.set requires 3 arguments")
assert.throws(SyntaxError, () => Preference.set("foo"), "preference.set requires 3 arguments")
assert.throws(SyntaxError, () => Preference.set("foo", "bar"), "preference.set requires 3 arguments")

Preference.set("foo", 1, 2);
assert.sameValue(Preference.get("foo", 1), 2, "preference get integer");

Preference.set("foo", 1, "ZYXé¡");
assert.sameValue(Preference.get("foo", 1), "ZYXé¡", "preference get string");

Preference.set("foo", 1, true);
assert.sameValue(Preference.get("foo", 1), true, "preference get true");

Preference.set("foo", 1, false);
assert.sameValue(Preference.get("foo", 1), false, "preference get false");

Preference.set("foo", 1, Uint8Array.of(1, 2, 3).buffer);
let b = Preference.get("foo", 1);
assert(b instanceof ArrayBuffer, "preference get isn't arraybuffer");
assert(3 === b.byteLength, "preference buffer wrong length");
b = new Uint8Array(b);
assert(1 == b[0] && 2 == b[1] && 3 == b[2], "preference buffer contents bad");

assert.throws(Error, () => Preference.set("foo", 1, undefined), "preference.set should reject undefined");
assert.throws(Error, () => Preference.set("foo", 1, null), "preference.set should reject null");
assert.throws(Error, () => Preference.set("foo", 1, 2.2), "preference.set should reject float");
assert.throws(Error, () => Preference.set("foo", 1, {x: 12}), "preference.set should reject object");
assert.throws(Error, () => Preference.set("foo", 1, Preference.set), "preference.set should reject function");
assert.throws(Error, () => Preference.set("foo", 1, 10001n), "preference.set should reject BigInt");

const long = "012345678901234";
Preference.set(long, long, 123);
assert.sameValue(Preference.get(long, long), 123, "preference get failed with long domain/key");
