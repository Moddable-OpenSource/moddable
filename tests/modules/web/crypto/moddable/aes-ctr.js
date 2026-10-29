/*---
description:
flags: [async, module]
---*/

import {subtle, V, hex, toHex, assertRejects} from "./webcrypto_FIXTURE.js";

(async () => {
	for (const vec of V.ctr) {
		const key = await subtle.importKey("raw", hex(vec.key), "AES-CTR", false, ["encrypt", "decrypt"]);
		const counter = hex(vec.iv);
		assert.sameValue(toHex(await subtle.encrypt({name: "AES-CTR", counter, length: 128}, key, hex(vec.pt))), vec.ct, `length 128 encrypt ${key.algorithm.length}`);
		assert.sameValue(toHex(await subtle.decrypt({name: "AES-CTR", counter, length: 64}, key, hex(vec.ct))), vec.pt, "length 64 (no wrap) decrypt");
	}
	for (const vec of [V.ctr_wrap32, V.ctr_wrap64]) {
		const key = await subtle.importKey("raw", hex(vec.key), "AES-CTR", false, ["encrypt", "decrypt"]);
		const counter = hex(vec.counter);
		assert.sameValue(toHex(await subtle.encrypt({name: "AES-CTR", counter, length: vec.length}, key, hex(vec.pt))), vec.ct, `counter wraps within ${vec.length} bits`);
		assert.sameValue(toHex(await subtle.decrypt({name: "AES-CTR", counter, length: vec.length}, key, hex(vec.ct))), vec.pt, `wrap ${vec.length} decrypt`);
	}
	const key = await subtle.importKey("raw", hex(V.ctr[0].key), "AES-CTR", false, ["encrypt"]);
	const counter = hex(V.ctr[0].iv), data = hex(V.ctr[0].pt);
	await assertRejects(subtle.encrypt({name: "AES-CTR", counter, length: 0}, key, data), "OperationError", "length 0");
	await assertRejects(subtle.encrypt({name: "AES-CTR", counter, length: 129}, key, data), "OperationError", "length 129");
	await assertRejects(subtle.encrypt({name: "AES-CTR", counter: counter.subarray(1), length: 64}, key, data), "OperationError", "counter 15 bytes");
	assert.sameValue((await subtle.encrypt({name: "AES-CTR", counter, length: 64}, key, new Uint8Array(0))).byteLength, 0, "empty");
})().then(() => $DONE(), $DONE);
