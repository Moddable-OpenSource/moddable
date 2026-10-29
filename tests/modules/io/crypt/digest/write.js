/*---
description:
flags: [module]
---*/

import Digest, {KAT} from "./digest_FIXTURE.js";
import TextEncoder from "text/encoder";

const enc = new TextEncoder();

// empty write is a no-op: hash matches "no writes at all"
{
	const a = new Digest({algorithm: "SHA-256"});
	a.write(new ArrayBuffer(0));
	const ha = new Uint8Array(a.read()).toHex();
	a.close();

	const b = new Digest({algorithm: "SHA-256"});
	const hb = new Uint8Array(b.read()).toHex();
	b.close();

	assert.sameValue(ha, hb, "empty write produces same hash as no write");
	assert.sameValue(ha, KAT["SHA-256"][0].out, "matches SHA-256 empty KAT");
}

// N partial writes equal a single concatenated write
{
	const chunks = [enc.encode("The quick brown fox "), enc.encode("jumps over "), enc.encode("the lazy dog")];
	const whole = enc.encode("The quick brown fox jumps over the lazy dog");

	const partial = new Digest({algorithm: "SHA-256"});
	for (const c of chunks) partial.write(c);
	const hp = new Uint8Array(partial.read()).toHex();
	partial.close();

	const single = new Digest({algorithm: "SHA-256"});
	single.write(whole);
	const hs = new Uint8Array(single.read()).toHex();
	single.close();

	assert.sameValue(hp, hs, "concatenated writes equal single write");
	assert.sameValue(hp, KAT["SHA-256"][2].out, "matches SHA-256 QBF KAT");
}

// various buffer types are all accepted as input
{
	const expected = KAT["SHA-256"][1].out;	// "abc"
	const abcBytes = enc.encode("abc");
	const abcBuffer = abcBytes.buffer.slice(abcBytes.byteOffset, abcBytes.byteOffset + abcBytes.byteLength);

	const cases = [
		["Uint8Array",        abcBytes],
		["ArrayBuffer",       abcBuffer],
		["DataView",          new DataView(abcBuffer)],
		["Uint8ClampedArray", new Uint8ClampedArray(abcBuffer)],
		["Int8Array",         new Int8Array(abcBuffer)],
		["SharedArrayBuffer", (() => { const s = new SharedArrayBuffer(3); new Uint8Array(s).set(abcBytes); return s; })()],
	];

	for (const [label, buf] of cases) {
		const d = new Digest({algorithm: "SHA-256"});
		d.write(buf);
		const h = new Uint8Array(d.read()).toHex();
		d.close();
		assert.sameValue(h, expected, `${label}: SHA-256('abc') matches`);
	}
}
