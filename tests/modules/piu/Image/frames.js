/*---
description: 
flags: [onlyStrict]
---*/

const sampleImage = new Image(null, ({
	path: "robby.cs"
}));

assert.sameValue(sampleImage.frameCount, 5, "`frameCount` should be 5");

assert.sameValue(sampleImage.frameIndex, 0, "`frameIndex` should be 0 by default");

sampleImage.frameIndex = 4;
assert.sameValue(sampleImage.frameIndex, 4, "`frameIndex` should be 4");

sampleImage.frameIndex = 100;
assert.sameValue(sampleImage.frameIndex, 4, "Cannot set `frameIndex` to be greater than `frameCount`-1; `frameIndex` should be 4 ");