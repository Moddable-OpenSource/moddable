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

screen.checkImage("37abed92b6492496943ea1c3b39713e8");

sampleImage.frameIndex = sampleImage.frameIndex + 1;
screen.checkImage(undefined);
