/*---
description:
flags: [module]
---*/

import Signature, {V, hex, publicKey, privateKey} from "./signature_FIXTURE.js";

function throwsAnything(f, message) {
	try { f(); } catch { return; }
	throw new Test262Error(`${message}: did not throw`);
}

using pub = publicKey();
using priv = privateKey();
const digest = hex(V.pkcs1v15_sign[1].digest);
assert.throws(Error, () => new Signature({algorithm: "DSA", key: priv, hash: "SHA-256"}), "unsupported algorithm");
assert.throws(TypeError, () => new Signature({algorithm: "ECDSA", key: priv}), "ECDSA needs an EC key");
assert.throws(TypeError, () => new Signature({algorithm: "RSASSA-PKCS1-v1_5", key: hex(V.key.spki), hash: "SHA-256"}), "key must be a Key");
assert.throws(RangeError, () => new Signature({algorithm: "RSASSA-PKCS1-v1_5", key: priv, hash: "SHA3-256"}), "unsupported hash");
assert.throws(RangeError, () => new Signature({algorithm: "RSASSA-PKCS1-v1_5", key: priv}), "hash required");
assert.throws(RangeError, () => new Signature({algorithm: "RSASSA-PKCS1-v1_5", key: priv, hash: "SHA-256", saltLength: 32}), "saltLength is RSA-PSS's");
{
	using s = new Signature({algorithm: "RSASSA-PKCS1-v1_5", key: pub, hash: "SHA-256"});
	assert.throws(Error, () => s.sign(digest), "sign with a public key");
	assert.throws(TypeError, () => s.verify("abc", new Uint8Array(256)), "digest string");
	assert.throws(TypeError, () => s.verify(digest, 42), "signature number");
}
{
	const s = new Signature({algorithm: "RSASSA-PKCS1-v1_5", key: priv, hash: "SHA-256"});
	s.close();
	s.close();
	// the error class after close depends on the backend: Error from the script implementation, SyntaxError from the host data validator
	throwsAnything(() => s.sign(digest), "sign after close");
	throwsAnything(() => s.verify(digest, new Uint8Array(256)), "verify after close");
	assert.sameValue(s.algorithm, "RSASSA-PKCS1-v1_5", "algorithm after close");
}
{
	const k = privateKey();
	const s = new Signature({algorithm: "RSASSA-PKCS1-v1_5", key: k, hash: "SHA-256"});
	k.close();
	assert.throws(Error, () => s.sign(digest), "key closed under the signature");
	assert.throws(Error, () => new Signature({algorithm: "RSASSA-PKCS1-v1_5", key: k, hash: "SHA-256"}), "closed key");
}
