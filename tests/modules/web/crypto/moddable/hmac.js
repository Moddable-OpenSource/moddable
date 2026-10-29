/*---
description:
flags: [async, module]
---*/

import {subtle, V, hex, toHex, assertRejects} from "./webcrypto_FIXTURE.js";

(async () => {
	for (const vec of V.hmac) {
		const key = await subtle.importKey("raw", hex(vec.key), {name: "HMAC", hash: vec.hash}, false, ["sign", "verify"]);
		assert.sameValue(key.type, "secret");
		assert.sameValue(key.algorithm.name, "HMAC");
		assert.sameValue(key.algorithm.hash.name, vec.hash);
		assert.sameValue(key.algorithm.length, vec.key.length * 4, "length in bits");
		const label = `HMAC-${vec.hash} key ${vec.key.length / 2}B`;
		assert.sameValue(toHex(await subtle.sign("HMAC", key, hex(vec.data))), vec.mac, label);
		assert.sameValue(await subtle.verify({name: "hmac"}, key, hex(vec.mac), hex(vec.data)), true, `${label} verify`);
		const bad = hex(vec.mac); bad[0] ^= 1;
		assert.sameValue(await subtle.verify("HMAC", key, bad, hex(vec.data)), false, `${label} verify tampered`);
		assert.sameValue(await subtle.verify("HMAC", key, hex(vec.mac).subarray(1), hex(vec.data)), false, `${label} verify short`);
	}

	// jwk round trip and length parameter
	const raw = hex(V.hmac[1].key);
	const key = await subtle.importKey("raw", raw, {name: "HMAC", hash: {name: "SHA-256"}}, true, ["sign"]);
	const jwk = await subtle.exportKey("jwk", key);
	assert.sameValue(jwk.kty, "oct");
	assert.sameValue(jwk.alg, "HS256");
	assert.sameValue(jwk.ext, true);
	assert.compareArray(jwk.key_ops, ["sign"]);
	assert.sameValue(toHex(Uint8Array.fromBase64(jwk.k, {alphabet: "base64url"})), toHex(raw), "jwk k");
	const again = await subtle.importKey("jwk", jwk, {name: "HMAC", hash: "SHA-256"}, true, ["sign"]);
	assert.sameValue(toHex(await subtle.exportKey("raw", again)), toHex(raw), "jwk import");
	const sized = await subtle.importKey("raw", raw, {name: "HMAC", hash: "SHA-256", length: 508}, false, ["sign"]);
	assert.sameValue(sized.algorithm.length, 508, "explicit length");
	await assertRejects(subtle.importKey("raw", raw, {name: "HMAC", hash: "SHA-256", length: 500}, false, ["sign"]), "DataError", "length more than 8 bits short");

	// errors
	const encKey = await subtle.importKey("raw", raw.subarray(0, 32), "AES-GCM", false, ["encrypt"]);
	await assertRejects(subtle.sign("HMAC", encKey, raw), "InvalidAccessError", "AES key for HMAC");
	const verifyOnly = await subtle.importKey("raw", raw, {name: "HMAC", hash: "SHA-256"}, false, ["verify"]);
	await assertRejects(subtle.sign("HMAC", verifyOnly, raw), "InvalidAccessError", "no sign usage");
	await assertRejects(subtle.importKey("raw", raw, {name: "HMAC"}, false, ["sign"]), TypeError, "hash required");
	await assertRejects(subtle.importKey("raw", raw, {name: "HMAC", hash: "SHA-256"}, false, ["encrypt"]), "SyntaxError", "bad usage");
	await assertRejects(subtle.importKey("raw", new Uint8Array(0), {name: "HMAC", hash: "SHA-256"}, false, ["sign"]), "DataError", "empty key");
	await assertRejects(subtle.importKey("raw", raw, {name: "HMAC", hash: "SHA-256", length: 1000}, false, ["sign"]), "DataError", "length too long");
	await assertRejects(subtle.importKey("jwk", {...jwk, alg: "HS512"}, {name: "HMAC", hash: "SHA-256"}, true, ["sign"]), "DataError", "jwk alg mismatch");
	await assertRejects(subtle.importKey("jwk", {...jwk, ext: false}, {name: "HMAC", hash: "SHA-256"}, true, ["sign"]), "DataError", "jwk ext false");
	await assertRejects(subtle.importKey("jwk", {...jwk, key_ops: ["verify"]}, {name: "HMAC", hash: "SHA-256"}, true, ["sign"]), "DataError", "jwk key_ops");
})().then(() => $DONE(), $DONE);
