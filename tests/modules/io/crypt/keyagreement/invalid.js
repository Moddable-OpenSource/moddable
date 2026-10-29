/*---
description: KeyAgreement rejects wrong algorithms, keys and arguments
flags: [module]
---*/

import KeyAgreement, {Key, V, hex} from "./keyagreement_FIXTURE.js";

function throwsAnything(f, message) {
	try { f(); } catch { return; }
	throw new Test262Error(`${message}: did not throw`);
}

using priv = new Key({algorithm: "EC", format: "pkcs8", data: hex(V["P-256"].pkcs8)});
using pub = new Key({algorithm: "EC", format: "spki", data: hex(V["P-256"].spki)});
assert.throws(Error, () => new KeyAgreement({algorithm: "DH", key: priv}), "unsupported algorithm");
assert.throws(TypeError, () => new KeyAgreement({algorithm: "ECDH", key: hex(V["P-256"].pkcs8)}), "key must be a Key");
assert.throws(Error, () => new KeyAgreement({algorithm: "ECDH", key: pub}), "private key required");
assert.throws(TypeError, () => new KeyAgreement({algorithm: "ECDH"}), "key missing");
{
	const agreement = new KeyAgreement({algorithm: "ECDH", key: priv});
	assert.throws(TypeError, () => agreement.derive(hex(V["P-256"].raw)), "publicKey must be a Key");
	agreement.close();
	agreement.close();
	throwsAnything(() => agreement.derive(pub), "derive after close");		// Error from script, SyntaxError from the host data validator
}
{
	const k = new Key({algorithm: "EC", format: "pkcs8", data: hex(V["P-256"].pkcs8)});
	const agreement = new KeyAgreement({algorithm: "ECDH", key: k});
	k.close();
	assert.throws(Error, () => agreement.derive(pub), "key closed under the agreement");
}
