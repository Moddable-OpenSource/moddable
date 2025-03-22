/*---
description: 
flags: [module]
---*/

import update from "embedded:update";
import flash from "embedded:storage/flash";

const running = flash.open({path: "running"});
const ota = update.open({partition: flash.open({path: "nextota"})});
ota.close();
ota.close();
ota.close();

assert.throws(SyntaxError, () => ota.complete());
const block = running.read(1024, 1024);
assert.throws(SyntaxError, () => ota.write(block));

