/*---
description:
flags: [module]
---*/

import {KAT, digestOf} from "./digest_FIXTURE.js";

for (const algorithm of ["SHA-1", "SHA-224", "SHA-256", "SHA-384", "SHA-512"]) {
	for (const {in: input, out} of KAT[algorithm]) {
		assert.sameValue(digestOf({algorithm}, input), out, `${algorithm}(${input.byteLength}B)`);
	}
}
