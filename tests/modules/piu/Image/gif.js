/*---
description: 
flags: [onlyStrict]
---*/

const sampleImage = new Image(null, ({
	path: "color-dots-20.cs"
}));

assert.sameValue(sampleImage.frameCount, 3, "`frameCount` should be 3");

assert.sameValue(sampleImage.frameIndex, 0, "`frameIndex` should be 0 by default");

sampleImage.frameIndex = 2;
assert.sameValue(sampleImage.frameIndex, 2, "`frameIndex` should be 2");

sampleImage.frameIndex = 100;
assert.sameValue(sampleImage.frameIndex, 2, "Cannot set `frameIndex` to be greater than `frameCount`-1; `frameIndex` should be 2");