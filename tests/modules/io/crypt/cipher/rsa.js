/*---
description:
flags: [module]
---*/

import Cipher from "./cipher_FIXTURE.js";
import Key, {V as R, hex, toHex, publicKey, privateKey} from "../key/rsa_FIXTURE.js";

using pub = publicKey();
using priv = privateKey();

// the direction is fixed at construction: the other method throws
{
	using e = new Cipher({algorithm: "RSAES-PKCS1-v1_5", key: pub, direction: "encrypt"});
	assert.throws(Error, () => e.decrypt(new Uint8Array(256)), "decrypt on an encrypting instance");
	using d = new Cipher({algorithm: "RSAES-PKCS1-v1_5", key: priv, direction: "decrypt"});
	assert.throws(Error, () => d.encrypt(new Uint8Array(8)), "encrypt on a decrypting instance");
}

// OAEP: decrypt Python ciphertexts, encrypt/decrypt round trips
for (const vec of R.oaep) {
	const options = {algorithm: "RSA-OAEP", hash: vec.hash};
	if (vec.label.length) options.label = hex(vec.label);
	if (!Cipher.supports(options)) {		// SecKey's OAEP has no label
		assert.throws(RangeError, () => new Cipher({...options, key: priv, direction: "decrypt"}), `OAEP label unsupported`);
		continue;
	}
	using d = new Cipher({...options, key: priv, direction: "decrypt"});
	assert.sameValue(d.blockSize, 256, "blockSize is the key size");
	assert.sameValue(d.keySize, 2048, "keySize in bits");
	assert.sameValue(d.tagLength, 0, "tagLength");
	assert.sameValue(toHex(d.decrypt(hex(vec.ct))), vec.pt, `OAEP ${vec.hash} label ${vec.label.length / 2}B decrypt`);
	assert.throws(Error, () => d.decrypt(hex(vec.ct)), "one operation per reset");
	d.reset();
	const out = new Uint8Array(256);
	assert.sameValue(d.decrypt(hex(vec.ct), {output: out}), vec.pt.length / 2, "BYOB count");
	assert.sameValue(toHex(out.subarray(0, vec.pt.length / 2)), vec.pt, "BYOB bytes");
	using e = new Cipher({...options, key: pub, direction: "encrypt"});
	const ct = e.encrypt(hex(vec.pt));
	assert.sameValue(ct.byteLength, 256, "ciphertext is the key size");
	assert.notSameValue(toHex(ct), vec.ct, "OAEP is randomized");
	d.reset();
	assert.sameValue(toHex(d.decrypt(ct)), vec.pt, "round trip");
	// a private key encrypts with its public part
	using e2 = new Cipher({...options, key: priv, direction: "encrypt"});
	d.reset();
	assert.sameValue(toHex(d.decrypt(e2.encrypt(hex(vec.pt)))), vec.pt, "encrypt with private key");
}
// PKCS#1 v1.5
{
	const vec = R.pkcs1v15_encrypt[0];
	using d = new Cipher({algorithm: "RSAES-PKCS1-v1_5", key: priv, direction: "decrypt"});
	assert.sameValue(toHex(d.decrypt(hex(vec.ct))), vec.pt, "v1.5 decrypt");
	using e = new Cipher({algorithm: "RSAES-PKCS1-v1_5", key: pub, direction: "encrypt"});
	const ct = e.encrypt(hex(vec.pt));
	d.reset();
	assert.sameValue(toHex(d.decrypt(ct)), vec.pt, "v1.5 round trip");
	e.reset();
	assert.sameValue(e.encrypt(new Uint8Array(0)).byteLength, 256, "empty message");
	e.reset();
	assert.sameValue(e.encrypt(new Uint8Array(245)).byteLength, 256, "longest message");
	e.reset();
	assert.throws(RangeError, () => e.encrypt(new Uint8Array(246)), "message too long");
}
// failures
{
	using d = new Cipher({algorithm: "RSA-OAEP", key: priv, direction: "decrypt", hash: "SHA-256"});
	const bad = hex(R.oaep[1].ct); bad[200] ^= 1;
	assert.throws(Error, () => d.decrypt(bad), "tampered ciphertext");
	d.reset();
	assert.throws(RangeError, () => d.decrypt(hex(R.oaep[1].ct).subarray(1)), "wrong ciphertext length");
	d.reset();
	using wrongHash = new Cipher({algorithm: "RSA-OAEP", key: priv, direction: "decrypt", hash: "SHA-1"});
	assert.throws(Error, () => wrongHash.decrypt(hex(R.oaep[1].ct)), "wrong hash");
	if (Cipher.supports({algorithm: "RSA-OAEP", hash: "SHA-256", label: new Uint8Array(3)})) {
		using wrongLabel = new Cipher({algorithm: "RSA-OAEP", key: priv, direction: "decrypt", hash: "SHA-256", label: new Uint8Array(3)});
		assert.throws(Error, () => wrongLabel.decrypt(hex(R.oaep[1].ct)), "wrong label");
	}
	using e = new Cipher({algorithm: "RSA-OAEP", key: pub, direction: "encrypt", hash: "SHA-256"});
	assert.throws(RangeError, () => e.encrypt(new Uint8Array(16), {more: true}), "RSA does not stream");
	assert.throws(RangeError, () => e.encrypt(new Uint8Array(191)), "OAEP message too long");
	assert.throws(RangeError, () => e.encrypt(new Uint8Array(16), {output: new Uint8Array(255)}), "output too small");
}

function throwsAnything(f, message) {
	try { f(); } catch { return; }
	throw new Test262Error(`${message}: did not throw`);
}

// construction errors
assert.throws(Error, () => new Cipher({algorithm: "RSA-OAEP", key: pub, direction: "decrypt", hash: "SHA-256"}), "decrypt with a public key");
assert.throws(TypeError, () => new Cipher({algorithm: "RSA-OAEP", key: hex(R.key.spki), direction: "encrypt", hash: "SHA-256"}), "key must be a Key");
assert.throws(RangeError, () => new Cipher({algorithm: "RSA-OAEP", key: pub, direction: "encrypt"}), "OAEP hash required");
assert.throws(RangeError, () => new Cipher({algorithm: "RSA-OAEP", key: pub, direction: "encrypt", hash: "SHA-256", iv: new Uint8Array(16)}), "iv not supported");
assert.throws(RangeError, () => new Cipher({algorithm: "RSAES-PKCS1-v1_5", key: pub, direction: "encrypt", hash: "SHA-256"}), "v1.5 has no hash");
assert.throws(RangeError, () => new Cipher({algorithm: "RSA-OAEP", key: pub, direction: "both", hash: "SHA-256"}), "bad direction");
assert.throws(Error, () => new Cipher({algorithm: "RSA-XYZ", key: pub, direction: "encrypt"}), "unknown RSA algorithm");
{
	const c = new Cipher({algorithm: "RSA-OAEP", key: pub, direction: "encrypt", hash: "SHA-256"});
	c.close();
	c.close();
	// the error class after close depends on the backend: Error from the script implementation, SyntaxError from the host data validator
	throwsAnything(() => c.encrypt(new Uint8Array(4)), "encrypt after close");
	throwsAnything(() => c.blockSize, "blockSize after close");
}
{
	const k = publicKey();
	const c = new Cipher({algorithm: "RSA-OAEP", key: k, direction: "encrypt", hash: "SHA-256"});
	k.close();
	assert.throws(Error, () => c.encrypt(new Uint8Array(4)), "key closed under the cipher");
	c.close();
}
