/*---
description:
flags: [module]
---*/

import Cipher, {V, hex} from "./cipher_FIXTURE.js";

const c = new Cipher({algorithm: "AES-ECB", key: hex(V.ecb[0].key), direction: "encrypt"});
c.close();

// double close is a no-op
c.close();

// every method/getter after close throws SyntaxError
assert.throws(SyntaxError, () => c.encrypt(new Uint8Array(16)), "encrypt after close");
assert.throws(SyntaxError, () => c.encrypt(),                   "flush after close");
assert.throws(SyntaxError, () => c.reset(),                     "reset after close");
assert.throws(SyntaxError, () => c.blockSize,                   "blockSize after close");
assert.throws(SyntaxError, () => c.keySize,                     "keySize after close");
assert.throws(SyntaxError, () => c.tagLength,                   "tagLength after close");
