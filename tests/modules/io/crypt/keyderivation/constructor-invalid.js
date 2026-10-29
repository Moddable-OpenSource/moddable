/*---
description: KeyDerivation constructor rejects missing, invalid and misplaced options
flags: [module]
---*/

import KeyDerivation from "./keyderivation_FIXTURE.js";

const key = new Uint8Array(32), salt = new Uint8Array(16), info = new Uint8Array(4);
assert.throws(Error, () => new KeyDerivation({hash: "SHA-256", key}), "algorithm required");
assert.throws(Error, () => new KeyDerivation({algorithm: "Argon2", hash: "SHA-256", key}), "unsupported algorithm");
assert.throws(Error, () => new KeyDerivation({algorithm: "HKDF", key}), "hash required");
assert.throws(RangeError, () => new KeyDerivation({algorithm: "HKDF", hash: "SHA-3", key}), "unsupported hash");
assert.throws(Error, () => new KeyDerivation({algorithm: "HKDF", hash: "SHA-256"}), "key required");
assert.throws(TypeError, () => new KeyDerivation({algorithm: "HKDF", hash: "SHA-256", key: "secret"}), "key must be a byte buffer");
assert.throws(RangeError, () => new KeyDerivation({algorithm: "HKDF", hash: "SHA-256", key, iterations: 2}), "iterations is PBKDF2 only");
assert.throws(RangeError, () => new KeyDerivation({algorithm: "HKDF-Extract", hash: "SHA-256", key, info}), "Extract takes no info");
assert.throws(RangeError, () => new KeyDerivation({algorithm: "HKDF-Expand", hash: "SHA-256", key, salt}), "Expand takes no salt");
assert.throws(RangeError, () => new KeyDerivation({algorithm: "PBKDF2", hash: "SHA-256", key, salt, iterations: 1, info}), "PBKDF2 takes no info");
assert.throws(RangeError, () => new KeyDerivation({algorithm: "HKDF", hash: "SHA-256", key}).read(-1), "negative count");
assert.throws(Error, () => new KeyDerivation({}), "empty options");
