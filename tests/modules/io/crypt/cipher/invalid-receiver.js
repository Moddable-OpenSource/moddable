/*---
description:
flags: [module]
---*/

import Cipher, {V, hex} from "./cipher_FIXTURE.js";

const proto = Cipher.prototype;
const getters = ["blockSize", "keySize", "tagLength"].map(name => [name, Object.getOwnPropertyDescriptor(proto, name).get]);

for (const {label, value} of [
	{label: "plain object", value: {}},
	{label: "Array",        value: []},
	{label: "ArrayBuffer",  value: new ArrayBuffer(16)},
]) {
	assert.throws(SyntaxError, () => proto.encrypt.call(value, new Uint8Array(16)), `${label}: encrypt`);
	assert.throws(SyntaxError, () => proto.decrypt.call(value, new Uint8Array(16)), `${label}: decrypt`);
	assert.throws(SyntaxError, () => proto.reset.call(value),                       `${label}: reset`);
	for (const [name, get] of getters)
		assert.throws(SyntaxError, () => get.call(value), `${label}: ${name} getter`);
}
