/*---
description: 
flags: [onlyStrict, async]
---*/

const frames = [
	"223a16a534f0b594862bf4520fb90088",
	"7b8ed64d8de598bab8f76bacbe2563f0",
	"ea1169622be6b818971ccd262e14ad9e",
	undefined
];

class ImageBehavior extends Behavior {
	onDisplaying(image) {
		let index = 0;
		while (index < frames.length) {
			image.frameIndex = index;
			screen.checkImage(frames[index]);
			index++;
		}
		$DONE();
	}
}

const sampleImage = new Image(null, ({
	path: "color-dots-20.cs",
	Behavior: ImageBehavior
}));

new Application(null, {
	skin: new Skin({ fill: "black" }),
	contents: [ sampleImage ]
});

$TESTMC.timeout(500, "`onDisplaying` should have been triggered");