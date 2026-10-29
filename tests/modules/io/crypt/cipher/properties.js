/*---
description:
flags: [module]
---*/

import Cipher, {V, hex} from "./cipher_FIXTURE.js";

const tagLength = Cipher.supports({algorithm: "AES-GCM", tagLength: 12}) ? 12 : 16;
using c = new Cipher({algorithm: "AES-GCM", key: hex(V.ecb[1].key), direction: "encrypt", iv: hex(V.gcm[0].iv), tagLength});
assert.sameValue(c.blockSize, 16, "blockSize");
assert.sameValue(c.keySize, 192, "keySize in bits");
assert.sameValue(c.tagLength, tagLength, "tagLength");

// getter-only descriptors: strict-mode assignment throws TypeError
assert.throws(TypeError, () => { c.blockSize = 0; }, "blockSize is read-only");
assert.throws(TypeError, () => { c.keySize = 0; },   "keySize is read-only");
assert.throws(TypeError, () => { c.tagLength = 0; }, "tagLength is read-only");
