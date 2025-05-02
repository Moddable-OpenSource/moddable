/*---
description: 
flags: [module]
---*/

import update from "embedded:update";
import flash from "embedded:storage/flash";

const running = flash.open({path: "running"});
const ota = update.open({partition: flash.open({path: "nextota"})});
const blockSize = running.status().blockLength;

ota.write(running.read(blockSize, 0));
ota.write(new Uint8Array(running.read(blockSize, blockSize)));
assert.throws(SyntaxError, () => ota.write());
assert.throws(TypeError, () => ota.write(1));
assert.throws(TypeError, () => ota.write("not a number"));
assert.throws(TypeError, () => ota.write({}));
assert.throws(TypeError, () => ota.write(new Uint32Array(1)));

ota.close();
