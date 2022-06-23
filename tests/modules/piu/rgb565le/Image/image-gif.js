/*---
description: 
flags: [onlyStrict, async]
---*/

const frames = [
	"621d95f6ef8faf5c5cd3b512c8eeddde",
	"be2002ecc9b7f0dfd6ec82aba73997e3",
	"19a3f332eed85190b7b05adb069df9f2",
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