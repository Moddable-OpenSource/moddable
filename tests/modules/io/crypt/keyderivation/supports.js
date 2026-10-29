/*---
description: KeyDerivation.supports: the four algorithms over the hashes
flags: [module]
---*/

import KeyDerivation from "embedded:crypt/keyderivation";

for (const algorithm of ["HKDF", "HKDF-Extract", "HKDF-Expand", "PBKDF2"]) {
	assert.sameValue(KeyDerivation.supports({algorithm}), true, algorithm);
	assert.sameValue(KeyDerivation.supports({algorithm, hash: "SHA-256"}), true, `${algorithm} SHA-256`);
	assert.sameValue(KeyDerivation.supports({algorithm, hash: "SHA-3"}), false, `${algorithm} unknown hash`);
}
assert.sameValue(KeyDerivation.supports({algorithm: "Argon2"}), false, "unknown algorithm");
assert.throws(TypeError, () => KeyDerivation.supports(), "options required");
