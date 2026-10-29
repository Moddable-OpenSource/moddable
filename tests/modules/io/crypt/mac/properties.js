/*---
description: MAC instance properties, non-destructive read, reset, close
flags: [module]
---*/

import MAC, {Hashes, hex, toHex} from "./mac_FIXTURE.js";

const key = hex("0b".repeat(20));
for (const {hash, blockSize, outputSize} of Hashes) {
	const mac = new MAC({algorithm: "HMAC", hash, key});
	assert.sameValue(mac.algorithm, "HMAC", "algorithm");
	assert.sameValue(mac.hash, hash, "hash");
	assert.sameValue(mac.blockSize, blockSize, `${hash} blockSize`);
	assert.sameValue(mac.outputSize, outputSize, `${hash} outputSize`);
	assert.sameValue(mac.read().byteLength, outputSize, "tag length");

	const empty = toHex(mac.read());
	mac.write(hex("48692054686572"));
	const first = toHex(mac.read());
	assert.sameValue(toHex(mac.read()), first, "read is non-destructive");
	assert.notSameValue(first, empty, "the message changes the tag");
	mac.write(hex("65"));
	assert.notSameValue(toHex(mac.read()), first, "write after read continues the message");
	mac.reset();
	assert.sameValue(toHex(mac.read()), empty, "reset restores the keyed state");
	mac.close();
	mac.close();		// a second close is harmless
	assert.throws(SyntaxError, () => mac.write(key), "write after close");
	assert.throws(SyntaxError, () => mac.read(), "read after close");
}

// the key may be empty or longer than the block
assert.sameValue(toHex(new MAC({algorithm: "HMAC", hash: "SHA-256", key: new Uint8Array(0)}).read()), "b613679a0814d9ec772f95d778c35fc5ff1697c493715653c6c712144292c5ad", "empty key");
