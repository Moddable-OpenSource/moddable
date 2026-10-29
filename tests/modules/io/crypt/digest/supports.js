/*---
description: Digest.supports answers without constructing
flags: [module]
---*/

import Digest from "embedded:crypt/digest";

for (const algorithm of ["MD5", "SHA-1", "SHA-224", "SHA-256", "SHA-384", "SHA-512"]) {		// a build may omit some: supports agrees with construction
	let built = true;
	try { new Digest({algorithm}).close(); } catch { built = false; }
	assert.sameValue(Digest.supports({algorithm}), built, algorithm);
}
assert.sameValue(Digest.supports({algorithm: "SHA-256"}), true, "SHA-256 is always present");
assert.sameValue(Digest.supports({algorithm: "SHA-3"}), false, "unknown algorithm");
assert.sameValue(Digest.supports({}), false, "no algorithm");
assert.sameValue(Digest.supports({algorithm: 256}), false, "algorithm not a string");
assert.throws(TypeError, () => Digest.supports(), "options required");
assert.throws(TypeError, () => Digest.supports(null), "options must be an object");
