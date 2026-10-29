/*---
description:
flags: [async, module]
---*/

import {subtle, hex, toHex, assertRejects} from "./webcrypto_FIXTURE.js";
import Cipher from "embedded:crypt/cipher";
import Signature from "embedded:crypt/signature";
import {V as R} from "../../../io/crypt/key/rsa_FIXTURE.js";
import TextEncoder from "text/encoder";

const message = hex(R.message);
(async () => {
	// import in every format, properties
	const pub = await subtle.importKey("spki", hex(R.key.spki), {name: "RSASSA-PKCS1-v1_5", hash: "SHA-256"}, true, ["verify"]);
	assert.sameValue(pub.type, "public");
	assert.sameValue(pub.algorithm.name, "RSASSA-PKCS1-v1_5");
	assert.sameValue(pub.algorithm.modulusLength, 2048);
	assert.sameValue(toHex(pub.algorithm.publicExponent), "010001");
	assert.sameValue(pub.algorithm.hash.name, "SHA-256");
	const priv = await subtle.importKey("pkcs8", hex(R.key.pkcs8), {name: "RSASSA-PKCS1-v1_5", hash: "SHA-256"}, true, ["sign"]);
	assert.sameValue(priv.type, "private");
	const jwkPub = await subtle.importKey("jwk", {...R.key.jwk_public, alg: "RS256"}, {name: "RSASSA-PKCS1-v1_5", hash: "SHA-256"}, true, ["verify"]);
	const jwkPriv = await subtle.importKey("jwk", R.key.jwk_private, {name: "RSASSA-PKCS1-v1_5", hash: "SHA-256"}, true, ["sign"]);

	// export round trips
	assert.sameValue(toHex(await subtle.exportKey("spki", pub)), R.key.spki, "spki export");
	assert.sameValue(toHex(await subtle.exportKey("pkcs8", priv)), R.key.pkcs8, "pkcs8 export");
	const jwk = await subtle.exportKey("jwk", priv);
	assert.sameValue(jwk.alg, "RS256");
	assert.sameValue(jwk.n, R.key.jwk_private.n);
	assert.sameValue(jwk.qi, R.key.jwk_private.qi);
	assert.compareArray(jwk.key_ops, ["sign"]);
	assert.sameValue((await subtle.exportKey("jwk", pub)).d, undefined, "public jwk has no d");

	// RSASSA-PKCS1-v1_5 sign / verify against Python
	for (const vec of R.pkcs1v15_sign) {
		const k = await subtle.importKey("pkcs8", hex(R.key.pkcs8), {name: "RSASSA-PKCS1-v1_5", hash: vec.hash}, false, ["sign"]);
		assert.sameValue(toHex(await subtle.sign("RSASSA-PKCS1-v1_5", k, message)), vec.signature, `v1.5 sign ${vec.hash}`);
		const p = await subtle.importKey("spki", hex(R.key.spki), {name: "RSASSA-PKCS1-v1_5", hash: vec.hash}, false, ["verify"]);
		assert.sameValue(await subtle.verify("RSASSA-PKCS1-v1_5", p, hex(vec.signature), message), true, `v1.5 verify ${vec.hash}`);
		const bad = hex(vec.signature); bad[5] ^= 1;
		assert.sameValue(await subtle.verify("RSASSA-PKCS1-v1_5", p, bad, message), false, "tampered");
	}
	// RSA-PSS
	// the script backend honors any saltLength; the PSA and SecKey backends sign with a salt of the hash length only (PSA verifies any)
	const hashLengths = {"SHA-1": 20, "SHA-256": 32, "SHA-384": 48, "SHA-512": 64};
	for (const vec of R.pss_sign) {
		const p = await subtle.importKey("spki", hex(R.key.spki), {name: "RSA-PSS", hash: vec.hash}, false, ["verify"]);
		assert.sameValue(await subtle.verify({name: "RSA-PSS", saltLength: vec.saltLength}, p, hex(vec.signature), message), true, `PSS verify ${vec.hash}`);
		const k = await subtle.importKey("pkcs8", hex(R.key.pkcs8), {name: "RSA-PSS", hash: vec.hash}, false, ["sign"]);
		const saltLength = hashLengths[vec.hash];
		const sig = await subtle.sign({name: "RSA-PSS", saltLength}, k, message);
		assert.sameValue(await subtle.verify({name: "RSA-PSS", saltLength}, p, sig, message), true, "PSS round trip");
		if (Signature.supports({algorithm: "RSA-PSS", hash: vec.hash, saltLength: 0}))
			assert.sameValue(await subtle.verify({name: "RSA-PSS", saltLength: 0}, p, sig, message), false, "PSS salt mismatch");
		else {
			const outcome = await subtle.verify({name: "RSA-PSS", saltLength: 0}, p, sig, message).then(v => v, e => e.name);
			assert.sameValue(("boolean" === typeof outcome) || ("OperationError" === outcome), true, "a fixed salt length: verifies any salt (PSA) or cannot verify this one (SecKey)");
			const name = await subtle.sign({name: "RSA-PSS", saltLength: 0}, k, message).then(() => "resolved", e => e.name);
			assert.sameValue(name, "OperationError", "this backend signs with a salt of the hash length only");
		}
	}
	// RSA-OAEP
	for (const vec of R.oaep) {
		const k = await subtle.importKey("pkcs8", hex(R.key.pkcs8), {name: "RSA-OAEP", hash: vec.hash}, false, ["decrypt"]);
		const params = {name: "RSA-OAEP"};
		if (vec.label.length) params.label = hex(vec.label);
		if (vec.label.length && !Cipher.supports({algorithm: "RSA-OAEP", hash: vec.hash, label: params.label})) {		// SecKey's OAEP has no label
			await assertRejects(subtle.decrypt(params, k, hex(vec.ct)), "OperationError", `OAEP label unsupported ${vec.hash}`);
			continue;
		}
		assert.sameValue(toHex(await subtle.decrypt(params, k, hex(vec.ct))), vec.pt, `OAEP decrypt ${vec.hash}`);
		const p = await subtle.importKey("spki", hex(R.key.spki), {name: "RSA-OAEP", hash: vec.hash}, false, ["encrypt", "wrapKey"]);
		const ct = await subtle.encrypt(params, p, hex(vec.pt));
		assert.sameValue(toHex(await subtle.decrypt(params, k, ct)), vec.pt, "OAEP round trip");
	}
	// wrap an AES key with RSA-OAEP
	{
		const wrapper = await subtle.importKey("spki", hex(R.key.spki), {name: "RSA-OAEP", hash: "SHA-256"}, false, ["wrapKey"]);
		const unwrapper = await subtle.importKey("pkcs8", hex(R.key.pkcs8), {name: "RSA-OAEP", hash: "SHA-256"}, false, ["unwrapKey"]);
		const aes = await subtle.generateKey({name: "AES-GCM", length: 256}, true, ["encrypt", "decrypt"]);
		const wrapped = await subtle.wrapKey("raw", aes, wrapper, {name: "RSA-OAEP"});
		assert.sameValue(wrapped.byteLength, 256);
		const again = await subtle.unwrapKey("raw", wrapped, unwrapper, {name: "RSA-OAEP"}, {name: "AES-GCM"}, true, ["encrypt"]);
		assert.sameValue(toHex(await subtle.exportKey("raw", again)), toHex(await subtle.exportKey("raw", aes)), "wrapped key round trip");
	}
	// errors
	await assertRejects(subtle.generateKey({name: "RSA-OAEP", modulusLength: 2048, publicExponent: new Uint8Array([1, 0, 1]), hash: "SHA-256"}, true, ["encrypt", "decrypt"]), "NotSupportedError", "generateKey RSA");
	await assertRejects(subtle.sign("RSASSA-PKCS1-v1_5", pub, message), "InvalidAccessError", "sign with public key");
	await assertRejects(subtle.verify("RSASSA-PKCS1-v1_5", priv, new Uint8Array(256), message), "InvalidAccessError", "verify with private key");
	await assertRejects(subtle.sign("RSA-PSS", priv, message), "InvalidAccessError", "PSS with a v1.5 key");
	await assertRejects(subtle.importKey("spki", hex(R.key.spki), {name: "RSASSA-PKCS1-v1_5"}, true, ["verify"]), TypeError, "hash required");
	await assertRejects(subtle.importKey("spki", hex(R.key.spki), {name: "RSASSA-PKCS1-v1_5", hash: "SHA-256"}, true, ["sign"]), "SyntaxError", "sign on a public key");
	await assertRejects(subtle.importKey("pkcs8", hex(R.key.spki), {name: "RSASSA-PKCS1-v1_5", hash: "SHA-256"}, true, ["sign"]), "DataError", "spki as pkcs8");
	await assertRejects(subtle.importKey("jwk", {...R.key.jwk_public, alg: "RS512"}, {name: "RSASSA-PKCS1-v1_5", hash: "SHA-256"}, true, ["verify"]), "DataError", "jwk alg mismatch");
	await assertRejects(subtle.importKey("jwk", {...R.key.jwk_private, qi: undefined}, {name: "RSASSA-PKCS1-v1_5", hash: "SHA-256"}, true, ["sign"]), "DataError", "incomplete private jwk");
	await assertRejects(subtle.importKey("raw", hex(R.key.spki), {name: "RSASSA-PKCS1-v1_5", hash: "SHA-256"}, true, ["verify"]), "NotSupportedError", "raw RSA");
	const secret = await subtle.importKey("pkcs8", hex(R.key.pkcs8), {name: "RSA-OAEP", hash: "SHA-256"}, false, ["decrypt"]);
	await assertRejects(subtle.exportKey("pkcs8", secret), "InvalidAccessError", "not extractable");
	await assertRejects(subtle.exportKey("spki", secret), "InvalidAccessError", "not extractable (spki)");
	await assertRejects(subtle.exportKey("pkcs8", pub), "InvalidAccessError", "pkcs8 of a public key");
	await assertRejects(subtle.decrypt({name: "RSA-OAEP"}, secret, new Uint8Array(255)), "OperationError", "bad ciphertext length");
	const publicOnly = await subtle.importKey("spki", hex(R.key.spki), {name: "RSA-OAEP", hash: "SHA-256"}, true, []);
	assert.compareArray(publicOnly.usages, [], "public keys may have no usages");
})().then(() => $DONE(), $DONE);
