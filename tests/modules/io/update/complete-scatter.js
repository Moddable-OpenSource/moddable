/*---
description: 
flags: [module]
---*/

import update from "embedded:update";
import flash from "embedded:storage/flash";

const running = flash.open({path: "running"});
const nextota = flash.open({path: "nextota"});

const byteLength = Math.min(running.status().size, nextota.status().size); 		// partitions may not be identical size
const ota = update.open({partition: nextota, byteLength, mode: "w"});

let block = running.read(1024, 0);
assert.throws(SyntaxError, () => ota.write(block));

trace("begin long OTA test\n");
const blockLength = running.status().blockLength;
const readLength = blockLength * 3 - 1;
block = new ArrayBuffer(readLength);
for (let position = 0; position < byteLength; position += readLength * 2) {
	if ((byteLength - position) < readLength)
		block = new ArrayBuffer(byteLength - position);
	running.read(block, position);
	ota.write(block, position);
}

block = new ArrayBuffer(readLength);
for (let position = readLength; position < byteLength; position += readLength * 2) {
	if ((byteLength - position) < readLength)
		block = new ArrayBuffer(byteLength - position);
	running.read(block, position);
	ota.write(block, position);
}

trace("complete long OTA test\n")

ota.complete();
ota.close();
