/*---
description:
flags: [async, module]
---*/

import crypto, {Crypto, assertRejects, DOMException} from "./webcrypto_FIXTURE.js";

(async () => {
	assert.sameValue(crypto instanceof Crypto, true);
	assert.sameValue(crypto.subtle, crypto.subtle, "subtle is stable");

	const a = new Uint8Array(32);
	assert.sameValue(crypto.getRandomValues(a), a, "returns the array");
	assert.sameValue(a.some(b => 0 !== b), true, "filled");
	const b = new Uint8Array(32);
	crypto.getRandomValues(b);
	assert.notSameValue(a.toHex(), b.toHex(), "two calls differ");
	for (const T of [Int8Array, Uint8ClampedArray, Int16Array, Uint16Array, Int32Array, Uint32Array, BigInt64Array, BigUint64Array])
		crypto.getRandomValues(new T(4));
	const view = new Uint8Array(new ArrayBuffer(8), 4, 2);
	crypto.getRandomValues(view);
	assert.sameValue(new Uint8Array(view.buffer).subarray(0, 4).every(x => 0 === x), true, "respects offset");
	crypto.getRandomValues(new Uint8Array(65536));

	for (const [label, value] of [["Float32Array", new Float32Array(4)], ["Float64Array", new Float64Array(2)], ["DataView", new DataView(new ArrayBuffer(4))], ["ArrayBuffer", new ArrayBuffer(4)], ["array", [1, 2]]]) {
		try {
			crypto.getRandomValues(value);
			throw new Test262Error(`${label} accepted`);
		}
		catch (e) {
			assert.sameValue(e instanceof DOMException && "TypeMismatchError" === e.name, true, `${label}: TypeMismatchError (${e.name})`);
		}
	}
	try {
		crypto.getRandomValues(new Uint8Array(65537));
		throw new Test262Error("65537 accepted");
	}
	catch (e) {
		assert.sameValue(e instanceof DOMException && "QuotaExceededError" === e.name, true, "QuotaExceededError");
	}

	const uuid = crypto.randomUUID();
	assert.sameValue(/^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/.test(uuid), true, `uuid v4 format ${uuid}`);
	assert.notSameValue(crypto.randomUUID(), uuid, "uuids differ");
})().then(() => $DONE(), $DONE);
