/*---
description:
flags: [async, module]
---*/

import {subtle, hex, toHex, assertRejects} from "./webcrypto_FIXTURE.js";
import TextEncoder from "text/encoder";

const abc = new TextEncoder().encode("abc");
const expected = {
	"SHA-1":   "a9993e364706816aba3e25717850c26c9cd0d89d",
	"SHA-256": "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
	"SHA-384": "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7",
	"SHA-512": "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f",
};

(async () => {
	for (const [name, digest] of Object.entries(expected)) {
		assert.sameValue(toHex(await subtle.digest(name, abc)), digest, `${name} string form`);
		assert.sameValue(toHex(await subtle.digest({name}, abc)), digest, `${name} object form`);
		assert.sameValue(toHex(await subtle.digest(name.toLowerCase(), abc.buffer)), digest, `${name} case-insensitive, ArrayBuffer`);
	}
	assert.sameValue(toHex(await subtle.digest("SHA-256", new Uint8Array(0))), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "empty");
	const view = new DataView(abc.buffer, 1, 1);		// "b"
	assert.sameValue(toHex(await subtle.digest("SHA-256", view)), "3e23e8160039594a33894f6564e1b1348bbd7a0088d42c4acb73eeaed59c009d", "DataView respects offset");
	assert.sameValue((await subtle.digest("SHA-256", abc)) instanceof ArrayBuffer, true, "returns ArrayBuffer");

	await assertRejects(subtle.digest("MD5", abc), "NotSupportedError", "MD5");
	await assertRejects(subtle.digest({}, abc), TypeError, "no name");
	await assertRejects(subtle.digest("SHA-256", "abc"), TypeError, "string data");
})().then(() => $DONE(), $DONE);
