/*---
description: 
flags: [onlyStrict]
---*/

const sampleImage = new Image(null, ({
	path: "screen2.cs"
}));

assert.sameValue(sampleImage.frameCount, 1, "`frameCount` should be 1");

assert.sameValue(sampleImage.frameIndex, 0, "`frameIndex` should be 0 by default");

sampleImage.frameIndex = 100;
assert.sameValue(sampleImage.frameIndex, 0, "Cannot set `frameIndex` to be greater than `frameCount`-1; `frameIndex` should be 0");