/*---
description: 
flags: [module]
---*/

import Base64 from "base64";

// from RFC 4648
assert.sameValue(Base64.encode(""), "");
assert.sameValue(Base64.encode("f"), "Zg==");
assert.sameValue(Base64.encode("fo"), "Zm8=");
assert.sameValue(Base64.encode("foo"), "Zm9v");
assert.sameValue(Base64.encode("foob"), "Zm9vYg==");
assert.sameValue(Base64.encode("fooba"), "Zm9vYmE=");
assert.sameValue(Base64.encode("foobar"), "Zm9vYmFy");

assert.sameValue(Base64.encode(new ArrayBuffer(4)), "AAAAAA==");
assert.sameValue(Base64.encode(Uint8Array.of(1, 2, 3, 4, 5)), "AQIDBAU=");
assert.sameValue(Base64.encode(new ArrayBuffer(0)), "");

assert.throws(SyntaxError, () => Base64.encode(), "no argument");
assert.throws(TypeError, () => Base64.encode(Symbol()), "symbol argument");
assert.throws(TypeError, () => Base64.encode(11), "number argument");

const encode = Base64.encode;
assert.sameValue(encode("foobar"), "Zm9vYmFy");

