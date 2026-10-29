/*---
description:
flags: [async, module]
---*/

import {subtle, V, hex, toHex, assertRejects} from "./webcrypto_FIXTURE.js";

(async () => {
	for (const vec of V.aeskw) {
		const kek = await subtle.importKey("raw", hex(vec.kek), "AES-KW", false, ["wrapKey", "unwrapKey"]);
		const key = await subtle.importKey("raw", hex(vec.key), "AES-GCM", true, ["encrypt"]);
		const wrapped = await subtle.wrapKey("raw", key, kek, "AES-KW");
		assert.sameValue(toHex(wrapped), vec.wrapped, `AES-KW ${vec.kek.length * 4}/${vec.key.length * 4}`);
		const unwrapped = await subtle.unwrapKey("raw", wrapped, kek, "AES-KW", "AES-GCM", true, ["encrypt"]);
		assert.sameValue(toHex(await subtle.exportKey("raw", unwrapped)), vec.key, "unwrap");
		assert.sameValue(unwrapped.algorithm.name, "AES-GCM");
		const tampered = hex(vec.wrapped); tampered[9] ^= 1;
		await assertRejects(subtle.unwrapKey("raw", tampered, kek, "AES-KW", "AES-GCM", true, ["encrypt"]), "OperationError", "tampered wrap");
	}

	// jwk wrapped with AES-GCM
	const wrapping = await subtle.generateKey({name: "AES-GCM", length: 256}, false, ["wrapKey", "unwrapKey"]);
	const hmac = await subtle.importKey("raw", hex(V.hmac[0].key), {name: "HMAC", hash: V.hmac[0].hash}, true, ["sign", "verify"]);
	const iv = hex(V.gcm[0].iv);
	const wrapped = await subtle.wrapKey("jwk", hmac, wrapping, {name: "AES-GCM", iv});
	const unwrapped = await subtle.unwrapKey("jwk", wrapped, wrapping, {name: "AES-GCM", iv}, {name: "HMAC", hash: V.hmac[0].hash}, false, ["sign"]);
	assert.sameValue(unwrapped.algorithm.hash.name, V.hmac[0].hash);
	assert.sameValue(unwrapped.extractable, false);
	assert.compareArray(unwrapped.usages, ["sign"]);
	assert.sameValue(toHex(await subtle.sign("HMAC", unwrapped, hex(V.hmac[0].data))), V.hmac[0].mac, "unwrapped HMAC key works");

	// errors
	const kek = await subtle.importKey("raw", hex(V.aeskw[0].kek), "AES-KW", false, ["wrapKey", "unwrapKey"]);
	const secret = await subtle.importKey("raw", hex(V.aeskw[0].key), "AES-GCM", false, ["encrypt"]);
	await assertRejects(subtle.wrapKey("raw", secret, kek, "AES-KW"), "InvalidAccessError", "wrap non-extractable");
	const encryptOnly = await subtle.importKey("raw", hex(V.aeskw[0].kek), "AES-GCM", false, ["encrypt"]);
	await assertRejects(subtle.wrapKey("raw", hmac, encryptOnly, {name: "AES-GCM", iv}), "InvalidAccessError", "no wrapKey usage");
	await assertRejects(subtle.wrapKey("raw", hmac, kek, {name: "AES-GCM", iv}), "InvalidAccessError", "AES-KW key with AES-GCM");
	await assertRejects(subtle.unwrapKey("raw", new Uint8Array(20), kek, "AES-KW", "AES-GCM", true, ["encrypt"]), "OperationError", "AES-KW bad length");
	const short = await subtle.importKey("raw", new Uint8Array(16), {name: "HMAC", hash: "SHA-256"}, true, ["sign"]);
	await assertRejects(subtle.wrapKey("raw", await subtle.importKey("raw", new Uint8Array(12), {name: "HMAC", hash: "SHA-256"}, true, ["sign"]), kek, "AES-KW"), "OperationError", "AES-KW needs a multiple of 8");
	assert.sameValue((await subtle.wrapKey("raw", short, kek, "AES-KW")).byteLength, 24, "16-byte HMAC key wraps");
})().then(() => $DONE(), $DONE);
