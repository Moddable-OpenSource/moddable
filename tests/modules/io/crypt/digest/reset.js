/*---
description:
flags: [module]
---*/

import Digest, {algMeta} from "./digest_FIXTURE.js";
import TextEncoder from "text/encoder";

const enc = new TextEncoder();

// for each algorithm: reset rewinds to fresh-construct state
for (const {algorithm, blockSize, outputSize} of algMeta) {
	const d = new Digest({algorithm});
	d.write(enc.encode("polluting state with garbage data"));
	d.read();	// take a snapshot too; should not matter
	d.reset();

	d.write(enc.encode("abc"));
	const afterReset = new Uint8Array(d.read()).toHex();

	const fresh = new Digest({algorithm});
	fresh.write(enc.encode("abc"));
	const freshHash = new Uint8Array(fresh.read()).toHex();
	fresh.close();

	assert.sameValue(afterReset, freshHash, `${algorithm}: reset matches fresh`);
	assert.sameValue(d.blockSize, blockSize, `${algorithm}: blockSize unchanged after reset`);
	assert.sameValue(d.outputSize, outputSize, `${algorithm}: outputSize unchanged after reset`);
	d.close();
}
