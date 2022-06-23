/*---
description: 
flags: [onlyStrict]
---*/

const sampleImage = new Image(null, ({
	path: "screen2.cs"
}));

assert.sameValue(sampleImage.frameCount, 1)

new Application(null, {
	skin: new Skin({ fill: "black" }),
	contents: [ sampleImage ]
});

screen.checkImage("fcd138d7781078338d04d319ae2323e2");

sampleImage.frameIndex = sampleImage.frameIndex + 1;
screen.checkImage(undefined);
