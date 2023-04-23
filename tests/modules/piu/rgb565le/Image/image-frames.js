/*---
description: 
flags: [onlyStrict, async]
---*/

const frames = [
	"547e1d56c7cdce05e785a78c467e0e12", 
	"99d8540b8c60bb85d72676065f8ec4e6", 
	"b2aa45b90faf0cd747314b4608b07b1b", 
	"99d8540b8c60bb85d72676065f8ec4e6", 
	"b2aa45b90faf0cd747314b4608b07b1b"
];

class ImageBehavior extends Behavior {
	onDisplaying(image) {
		for (let frameIndex = 0; frameIndex < frames.length; frameIndex++) {
			image.frameIndex = frameIndex;
			screen.checkImage(frames[frameIndex]);
		}
		$DONE();
	}
}

const sampleImage = new Image(null, ({
	path: "robby.cs",
	Behavior: ImageBehavior
}));

assert.sameValue(sampleImage.frameCount, frames.length)

new Application(null, {
	skin: new Skin({ fill: "black" }),
	contents: [ sampleImage ]
});

$TESTMC.timeout(500, "`onDisplaying` should have been triggered");
