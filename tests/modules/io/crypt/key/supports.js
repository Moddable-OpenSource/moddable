/*---
description: Key.supports: algorithms and the curves this build includes
flags: [module]
---*/

import Key from "embedded:crypt/key";

assert.sameValue(Key.supports({algorithm: "RSA"}), true, "RSA");
assert.sameValue(Key.supports({algorithm: "EC"}), true, "EC");
for (const namedCurve of ["P-256", "P-384", "X25519"])		// all present on the simulator; a device manifest may omit some
	assert.sameValue(Key.supports({algorithm: "EC", namedCurve}), true, namedCurve);
assert.sameValue(Key.supports({algorithm: "EC", namedCurve: "P-521"}), false, "P-521");
assert.sameValue(Key.supports({algorithm: "DSA"}), false, "unknown algorithm");
assert.sameValue(Key.supports({algorithm: "RSA", namedCurve: "P-256"}), false, "RSA does not accept namedCurve");
assert.throws(TypeError, () => Key.supports(), "options required");
