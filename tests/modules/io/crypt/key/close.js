/*---
description:
flags: [module]
---*/

import Key, {V, hex, publicKey} from "./rsa_FIXTURE.js";

const k = publicKey();
k.close();
k.close();		// no-op
assert.throws(Error, () => k.type, "type after close");
assert.throws(Error, () => k.keySize, "keySize after close");
assert.throws(Error, () => k.extract("spki"), "extract after close");
assert.sameValue(k.algorithm, "RSA", "algorithm is static");
