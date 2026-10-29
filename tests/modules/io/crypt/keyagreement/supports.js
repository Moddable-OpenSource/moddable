/*---
description: KeyAgreement.supports: ECDH over the curves this build includes
flags: [module]
---*/

import KeyAgreement from "embedded:crypt/keyagreement";

assert.sameValue(KeyAgreement.supports({algorithm: "ECDH"}), true, "ECDH");
for (const namedCurve of ["P-256", "P-384", "X25519"])
	assert.sameValue(KeyAgreement.supports({algorithm: "ECDH", namedCurve}), true, namedCurve);
assert.sameValue(KeyAgreement.supports({algorithm: "ECDH", namedCurve: "P-521"}), false, "P-521");
assert.sameValue(KeyAgreement.supports({algorithm: "DH"}), false, "unknown algorithm");
assert.throws(TypeError, () => KeyAgreement.supports(), "options required");
