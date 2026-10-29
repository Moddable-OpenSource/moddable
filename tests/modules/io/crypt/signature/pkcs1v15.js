/*---
description:
flags: [module]
---*/

import Signature, {V, hex, toHex, publicKey, privateKey} from "./signature_FIXTURE.js";

using pub = publicKey();
using priv = privateKey();
for (const vec of V.pkcs1v15_sign) {
	using s = new Signature({algorithm: "RSASSA-PKCS1-v1_5", key: priv, hash: vec.hash});
	assert.sameValue(s.outputSize, 256, "size");
	assert.sameValue(toHex(s.sign(hex(vec.digest))), vec.signature, `sign ${vec.hash}`);
	using v = new Signature({algorithm: "RSASSA-PKCS1-v1_5", key: pub, hash: vec.hash});
	assert.sameValue(v.verify(hex(vec.digest), hex(vec.signature)), true, `verify ${vec.hash}`);
	assert.sameValue(s.verify(hex(vec.digest), hex(vec.signature)), true, "private key verifies too");
	const bad = hex(vec.signature); bad[100] ^= 1;
	assert.sameValue(v.verify(hex(vec.digest), bad), false, "tampered signature");
	const other = hex(vec.digest); other[0] ^= 1;
	assert.sameValue(v.verify(other, hex(vec.signature)), false, "other digest");
	assert.sameValue(v.verify(hex(vec.digest), hex(vec.signature).subarray(1)), false, "short signature");
	assert.sameValue(v.verify(hex(vec.digest), new Uint8Array(256).fill(0xff)), false, "signature above the modulus");
}
// wrong hash for the digest length
{
	using s = new Signature({algorithm: "RSASSA-PKCS1-v1_5", key: priv, hash: "SHA-256"});
	assert.throws(RangeError, () => s.sign(new Uint8Array(20)), "digest length mismatch");
	assert.sameValue(s.verify(new Uint8Array(20), hex(V.pkcs1v15_sign[1].signature)), false, "verify with wrong digest length");
}
// 1024-bit key, e = 3
{
	using k = new (await import("embedded:crypt/key")).default({algorithm: "RSA", format: "pkcs8", data: hex(V.key1024.pkcs8)});
	using s = new Signature({algorithm: "RSASSA-PKCS1-v1_5", key: k, hash: "SHA-256"});
	assert.sameValue(toHex(s.sign(hex(V.pkcs1v15_sign[1].digest))), V.key1024.pkcs1v15_sign_sha256, "1024-bit sign");
}
