/*---
description:
flags: [module]
---*/

import Signature, {V, hex, toHex, publicKey, privateKey, pssAnySalt} from "./signature_FIXTURE.js";

using pub = publicKey();
using priv = privateKey();
for (const vec of V.pss_sign) {
	using v = new Signature({algorithm: "RSA-PSS", key: pub, hash: vec.hash, saltLength: vec.saltLength});
	assert.sameValue(v.verify(hex(vec.digest), hex(vec.signature)), true, `verify PSS ${vec.hash}`);
	const bad = hex(vec.signature); bad[7] ^= 1;
	assert.sameValue(v.verify(hex(vec.digest), bad), false, "tampered");
	if (Signature.supports({algorithm: "RSA-PSS", hash: vec.hash, saltLength: vec.saltLength + 1})) {
		using wrongSalt = new Signature({algorithm: "RSA-PSS", key: pub, hash: vec.hash, saltLength: vec.saltLength + 1});
		assert.sameValue(wrongSalt.verify(hex(vec.digest), hex(vec.signature)), false, "salt length mismatch");
	}
	else {		// a backend with a fixed salt length: SecKey rejects the option, PSA verifies any salt length
		let wrongSalt;
		try {
			wrongSalt = new Signature({algorithm: "RSA-PSS", key: pub, hash: vec.hash, saltLength: vec.saltLength + 1});
		}
		catch (e) {
			assert.sameValue(e instanceof RangeError, true, "an unsupported saltLength is a RangeError");
		}
		if (wrongSalt) {
			assert.sameValue(wrongSalt.verify(hex(vec.digest), hex(vec.signature)), true, "any salt length verifies on this backend");
			wrongSalt.close();
		}
	}
	// default saltLength is the hash length
	using d = new Signature({algorithm: "RSA-PSS", key: pub, hash: vec.hash});
	assert.sameValue(d.verify(hex(vec.digest), hex(vec.signature)), true, "default salt length");
	// saltLength is coerced, not type-checked
	using coerced = new Signature({algorithm: "RSA-PSS", key: pub, hash: vec.hash, saltLength: String(vec.saltLength)});
	assert.sameValue(coerced.verify(hex(vec.digest), hex(vec.signature)), true, "saltLength coerced from a string");
	// sign then verify: PSS signatures are randomized
	using s = new Signature({algorithm: "RSA-PSS", key: priv, hash: vec.hash});
	const sig1 = s.sign(hex(vec.digest)), sig2 = s.sign(hex(vec.digest));
	assert.notSameValue(toHex(sig1), toHex(sig2), "randomized");
	assert.sameValue(v.verify(hex(vec.digest), sig1) && v.verify(hex(vec.digest), sig2), true, "round trip");
}
{
	const digest = hex(V.pss_sign[1].digest);
	if (pssAnySalt) {
		using zero = new Signature({algorithm: "RSA-PSS", key: priv, hash: "SHA-256", saltLength: 0});
		const sig = zero.sign(digest);
		assert.sameValue(toHex(zero.sign(digest)), toHex(sig), "salt 0 is deterministic");
		assert.sameValue(zero.verify(digest, sig), true, "salt 0 verifies");
	}
	else		// construction or signing rejects the salt length
		assert.throws(RangeError, () => new Signature({algorithm: "RSA-PSS", key: priv, hash: "SHA-256", saltLength: 0}).sign(digest), "this backend signs with a salt of the hash length only");
	assert.throws(RangeError, () => new Signature({algorithm: "RSA-PSS", key: priv, hash: "SHA-256", saltLength: -1}), "negative salt");
	assert.throws(RangeError, () => new Signature({algorithm: "RSA-PSS", key: priv, hash: "SHA-256", saltLength: 300}).sign(digest), "salt too long for key");
}
