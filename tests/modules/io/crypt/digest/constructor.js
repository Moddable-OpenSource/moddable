/*---
description:
flags: [module]
---*/

import Digest, {algMeta} from "./digest_FIXTURE.js";

for (const {algorithm} of algMeta) {
	const d = new Digest({algorithm});
	assert.sameValue(typeof d, "object", `${algorithm}: constructor returned non-object`);
	d.close();
}
