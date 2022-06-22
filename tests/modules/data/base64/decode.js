/*---
description: 
flags: [module]
---*/

import Base64 from "base64";

// from RFC 4648
assert.sameValue("", String.fromArrayBuffer(Base64.decode("")));
assert.sameValue("f", String.fromArrayBuffer(Base64.decode("Zg==")));
assert.sameValue("fo", String.fromArrayBuffer(Base64.decode("Zm8=")));
assert.sameValue("foo", String.fromArrayBuffer(Base64.decode("Zm9v")));
assert.sameValue("foob", String.fromArrayBuffer(Base64.decode("Zm9vYg==")));
assert.sameValue("fooba", String.fromArrayBuffer(Base64.decode("Zm9vYmE=")));
assert.sameValue("foobar", String.fromArrayBuffer(Base64.decode("Zm9vYmFy")));

assert.sameValue("", String.fromArrayBuffer(Base64.decode(" &@!🤦")));
assert.sameValue("f", String.fromArrayBuffer(Base64.decode("🤦🤦Z🤦g🤦==")));
assert.sameValue("f", String.fromArrayBuffer(Base64.decode("Zg==🤦🤦")));
assert.sameValue("", String.fromArrayBuffer(Base64.decode("=====================")));

assert.sameValue(1, Base64.decode("Zg==").byteLength, "remove extra bytes from padding");
assert.sameValue(1, Base64.decode("Zg==\n\n\n\n\n\n\n\n\n\n").byteLength, "remove extra bytes from trailing garbage");
assert.sameValue(1, Base64.decode("\n\n\n\n\n\n\n\n\n\nZg==").byteLength, "remove extra bytes from leading garbage");

let b = Base64.decode(12.21);
assert.sameValue(b.byteLength, 3);
b = new Uint8Array(b);
assert.sameValue(b[0], 215);
assert.sameValue(b[1], 109);
assert.sameValue(b[2], 181);

assert.throws(SyntaxError, () => Base64.decode(), "no argument");
assert.throws(TypeError, () => Base64.decode(Symbol()), "symbol argument");

const decode = Base64.decode;
assert.sameValue("foobar", String.fromArrayBuffer(decode("Zm9vYmFy")));
