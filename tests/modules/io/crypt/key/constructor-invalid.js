/*---
description:
flags: [module]
---*/

import Key, {V, hex} from "./rsa_FIXTURE.js";

assert.throws(Error, () => new Key({algorithm: "DSA", format: "spki", data: hex(V.key.spki)}), "unsupported algorithm");
assert.throws(RangeError, () => new Key({algorithm: "EC", format: "spki", data: hex(V.key.spki)}), "an RSA key is not an EC key");
assert.throws(Error, () => new Key({format: "spki", data: hex(V.key.spki)}), "algorithm required");
assert.throws(RangeError, () => new Key({algorithm: "RSA", format: "pem", data: hex(V.key.spki)}), "unsupported format");
assert.throws(RangeError, () => new Key({algorithm: "RSA", format: "spki", data: hex(V.key.pkcs8)}), "pkcs8 bytes as spki");
assert.throws(RangeError, () => new Key({algorithm: "RSA", format: "pkcs8", data: hex(V.key.spki)}), "spki bytes as pkcs8");
assert.throws(RangeError, () => new Key({algorithm: "RSA", format: "pkcs1", data: hex(V.key.spki)}), "spki bytes as pkcs1");
assert.throws(TypeError, () => new Key({algorithm: "RSA", format: "spki", data: "not bytes"}), "string data");
assert.throws(RangeError, () => new Key({algorithm: "RSA", format: "jwk", data: hex(V.key.spki)}), "bytes as jwk: an object without kty");
assert.throws(RangeError, () => new Key({algorithm: "RSA", format: "jwk", data: {kty: "oct", k: "AAAA"}}), "jwk kty");
assert.throws(RangeError, () => new Key({algorithm: "RSA", format: "jwk", data: {kty: "RSA", n: V.key.jwk_public.n}}), "jwk e missing");
assert.throws(RangeError, () => new Key({algorithm: "RSA", format: "jwk", data: {...V.key.jwk_private, qi: undefined}}), "jwk private component missing");
// an EC SPKI (OID 1.2.840.10045.2.1) is not an RSA key
const ecSpki = hex("3059301306072a8648ce3d020106082a8648ce3d030107034200" + "04".padEnd(130, "1"));
assert.throws(RangeError, () => new Key({algorithm: "RSA", format: "spki", data: ecSpki}), "EC spki");
