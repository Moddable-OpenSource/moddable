/*---
description:
flags: [module]
---*/

import Random, {looksRandom} from "./random_FIXTURE.js";

using r = new Random({});

// read(count) allocates
for (const count of [0, 1, 7, 16, 256, 257, 1000, 8192]) {
	const result = r.read(count);
	assert.sameValue(result instanceof ArrayBuffer, true, `read(${count}) returns ArrayBuffer`);
	assert.sameValue(result.byteLength, count, `read(${count}) length`);
}
assert.sameValue(looksRandom(new Uint8Array(r.read(8192))), true, "every byte value appears in 8 KB");
assert.notSameValue(new Uint8Array(r.read(32)).toHex(), new Uint8Array(r.read(32)).toHex(), "successive reads differ");
assert.notSameValue(new Uint8Array(r.read(32)).toHex(), new Uint8Array((new Random({})).read(32)).toHex(), "instances do not repeat");

// read(buffer) fills in place and returns the count
const buffer = new Uint8Array(64);
assert.sameValue(r.read(buffer), 64, "count returned");
assert.sameValue(buffer.some(b => 0 !== b), true, "filled");
const view = new Uint8Array(new ArrayBuffer(16), 4, 8);
assert.sameValue(r.read(view), 8, "view count");
const whole = new Uint8Array(view.buffer);
assert.sameValue(whole.subarray(0, 4).every(b => 0 === b) && whole.subarray(12).every(b => 0 === b), true, "only the view is written");
assert.sameValue(r.read(new ArrayBuffer(0)), 0, "empty buffer");
assert.sameValue(r.read(new DataView(new ArrayBuffer(4))), 4, "DataView");
assert.sameValue(looksRandom((() => { const b = new Uint8Array(8192); r.read(b); return b; })()), true, "filled bytes look random");
