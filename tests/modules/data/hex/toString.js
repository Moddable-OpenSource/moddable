/*---
description: 
flags: [module]
---*/

import Hex from "hex";

const bytes = Uint8Array.of(0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef);

assert.sameValue("0123456789ABCDEF", Hex.toString(bytes), "Uint8Array");
assert.sameValue("0123456789ABCDEF", Hex.toString(bytes.buffer), "ArrayBuffer");

assert.sameValue("0123456789ABCDEF", Hex.toString(bytes, ""), "Uint8Array empty separator");
assert.sameValue("01.23.45.67.89.AB.CD.EF", Hex.toString(bytes, "."), "Uint8Array .");
assert.sameValue("01-23-45-67-89-AB-CD-EF", Hex.toString(bytes, "-"), "Uint8Array -");
assert.throws(Error, () => Hex.toString(bytes, undefined), "undefined separator");
assert.sameValue("011231451671891AB1CD1EF", Hex.toString(bytes, 1), "number separator");
assert.sameValue("010230450670890AB0CD0EF", Hex.toString(bytes, 0n), "bigint separator");
assert.throws(Error, () => Hex.toString(bytes, `🤦`), "UTF-8 separator");
assert.throws(Error, () => Hex.toString(bytes, `123`), "multi-character separator");
assert.throws(Error, () => Hex.toString(bytes, `\0`), "null separator");

assert.sameValue("0123456789abcdef", Hex.toString(bytes, "", "0123456789abcdef"), "Uint8Array lower");
assert.sameValue("01-23-45-67-89-ab-cd-ef", Hex.toString(bytes, "-", "0123456789abcdef"), "Uint8Array - lower");
assert.sameValue("AB-CD-EF-GH-IJ-KL-MN-OP", Hex.toString(bytes, "-", "ABCDEFGHIJKLMNOP"), "Uint8Array - letters");
assert.throws(Error, () => Hex.toString(bytes, "", ""), "empty hex");
assert.throws(Error, () => Hex.toString(bytes, "", "0123456789abcde\0"), "null in hex");
assert.throws(Error, () => Hex.toString(bytes, "", "0123456789abcdefg"), "hex too long");
assert.throws(Error, () => Hex.toString(bytes, "", undefined), "hex undefined");
assert.throws(Error, () => Hex.toString(bytes, "", 1n), "bigint hex");

assert.sameValue("01", Hex.toString(Uint8Array.of(1)), "byte");

assert.throws(Error, () => Hex.toString(Uint8Array.of()), "empty");
assert.throws(TypeError, () => Hex.toString("string"), "input string");
assert.throws(TypeError, () => Hex.toString(Symbol()), "input symbol");
assert.throws(SyntaxError, () => Hex.toString(), "no arguments");
