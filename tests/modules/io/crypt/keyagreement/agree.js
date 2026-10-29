/*---
description: ECDH shared secrets match Python cryptography on every curve, both directions, and in place
flags: [module]
---*/

import KeyAgreement, {Key, V, hex, toHex} from "./keyagreement_FIXTURE.js";

for (const name of ["P-256", "P-384", "X25519"]) {
	const v = V[name];
	using priv = new Key({algorithm: "EC", format: "pkcs8", data: hex(v.pkcs8)});
	using peer = new Key({algorithm: "EC", format: "spki", data: hex(v.peer_spki)});
	using agreement = new KeyAgreement({algorithm: "ECDH", key: priv});
	assert.sameValue(agreement.algorithm, "ECDH");
	assert.sameValue(toHex(agreement.derive(peer)), v.shared, `${name} shared secret`);
	assert.sameValue(toHex(agreement.derive(peer)), v.shared, `${name} again with the same instance`);
	// generated keys agree with each other
	using a = new Key({algorithm: "EC", namedCurve: name, extractable: true});
	using b = new Key({algorithm: "EC", namedCurve: name, extractable: true});
	using aPublic = new Key({algorithm: "EC", format: "raw", namedCurve: name, data: a.extract("raw")});
	using bPublic = new Key({algorithm: "EC", format: "raw", namedCurve: name, data: b.extract("raw")});
	using fromA = new KeyAgreement({algorithm: "ECDH", key: a});
	using fromB = new KeyAgreement({algorithm: "ECDH", key: b});
	assert.sameValue(toHex(fromA.derive(bPublic)), toHex(fromB.derive(aPublic)), `${name} generated keys agree`);
	assert.throws(RangeError, () => fromA.derive(name === "P-256" ? new Key({algorithm: "EC", format: "spki", data: hex(V["P-384"].spki)}) : new Key({algorithm: "EC", format: "spki", data: hex(V["P-256"].spki)})), "curve mismatch");
}
