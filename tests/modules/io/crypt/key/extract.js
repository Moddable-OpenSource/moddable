/*---
description:
flags: [module]
---*/

import Key, {V, hex, toHex, publicKey, privateKey} from "./rsa_FIXTURE.js";

{
	using pub = publicKey();
	assert.sameValue(toHex(pub.extract("spki")), V.key.spki, "spki round trip");
	assert.sameValue(toHex(pub.extract("pkcs1")), V.key.pkcs1_public, "pkcs1 public");
	const jwk = pub.extract("jwk");
	assert.sameValue(jwk.kty, "RSA");
	assert.sameValue(jwk.n, V.key.jwk_public.n, "jwk n");
	assert.sameValue(jwk.e, V.key.jwk_public.e, "jwk e");
	assert.sameValue("d" in jwk, false, "no private members");
	assert.throws(RangeError, () => pub.extract("pkcs8"), "public key has no pkcs8");
	assert.throws(RangeError, () => pub.extract("pem"), "unsupported format");
}
{
	using priv = privateKey();
	assert.sameValue(toHex(priv.extract("pkcs8")), V.key.pkcs8, "pkcs8 round trip");
	assert.sameValue(toHex(priv.extract("pkcs1")), V.key.pkcs1_private, "pkcs1 private");
	assert.sameValue(toHex(priv.extract("spki")), V.key.spki, "spki of a private key is its public key");
	const jwk = priv.extract("jwk");
	for (const name of ["n", "e", "d", "p", "q", "dp", "dq", "qi"])
		assert.sameValue(jwk[name], V.key.jwk_private[name], `jwk ${name}`);
	using again = new Key({algorithm: "RSA", format: "jwk", data: jwk, extractable: true});
	assert.sameValue(toHex(again.extract("pkcs1")), V.key.pkcs1_private, "jwk extract re-imports");
}
{
	using secret = privateKey({extractable: false});
	assert.sameValue(secret.extractable, false);
	assert.throws(Error, () => secret.extract("pkcs8"), "not extractable: pkcs8");
	assert.throws(Error, () => secret.extract("pkcs1"), "not extractable: pkcs1");
	assert.throws(Error, () => secret.extract("jwk"), "not extractable: jwk");
	assert.throws(Error, () => secret.extract("spki"), "not extractable: spki, the public part too");
	using hidden = publicKey({extractable: false});
	assert.sameValue(hidden.extractable, false);
	assert.throws(Error, () => hidden.extract("spki"), "not extractable: a public key");
	using unstated = new Key({algorithm: "RSA", format: "spki", data: hex(V.key.spki)});
	assert.sameValue(unstated.extractable, false, "extractable defaults to false for a public key");
}
{
	using small = new Key({algorithm: "RSA", format: "pkcs8", data: hex(V.key1024.pkcs8), extractable: true});
	assert.sameValue(toHex(small.extract("pkcs8")), V.key1024.pkcs8, "1024-bit pkcs8 round trip");
	assert.sameValue(small.extract("jwk").e, "Aw", "e = 3");
}
