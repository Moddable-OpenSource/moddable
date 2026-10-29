/*---
description:
flags: [async, module]
---*/

import {subtle, V, hex, toHex, assertRejects} from "./webcrypto_FIXTURE.js";

(async () => {
	for (const vec of V.cbc_pkcs7) {
		const key = await subtle.importKey("raw", hex(vec.key), "AES-CBC", false, ["encrypt", "decrypt"]);
		assert.sameValue(key.algorithm.length, 128);
		const iv = hex(vec.iv);
		assert.sameValue(toHex(await subtle.encrypt({name: "AES-CBC", iv}, key, hex(vec.pt))), vec.ct, `encrypt ${vec.pt.length / 2}B`);
		assert.sameValue(toHex(await subtle.decrypt({name: "aes-cbc", iv}, key, hex(vec.ct))), vec.pt, `decrypt ${vec.pt.length / 2}B`);
	}
	const key = await subtle.importKey("raw", hex(V.cbc_pkcs7[0].key), {name: "AES-CBC"}, false, ["encrypt", "decrypt"]);
	const iv = hex(V.cbc_pkcs7[0].iv), data = hex(V.cbc_pkcs7[0].pt);
	await assertRejects(subtle.encrypt({name: "AES-CBC", iv: iv.subarray(0, 12)}, key, data), "OperationError", "iv 12 bytes");
	await assertRejects(subtle.encrypt({name: "AES-CBC"}, key, data), TypeError, "iv missing");
	await assertRejects(subtle.decrypt({name: "AES-CBC", iv}, key, new Uint8Array(20)), "OperationError", "unaligned ciphertext");
	await assertRejects(subtle.decrypt({name: "AES-CBC", iv}, key, new Uint8Array(0)), "OperationError", "empty ciphertext");
	await assertRejects(subtle.decrypt({name: "AES-CBC", iv}, key, new Uint8Array(32)), "OperationError", "bad padding");
	const encOnly = await subtle.importKey("raw", hex(V.cbc_pkcs7[0].key), "AES-CBC", false, ["encrypt"]);
	await assertRejects(subtle.decrypt({name: "AES-CBC", iv}, encOnly, hex(V.cbc_pkcs7[0].ct)), "InvalidAccessError", "no decrypt usage");
	const gcmKey = await subtle.importKey("raw", hex(V.cbc_pkcs7[0].key), "AES-GCM", false, ["encrypt"]);
	await assertRejects(subtle.encrypt({name: "AES-CBC", iv}, gcmKey, data), "InvalidAccessError", "GCM key with CBC");
	await assertRejects(subtle.encrypt({name: "AES-CBC", iv}, {}, data), TypeError, "not a CryptoKey");
})().then(() => $DONE(), $DONE);
