/*---
description: Cipher.supports answers for the symmetric table and the RSA provider
flags: [module]
---*/

import Cipher from "embedded:crypt/cipher";

for (const algorithm of ["AES-ECB", "AES-CBC", "AES-CTR", "AES-GCM", "ChaCha20-Poly1305", "RSA-OAEP", "RSAES-PKCS1-v1_5"])
	assert.sameValue(Cipher.supports({algorithm}), true, algorithm);
assert.sameValue(typeof Cipher.supports({algorithm: "ChaCha20"}), "boolean", "ChaCha20 is absent from the macOS backend");
// the constructor's properties, and more
assert.sameValue(Cipher.supports({algorithm: "AES-GCM", tagLength: 16}), true, "GCM full tag");
assert.sameValue(Cipher.supports({algorithm: "AES-GCM", tagLength: 17}), false, "GCM tag too long");
assert.sameValue(Cipher.supports({algorithm: "AES-CTR", tagLength: 16}), false, "tagLength is the authenticated modes'");
assert.sameValue(Cipher.supports({algorithm: "AES-CBC", iv: new Uint8Array(16)}), true, "CBC iv");
assert.sameValue(Cipher.supports({algorithm: "AES-CBC", iv: new Uint8Array(12)}), false, "CBC iv of the wrong size");
assert.sameValue(Cipher.supports({algorithm: "AES-ECB", iv: new Uint8Array(16)}), false, "ECB has no iv");
assert.sameValue(Cipher.supports({algorithm: "AES-CBC", more: true}), true, "the block modes stream");
assert.sameValue(typeof Cipher.supports({algorithm: "AES-GCM", more: true}), "boolean", "an authenticated mode may be one shot");
assert.sameValue(Cipher.supports({algorithm: "DES"}), false, "unknown algorithm");
assert.sameValue(Cipher.supports({algorithm: "RSA-PSS"}), false, "not a cipher");
assert.sameValue(Cipher.supports({algorithm: "RSA-OAEP", hash: "SHA-256"}), true, "RSA-OAEP with its hash");
assert.sameValue(Cipher.supports({algorithm: "AES-GCM", hash: "SHA-256"}), false, "AES-GCM does not accept hash");
assert.sameValue(Cipher.supports({algorithm: "RSAES-PKCS1-v1_5", hash: "SHA-256"}), false, "RSAES-PKCS1-v1_5 does not accept hash");
assert.throws(TypeError, () => Cipher.supports(), "options required");
