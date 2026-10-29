/*---
description:
flags: [module]
---*/

import Digest from "./digest_FIXTURE.js";

// Prototype methods/getters fail brand check on non-Digest receivers.
const proto = Object.getPrototypeOf(new Digest({algorithm: "SHA-256"}));
const blockSizeGet = Object.getOwnPropertyDescriptor(proto, "blockSize").get;
const outputSizeGet = Object.getOwnPropertyDescriptor(proto, "outputSize").get;

const aliens = [
	{label: "plain object", value: {}},
	{label: "Array",        value: []},
	{label: "ArrayBuffer",  value: new ArrayBuffer(16)},
];

for (const {label, value} of aliens) {
	assert.throws(SyntaxError, () => proto.write.call(value, Uint8Array.of(1)), `${label}: write`);
	assert.throws(SyntaxError, () => proto.read.call(value),                    `${label}: read`);
	assert.throws(SyntaxError, () => proto.reset.call(value),                   `${label}: reset`);
	assert.throws(SyntaxError, () => blockSizeGet.call(value),                  `${label}: blockSize getter`);
	assert.throws(SyntaxError, () => outputSizeGet.call(value),                 `${label}: outputSize getter`);
}
