/*---
description: 
flags: [module]
---*/

import Hex from "hex";

assert.sameValue("01", Hex.toString(Hex.toBuffer("01")));
assert.sameValue("12", Hex.toString(Hex.toBuffer(12)));
assert.sameValue("0102", Hex.toString(Hex.toBuffer("0102")));
assert.sameValue("0102", Hex.toString(Hex.toBuffer("0102", "")));
assert.sameValue("0123456789ABCDEF", Hex.toString(Hex.toBuffer("0123456789ABCDEF")));
assert.sameValue("0102", Hex.toString(Hex.toBuffer("01-02", "-")));
assert.sameValue("0123456789ABCDEF", Hex.toString(Hex.toBuffer("010230450670890AB0CD0EF", 0)));

assert.throws(SyntaxError, () => Hex.toBuffer(), "no args");
assert.throws(TypeError, () => Hex.toBuffer(Symbol()), "symbol buffer");
assert.throws(Error, () => Hex.toBuffer("1"), "invalid string");
assert.throws(Error, () => Hex.toBuffer("1-", "-"), "invalid string");
assert.throws(Error, () => Hex.toBuffer("11-1", "-"), "invalid string");
assert.throws(Error, () => Hex.toBuffer("ZZ"), "bad data");
assert.throws(Error, () => Hex.toBuffer("01020304ABZZ"), "bad data");

assert.throws(Error, () => Hex.toBuffer("01", "\0"), "null separator");
assert.throws(Error, () => Hex.toBuffer("01", "🤦"), "emoji separator");
assert.throws(Error, () => Hex.toBuffer("01", "--"), "multi-character separator");
