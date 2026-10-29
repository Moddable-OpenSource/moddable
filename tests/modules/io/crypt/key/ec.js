/*---
description: EC keys: every encoding round-trips, raw points, generation
flags: [module]
---*/

import Key, {V, hex, toHex} from "./ec_FIXTURE.js";

for (const name of ["P-256", "P-384", "X25519"]) {
	const v = V[name];
	// public: spki, raw, jwk
	using pub = new Key({algorithm: "EC", format: "spki", data: hex(v.spki), extractable: true});
	assert.sameValue(pub.algorithm, "EC");
	assert.sameValue(pub.type, "public");
	assert.sameValue(pub.namedCurve, name);
	assert.sameValue(pub.keySize, ("X25519" === name) ? 255 : v.bytes * 8, `${name} size`);
	assert.sameValue(toHex(pub.extract("spki")), v.spki, `${name} spki round trip`);
	assert.sameValue(toHex(pub.extract("raw")), v.raw, `${name} raw extract`);
	const jwk = pub.extract("jwk");
	assert.sameValue(jwk.kty, v.jwk_public.kty);
	assert.sameValue(jwk.crv, name);
	assert.sameValue(jwk.x, v.jwk_public.x, `${name} jwk x`);
	if ("X25519" !== name)
		assert.sameValue(jwk.y, v.jwk_public.y, `${name} jwk y`);
	assert.sameValue("d" in jwk, false);
	using raw = new Key({algorithm: "EC", format: "raw", namedCurve: name, data: hex(v.raw), extractable: true});
	assert.sameValue(toHex(raw.extract("spki")), v.spki, `${name} raw import`);
	using fromJwk = new Key({algorithm: "EC", format: "jwk", data: v.jwk_public, extractable: true});
	assert.sameValue(toHex(fromJwk.extract("raw")), v.raw, `${name} jwk import`);
	assert.throws(RangeError, () => pub.extract("pkcs8"), "public key has no pkcs8");
	assert.throws(RangeError, () => pub.extract("pkcs1"), "pkcs1 is not an EC format");
	// private: pkcs8, jwk (and sec1 for the Weierstrass curves)
	using priv = new Key({algorithm: "EC", format: "pkcs8", data: hex(v.pkcs8), extractable: true});
	assert.sameValue(priv.type, "private");
	assert.sameValue(toHex(priv.extract("pkcs8")), v.pkcs8, `${name} pkcs8 round trip`);
	assert.sameValue(toHex(priv.extract("spki")), v.spki, `${name} spki of a private key`);
	assert.sameValue(toHex(priv.extract("raw")), v.raw, `${name} raw of a private key`);
	const pjwk = priv.extract("jwk");
	assert.sameValue(pjwk.d, v.jwk_private.d, `${name} jwk d`);
	assert.sameValue(pjwk.x, v.jwk_private.x, `${name} jwk private x`);
	using again = new Key({algorithm: "EC", format: "jwk", data: pjwk, extractable: true});
	assert.sameValue(toHex(again.extract("pkcs8")), v.pkcs8, `${name} jwk private re-import`);
	if ("X25519" !== name) {
		assert.sameValue(toHex(priv.extract("sec1")), v.sec1, `${name} sec1 extract`);
		using sec1 = new Key({algorithm: "EC", format: "sec1", data: hex(v.sec1), extractable: true});
		assert.sameValue(toHex(sec1.extract("pkcs8")), v.pkcs8, `${name} sec1 import`);
	}
	else
		assert.throws(RangeError, () => priv.extract("sec1"), "X25519 has no sec1 form");
	using secret = new Key({algorithm: "EC", format: "pkcs8", data: hex(v.pkcs8)});
	assert.throws(Error, () => secret.extract("pkcs8"), "not extractable");
	assert.throws(Error, () => secret.extract("raw"), "not extractable: the public point too");
	assert.throws(RangeError, () => new Key({algorithm: "EC", format: "spki", namedCurve: ("P-256" === name) ? "P-384" : "P-256", data: hex(v.spki)}), "namedCurve mismatch");
	// generation
	using generated = new Key({algorithm: "EC", namedCurve: name, extractable: true});
	assert.sameValue(generated.type, "private");
	assert.sameValue(generated.namedCurve, name);
	const point = generated.extract("raw");
	assert.sameValue(point.byteLength, ("X25519" === name) ? 32 : 1 + 2 * v.bytes, `${name} generated point length`);
	using reimported = new Key({algorithm: "EC", format: "jwk", data: generated.extract("jwk"), extractable: true});
	assert.sameValue(toHex(reimported.extract("raw")), toHex(point), `${name} generated key round trips through jwk`);
	using other = new Key({algorithm: "EC", namedCurve: name, extractable: true});
	assert.notSameValue(toHex(other.extract("raw")), toHex(point), `${name} generated keys differ`);
}
assert.throws(RangeError, () => new Key({algorithm: "EC", namedCurve: "P-521"}), "unsupported curve");
for (const name of ["P-256", "P-384"]) {		// compressed points decompress to the fixture's point
	const v = V[name], raw = hex(v.raw);
	const compressed = new Uint8Array(1 + v.bytes);
	compressed[0] = 2 + (raw[raw.byteLength - 1] & 1);
	compressed.set(raw.subarray(1, 1 + v.bytes), 1);
	using k = new Key({algorithm: "EC", format: "raw", namedCurve: name, data: compressed, extractable: true});
	assert.sameValue(toHex(k.extract("raw")), v.raw, `${name} compressed point`);
	const jwk = {...v.jwk_private, d: v.jwk_private.d.slice(2)};
	assert.throws(RangeError, () => new Key({algorithm: "EC", format: "jwk", data: jwk}), `${name} jwk d of the wrong length`);
	const mismatched = {...v.jwk_private, x: v.jwk_public.x.replace(/^./, c => c === "A" ? "B" : "A")};
	assert.throws(RangeError, () => new Key({algorithm: "EC", format: "jwk", data: mismatched}), `${name} jwk pair mismatch`);
}
assert.throws(RangeError, () => new Key({algorithm: "EC", format: "jwk", data: {...V.X25519.jwk_private, x: V.X25519.jwk_public.x.replace(/^./, c => c === "A" ? "B" : "A")}}), "X25519 jwk pair mismatch");
assert.throws(RangeError, () => new Key({algorithm: "EC", format: "raw", namedCurve: "P-256", data: new Uint8Array(65)}), "bad point prefix");
assert.throws(RangeError, () => new Key({algorithm: "RSA", format: "raw", data: new Uint8Array(65)}), "raw is not an RSA format");
