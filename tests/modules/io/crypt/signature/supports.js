/*---
description: Signature.supports: algorithms and hashes
flags: [module]
---*/

import Signature from "embedded:crypt/signature";

for (const algorithm of ["RSASSA-PKCS1-v1_5", "RSA-PSS", "ECDSA"])
	assert.sameValue(Signature.supports({algorithm}), true, algorithm);
assert.sameValue(Signature.supports({algorithm: "RSA-PSS", hash: "SHA-256"}), true, "PSS SHA-256");
assert.sameValue(Signature.supports({algorithm: "RSA-PSS", hash: "SHA-3"}), false, "unknown hash");
assert.sameValue(Signature.supports({algorithm: "RSA-PSS", hash: "SHA-256", saltLength: 32}), true, "PSS with the hash's salt length");
assert.sameValue(Signature.supports({algorithm: "RSASSA-PKCS1-v1_5", hash: "SHA-256", saltLength: 32}), false, "saltLength is RSA-PSS's");
assert.sameValue(Signature.supports({algorithm: "Ed25519"}), false, "not implemented");
assert.sameValue(typeof Signature.supports({algorithm: "ECDSA", hash: "SHA-256"}), "boolean", "deterministic ECDSA depends on the backend");
assert.sameValue(Signature.supports({algorithm: "ECDSA", hash: "SHA-3"}), false, "ECDSA with an unknown hash");
assert.throws(TypeError, () => Signature.supports(), "options required");
