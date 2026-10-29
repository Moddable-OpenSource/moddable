/*---
description: MAC.supports: HMAC over the hashes
flags: [module]
---*/

import MAC from "embedded:crypt/MAC";

assert.sameValue(MAC.supports({algorithm: "HMAC"}), true, "HMAC");
for (const hash of ["MD5", "SHA-1", "SHA-224", "SHA-256", "SHA-384", "SHA-512"])
	assert.sameValue(MAC.supports({algorithm: "HMAC", hash}), true, hash);
assert.sameValue(MAC.supports({algorithm: "HMAC", hash: "SHA-3"}), false, "unknown hash");
assert.sameValue(MAC.supports({algorithm: "HMAC", hash: "SHA-3"}), false, "unknown hash");
assert.sameValue(MAC.supports({algorithm: "CMAC"}), false, "unknown algorithm");
assert.throws(TypeError, () => MAC.supports(), "options required");
