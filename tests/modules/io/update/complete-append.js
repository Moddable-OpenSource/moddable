/*---
description: 
flags: [module]
---*/

import update from "embedded:update";
import flash from "embedded:storage/flash";

const running = flash.open({path: "running"});
const nextota = flash.open({path: "nextota"});

const byteLength = Math.min(running.status().size, nextota.status().size); 		// partitions may not be identical size
let ota = update.open({partition: nextota, byteLength, mode: "a"});

let block = running.read(1024, 0);
assert.throws(Error, () => ota.write(block, 0));

trace("begin long OTA test\n");
const blockLength = running.status().blockLength;
const readLength = blockLength * 3 - 1;
block = new ArrayBuffer(readLength);
for (let position = 0; position < byteLength; position += readLength) {
	if ((byteLength - position) < readLength)
		block = new ArrayBuffer(byteLength - position);
	running.read(block, position);
	ota.write(block);
}
trace("complete long OTA test\n")

ota.complete();
ota.close();


ota = update.open({partition: nextota, byteLength, mode: "a"});
assert.throws(Error, () => ota.complete());

assert.throws(Error, () => ota.write(running.read(0, blockLength * 2)));
ota.close();
