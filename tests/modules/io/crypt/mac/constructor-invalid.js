/*---
description: MAC constructor rejects missing and invalid options
flags: [module]
---*/

import MAC, {hex} from "./mac_FIXTURE.js";

const key = hex("0b".repeat(20));
assert.throws(Error, () => new MAC({hash: "SHA-256", key}), "algorithm required");
assert.throws(Error, () => new MAC({algorithm: "CMAC", hash: "SHA-256", key}), "unsupported algorithm");
assert.throws(Error, () => new MAC({algorithm: "HMAC", key}), "hash required");
assert.throws(RangeError, () => new MAC({algorithm: "HMAC", hash: "SHA-3", key}), "unsupported hash");
assert.throws(RangeError, () => new MAC({algorithm: "HMAC", hash: "SHA-3", key}), "unknown hash");
assert.throws(Error, () => new MAC({algorithm: "HMAC", hash: "SHA-256"}), "key required");
assert.throws(TypeError, () => new MAC({algorithm: "HMAC", hash: "SHA-256", key: "secret"}), "key must be a byte buffer");
assert.throws(Error, () => new MAC({}), "empty options");
