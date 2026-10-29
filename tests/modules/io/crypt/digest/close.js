/*---
description:
flags: [module]
---*/

import Digest from "./digest_FIXTURE.js";

const d = new Digest({algorithm: "SHA-256"});
d.close();

// double close is a no-op
d.close();

// every method/getter after close throws SyntaxError
assert.throws(SyntaxError, () => d.write(Uint8Array.of(1)), "write after close");
assert.throws(SyntaxError, () => d.read(),                  "read after close");
assert.throws(SyntaxError, () => d.read(new ArrayBuffer(32)), "read(buf) after close");
assert.throws(SyntaxError, () => d.reset(),                 "reset after close");
assert.throws(SyntaxError, () => d.blockSize,               "blockSize after close");
assert.throws(SyntaxError, () => d.outputSize,              "outputSize after close");
