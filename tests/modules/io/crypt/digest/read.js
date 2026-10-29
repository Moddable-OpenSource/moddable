/*---
description:
flags: [module]
---*/

import Digest, {KAT} from "./digest_FIXTURE.js";
import TextEncoder from "text/encoder";

const enc = new TextEncoder();
const expectAbc  = KAT["SHA-256"][1].out;
const expectQbf  = KAT["SHA-256"][2].out;

// no argument: allocates a fresh ArrayBuffer of outputSize
{
	const d = new Digest({algorithm: "SHA-256"});
	d.write(enc.encode("abc"));
	const r = d.read();
	assert.sameValue(r instanceof ArrayBuffer, true, "read() returns ArrayBuffer");
	assert.sameValue(r.byteLength, 32, "read() result has outputSize bytes");
	assert.sameValue(new Uint8Array(r).toHex(), expectAbc, "SHA-256('abc')");
	d.close();
}

// supplied ArrayBuffer: filled in place, returns outputSize
{
	const d = new Digest({algorithm: "SHA-256"});
	d.write(enc.encode("abc"));
	const buf = new ArrayBuffer(32);
	const n = d.read(buf);
	assert.sameValue(n, 32, "read(buf) returns count");
	assert.sameValue(new Uint8Array(buf).toHex(), expectAbc, "buf filled in place");
	d.close();
}

// supplied Uint8Array: also accepted
{
	const d = new Digest({algorithm: "SHA-256"});
	d.write(enc.encode("abc"));
	const buf = new Uint8Array(32);
	const n = d.read(buf);
	assert.sameValue(n, 32, "read(Uint8Array) returns count");
	assert.sameValue(buf.toHex(), expectAbc, "Uint8Array filled in place");
	d.close();
}

// larger-than-needed buffer is fine; only outputSize bytes written
{
	const d = new Digest({algorithm: "SHA-256"});
	d.write(enc.encode("abc"));
	const buf = new Uint8Array(64).fill(0xCC);
	const n = d.read(buf);
	assert.sameValue(n, 32, "oversized buffer: returns outputSize");
	assert.sameValue(buf.subarray(0, 32).toHex(), expectAbc, "first outputSize bytes filled");
	assert.sameValue(buf[32], 0xCC, "byte after outputSize untouched");
	d.close();
}

// non-destructive: read twice yields identical result
{
	const d = new Digest({algorithm: "SHA-256"});
	d.write(enc.encode("abc"));
	const r1 = new Uint8Array(d.read()).toHex();
	const r2 = new Uint8Array(d.read()).toHex();
	assert.sameValue(r1, r2, "two reads in a row are identical");
	assert.sameValue(r1, expectAbc, "matches SHA-256('abc')");
	d.close();
}

// snapshot semantics: write after read continues the hash
{
	const d = new Digest({algorithm: "SHA-256"});
	d.write(enc.encode("The quick brown fox "));
	d.read();	// snapshot, discarded
	d.write(enc.encode("jumps over "));
	d.read();	// snapshot, discarded
	d.write(enc.encode("the lazy dog"));
	const final = new Uint8Array(d.read()).toHex();
	assert.sameValue(final, expectQbf, "write after read continues hashing");
	d.close();
}
