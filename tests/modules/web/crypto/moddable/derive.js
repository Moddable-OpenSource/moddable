/*---
description:
flags: [async, module]
---*/

import {subtle, V, hex, toHex, assertRejects} from "./webcrypto_FIXTURE.js";

(async () => {
	for (const vec of V.pbkdf2) {
		const key = await subtle.importKey("raw", hex(vec.password), "PBKDF2", false, ["deriveBits", "deriveKey"]);
		const bits = await subtle.deriveBits({name: "PBKDF2", hash: vec.hash, salt: hex(vec.salt), iterations: vec.iterations}, key, vec.length * 8);
		assert.sameValue(toHex(bits), vec.out, `PBKDF2-${vec.hash} ${vec.iterations} iterations`);
	}
	for (const vec of V.hkdf) {
		const key = await subtle.importKey("raw", hex(vec.ikm), {name: "HKDF"}, false, ["deriveBits"]);
		const bits = await subtle.deriveBits({name: "HKDF", hash: vec.hash, salt: hex(vec.salt), info: hex(vec.info)}, key, vec.length * 8);
		assert.sameValue(toHex(bits), vec.out, `HKDF-${vec.hash} ${vec.length}B`);
	}

	// deriveKey: PBKDF2 -> AES-GCM, HKDF -> HMAC
	const password = await subtle.importKey("raw", hex(V.pbkdf2[1].password), "PBKDF2", false, ["deriveKey", "deriveBits"]);
	const aes = await subtle.deriveKey({name: "PBKDF2", hash: "SHA-256", salt: hex(V.pbkdf2[1].salt), iterations: 1000}, password, {name: "AES-GCM", length: 256}, true, ["encrypt", "decrypt"]);
	assert.sameValue(aes.algorithm.name, "AES-GCM");
	assert.sameValue(aes.algorithm.length, 256);
	assert.sameValue(toHex(await subtle.exportKey("raw", aes)), V.pbkdf2[1].out, "derived AES key bytes");
	const iv = new Uint8Array(12);
	const ct = await subtle.encrypt({name: "AES-GCM", iv}, aes, hex(V.pbkdf2[1].salt));
	assert.sameValue(toHex(await subtle.decrypt({name: "AES-GCM", iv}, aes, ct)), V.pbkdf2[1].salt, "derived key round trip");

	const ikm = await subtle.importKey("raw", hex(V.hkdf[0].ikm), "HKDF", false, ["deriveKey", "deriveBits"]);
	const mac = await subtle.deriveKey({name: "HKDF", hash: "SHA-256", salt: hex(V.hkdf[0].salt), info: hex(V.hkdf[0].info)}, ikm, {name: "HMAC", hash: "SHA-256", length: 336}, true, ["sign"]);
	assert.sameValue(mac.algorithm.name, "HMAC");
	assert.sameValue(mac.algorithm.length, 336);
	assert.sameValue(toHex(await subtle.exportKey("raw", mac)), V.hkdf[0].out, "derived HMAC key bytes");
	const mac2 = await subtle.deriveKey({name: "HKDF", hash: "SHA-256", salt: hex(V.hkdf[0].salt), info: hex(V.hkdf[0].info)}, ikm, {name: "HMAC", hash: "SHA-256"}, false, ["sign"]);
	assert.sameValue(mac2.algorithm.length, 512, "HMAC default length");

	// errors
	const salt = hex(V.pbkdf2[1].salt);
	await assertRejects(subtle.deriveBits({name: "PBKDF2", hash: "SHA-256", salt, iterations: 1000}, password, 100), "OperationError", "length not multiple of 8");
	assert.sameValue((await subtle.deriveBits({name: "PBKDF2", hash: "SHA-256", salt, iterations: 1000}, password, 0)).byteLength, 0, "length 0 derives nothing");
	await assertRejects(subtle.deriveBits({name: "PBKDF2", hash: "SHA-256", salt, iterations: 1000}, password, -8), TypeError, "length -8");
	await assertRejects(subtle.deriveBits({name: "PBKDF2", hash: "SHA-256", salt, iterations: 1000}, password, Infinity), TypeError, "length Infinity");
	await assertRejects(subtle.deriveBits({name: "PBKDF2", hash: "SHA-256", salt, iterations: 1000}, password, null), "OperationError", "length null");
	await assertRejects(subtle.deriveBits({name: "PBKDF2", hash: "SHA-256", salt, iterations: 0}, password, 256), "OperationError", "iterations 0");
	await assertRejects(subtle.deriveBits({name: "PBKDF2", hash: "SHA-256", iterations: 1}, password, 256), TypeError, "salt missing");
	await assertRejects(subtle.deriveBits({name: "PBKDF2", salt, iterations: 1}, password, 256), TypeError, "hash missing");
	await assertRejects(subtle.deriveBits({name: "HKDF", hash: "SHA-256", salt, info: salt}, password, 256), "InvalidAccessError", "PBKDF2 key with HKDF");
	await assertRejects(subtle.deriveBits({name: "HKDF", hash: "SHA-256", salt, info: salt}, ikm, 256 * 8 * 32), "OperationError", "HKDF too long");
	const keyOnly = await subtle.importKey("raw", hex(V.pbkdf2[1].password), "PBKDF2", false, ["deriveKey"]);
	await assertRejects(subtle.deriveBits({name: "PBKDF2", hash: "SHA-256", salt, iterations: 1}, keyOnly, 256), "InvalidAccessError", "no deriveBits usage");
	const bitsOnly = await subtle.importKey("raw", hex(V.hkdf[0].ikm), "HKDF", false, ["deriveBits"]);
	await assertRejects(subtle.deriveKey({name: "HKDF", hash: "SHA-256", salt, info: salt}, bitsOnly, {name: "AES-GCM", length: 128}, false, ["encrypt"]), "InvalidAccessError", "no deriveKey usage");
	await assertRejects(subtle.deriveKey({name: "HKDF", hash: "SHA-256", salt, info: salt}, ikm, {name: "AES-GCM", length: 100}, false, ["encrypt"]), "OperationError", "bad derived length");
	await assertRejects(subtle.deriveKey({name: "HKDF", hash: "SHA-256", salt, info: salt}, ikm, {name: "HKDF"}, false, ["deriveBits"]), "OperationError", "derive HKDF key");
})().then(() => $DONE(), $DONE);
