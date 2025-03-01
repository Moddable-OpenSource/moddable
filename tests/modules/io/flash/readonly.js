/*---
description: 
flags: [module]
---*/

import flash from "./flash-FIXTURE.js";
import {path} from "./flash-FIXTURE.js";

let f = flash.open({path});
f.eraseBlock(0);
f.close();

f = flash.open({path, mode: "r"});
assert.throws(Error, () => f.write(new ArrayBuffer(12), 0));

let bytes = new Uint8Array(f.read(12, 0));
for (let i = 0; i < 12; i++)
	assert.sameValue(bytes[i], 0xff);
f.close();

f = flash.open({path});
f.write(new ArrayBuffer(12), 0);
f.close();

f = flash.open({path, mode: "r"});
assert.throws(Error, () => f.eraseBlock(0));
bytes = new Uint8Array(f.read(12, 0));
for (let i = 0; i < 12; i++)
	assert.sameValue(bytes[i], 0);

f.close();
