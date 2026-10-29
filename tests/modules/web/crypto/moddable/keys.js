/*---
description:
flags: [async, module]
---*/

import crypto, {subtle, CryptoKey, V, hex, toHex, assertRejects} from "./webcrypto_FIXTURE.js";

(async () => {
	// generateKey
	for (const name of ["AES-CBC", "AES-CTR", "AES-GCM"]) {
		for (const length of [128, 192, 256]) {
			const key = await subtle.generateKey({name, length}, true, ["encrypt", "decrypt"]);
			assert.sameValue(key instanceof CryptoKey, true);
			assert.sameValue(key.type, "secret");
			assert.sameValue(key.extractable, true);
			assert.sameValue(key.algorithm.name, name);
			assert.sameValue(key.algorithm.length, length);
			assert.compareArray(key.usages, ["encrypt", "decrypt"]);
			const raw = await subtle.exportKey("raw", key);
			assert.sameValue(raw.byteLength, length / 8, `${name} ${length} raw length`);
			const other = await subtle.generateKey({name, length}, true, ["encrypt"]);
			assert.notSameValue(toHex(raw), toHex(await subtle.exportKey("raw", other)), "two keys differ");
		}
	}
	const kw = await subtle.generateKey({name: "AES-KW", length: 128}, false, ["wrapKey", "unwrapKey"]);
	assert.sameValue(kw.extractable, false);
	const hmac = await subtle.generateKey({name: "HMAC", hash: "SHA-256"}, true, ["sign", "verify"]);
	assert.sameValue(hmac.algorithm.length, 512, "HMAC default length is the block size");
	assert.sameValue((await subtle.exportKey("raw", hmac)).byteLength, 64);
	const hmac384 = await subtle.generateKey({name: "HMAC", hash: "SHA-384", length: 256}, true, ["sign"]);
	assert.sameValue((await subtle.exportKey("raw", hmac384)).byteLength, 32);

	await assertRejects(subtle.generateKey({name: "AES-GCM", length: 64}, true, ["encrypt"]), "OperationError", "AES length 64");
	await assertRejects(subtle.generateKey({name: "AES-GCM", length: 128}, true, []), "SyntaxError", "empty usages");
	await assertRejects(subtle.generateKey({name: "AES-GCM", length: 128}, true, ["sign"]), "SyntaxError", "sign on AES");
	await assertRejects(subtle.generateKey({name: "AES-KW", length: 128}, true, ["encrypt"]), "SyntaxError", "encrypt on AES-KW");
	await assertRejects(subtle.generateKey({name: "HMAC"}, true, ["sign"]), TypeError, "HMAC without hash");
	await assertRejects(subtle.generateKey({name: "RSA-OAEP", modulusLength: 2048}, true, ["encrypt"]), "NotSupportedError", "RSA");
	await assertRejects(subtle.generateKey({name: "PBKDF2"}, false, ["deriveBits"]), "NotSupportedError", "generate PBKDF2");

	// export / import, raw and jwk
	const raw = hex(V.jwk.key256);
	const key = await subtle.importKey("raw", raw, {name: "AES-GCM"}, true, ["encrypt", "decrypt", "wrapKey"]);
	assert.sameValue(toHex(await subtle.exportKey("raw", key)), V.jwk.key256);
	const jwk = await subtle.exportKey("jwk", key);
	assert.sameValue(jwk.kty, "oct");
	assert.sameValue(jwk.k, V.jwk.k256);
	assert.sameValue(jwk.alg, "A256GCM");
	assert.sameValue(jwk.ext, true);
	assert.compareArray(jwk.key_ops, ["encrypt", "decrypt", "wrapKey"]);
	const fromJwk = await subtle.importKey("jwk", {kty: "oct", k: V.jwk.k128, alg: "A128CBC"}, "AES-CBC", true, ["encrypt"]);
	assert.sameValue(toHex(await subtle.exportKey("raw", fromJwk)), V.jwk.key128, "jwk import");
	assert.sameValue(fromJwk.algorithm.length, 128);
	const minimal = await subtle.importKey("jwk", {kty: "oct", k: V.jwk.k128}, "AES-KW", false, ["wrapKey"]);
	assert.sameValue(minimal.algorithm.name, "AES-KW");

	const secret = await subtle.importKey("raw", raw, "AES-GCM", false, ["encrypt"]);
	await assertRejects(subtle.exportKey("raw", secret), "InvalidAccessError", "not extractable raw");
	await assertRejects(subtle.exportKey("jwk", secret), "InvalidAccessError", "not extractable jwk");
	await assertRejects(subtle.exportKey("pkcs8", key), "NotSupportedError", "pkcs8");
	await assertRejects(subtle.importKey("raw", new Uint8Array(15), "AES-GCM", false, ["encrypt"]), "DataError", "15-byte AES key");
	await assertRejects(subtle.importKey("raw", raw, "AES-GCM", false, ["sign"]), "SyntaxError", "sign on AES");
	await assertRejects(subtle.importKey("spki", raw, "AES-GCM", false, ["encrypt"]), "NotSupportedError", "spki");
	await assertRejects(subtle.importKey("raw", raw, "Ed25519", false, ["deriveBits"]), "NotSupportedError", "Ed25519");
	await assertRejects(subtle.importKey("jwk", {kty: "RSA"}, "AES-GCM", true, ["encrypt"]), "DataError", "jwk kty");
	await assertRejects(subtle.importKey("jwk", {kty: "oct", k: V.jwk.k128, alg: "A128GCM"}, "AES-CBC", true, ["encrypt"]), "DataError", "jwk alg mismatch");
	await assertRejects(subtle.importKey("jwk", {kty: "oct", k: V.jwk.k128, use: "sig"}, "AES-CBC", true, ["encrypt"]), "DataError", "jwk use");
	await assertRejects(subtle.importKey("jwk", {kty: "oct", k: V.jwk.k128, ext: false}, "AES-CBC", true, ["encrypt"]), "DataError", "jwk ext false but extractable");
	await assertRejects(subtle.importKey("jwk", {kty: "oct", k: "***"}, "AES-CBC", true, ["encrypt"]), "DataError", "jwk bad base64url");
	await assertRejects(subtle.importKey("raw", raw, "PBKDF2", true, ["deriveBits"]), "SyntaxError", "PBKDF2 extractable");
	await assertRejects(subtle.importKey("jwk", {kty: "oct", k: V.jwk.k128}, "HKDF", false, ["deriveBits"]), "NotSupportedError", "HKDF jwk");
	const pbkdf2 = await subtle.importKey("raw", raw, "PBKDF2", false, ["deriveBits", "deriveKey"]);
	assert.sameValue(pbkdf2.algorithm.name, "PBKDF2");
	await assertRejects(subtle.exportKey("raw", pbkdf2), "NotSupportedError", "PBKDF2 export");

	// key objects are frozen views of their state
	assert.sameValue(Object.isFrozen(key.algorithm), true, "algorithm frozen");
	assert.sameValue(Object.isFrozen(key.usages), true, "usages frozen");
	assert.throws(TypeError, () => new CryptoKey(), "CryptoKey is not constructible with no state");
})().then(() => $DONE(), $DONE);
