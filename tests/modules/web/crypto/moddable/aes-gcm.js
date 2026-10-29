/*---
description:
flags: [async, module]
---*/

import {subtle, V, hex, toHex, assertRejects} from "./webcrypto_FIXTURE.js";
import Cipher from "embedded:crypt/cipher";

(async () => {
	for (const vec of V.gcm) {
		const key = await subtle.importKey("raw", hex(vec.key), "AES-GCM", false, ["encrypt", "decrypt"]);
		const params = {name: "AES-GCM", iv: hex(vec.iv)};
		if (vec.aad.length)
			params.additionalData = hex(vec.aad);
		const label = `GCM-${vec.key.length * 4} iv${vec.iv.length / 2} aad${vec.aad.length / 2} pt${vec.pt.length / 2}`;
		if (!Cipher.supports({algorithm: "AES-GCM", iv: params.iv})) {		// CryptoKit: a nonce of at least 12 bytes
			await assertRejects(subtle.encrypt(params, key, hex(vec.pt)), "OperationError", `${label} unsupported nonce`);
			continue;
		}
		assert.sameValue(toHex(await subtle.encrypt(params, key, hex(vec.pt))), vec.ct + vec.tag, `${label} encrypt`);
		assert.sameValue(toHex(await subtle.decrypt(params, key, hex(vec.ct + vec.tag))), vec.pt, `${label} decrypt`);
	}
	const vec = V.gcm[1];
	const key = await subtle.importKey("raw", hex(vec.key), "AES-GCM", false, ["encrypt", "decrypt"]);
	const iv = hex(vec.iv), additionalData = hex(vec.aad), pt = hex(vec.pt);
	for (const tagLength of [96, 64, 32]) {
		if (!Cipher.supports({algorithm: "AES-GCM", tagLength: tagLength >> 3})) {		// CryptoKit: the 16-byte tag only
			await assertRejects(subtle.encrypt({name: "AES-GCM", iv, additionalData, tagLength}, key, pt), "OperationError", `tagLength ${tagLength} unsupported`);
			continue;
		}
		const ct = await subtle.encrypt({name: "AES-GCM", iv, additionalData, tagLength}, key, pt);
		assert.sameValue(toHex(ct), vec.ct + vec.tag.slice(0, tagLength / 4), `tagLength ${tagLength}`);
		assert.sameValue(toHex(await subtle.decrypt({name: "AES-GCM", iv, additionalData, tagLength}, key, ct)), vec.pt, `tagLength ${tagLength} decrypt`);
	}
	const tampered = hex(vec.ct + vec.tag); tampered[0] ^= 1;
	await assertRejects(subtle.decrypt({name: "AES-GCM", iv, additionalData}, key, tampered), "OperationError", "tampered ciphertext");
	await assertRejects(subtle.decrypt({name: "AES-GCM", iv}, key, hex(vec.ct + vec.tag)), "OperationError", "missing aad");
	await assertRejects(subtle.decrypt({name: "AES-GCM", iv, additionalData}, key, new Uint8Array(15)), "OperationError", "shorter than tag");
	await assertRejects(subtle.encrypt({name: "AES-GCM", iv, tagLength: 100}, key, pt), "OperationError", "tagLength 100");
	await assertRejects(subtle.encrypt({name: "AES-GCM", iv: new Uint8Array(0)}, key, pt), "OperationError", "empty iv");
	// NIST test case 2
	const zeroKey = await subtle.importKey("raw", new Uint8Array(16), "AES-GCM", false, ["encrypt"]);
	assert.sameValue(toHex(await subtle.encrypt({name: "AES-GCM", iv: new Uint8Array(12)}, zeroKey, new Uint8Array(16))), V.gcm_tc2.ct + V.gcm_tc2.tag, "GCM TC2");
})().then(() => $DONE(), $DONE);
