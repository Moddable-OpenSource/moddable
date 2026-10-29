/*---
description:
flags: [module]
---*/

import Digest, {algMeta} from "./digest_FIXTURE.js";

for (const {algorithm, blockSize, outputSize} of algMeta) {
	const d = new Digest({algorithm});

	assert.sameValue(d.blockSize, blockSize, `${algorithm}: blockSize`);
	assert.sameValue(d.outputSize, outputSize, `${algorithm}: outputSize`);

	// getter-only descriptors: strict-mode assignment throws TypeError
	assert.throws(TypeError, () => { d.blockSize = 0; },  `${algorithm}: blockSize is read-only`);
	assert.throws(TypeError, () => { d.outputSize = 0; }, `${algorithm}: outputSize is read-only`);

	d.close();
}
