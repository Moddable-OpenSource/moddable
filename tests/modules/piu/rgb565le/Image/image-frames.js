/*---
description: 
flags: [onlyStrict, async]
---*/

const frames = [
	"53f15f781ccfc270bf763dd1591fca4d", 
	"ac2d8df34ddc656ada0de8756f9bbef2", 
	"7b69c17e812bd64c6d48c6ea84d13719", 
	"ac2d8df34ddc656ada0de8756f9bbef2", 
	"7b69c17e812bd64c6d48c6ea84d13719"
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
