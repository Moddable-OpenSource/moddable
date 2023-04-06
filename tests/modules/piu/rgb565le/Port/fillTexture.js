/*---
description: 
flags: [onlyStrict]
---*/

const circleTexture = new Texture({ path: "circleish.png" });

const samplePort = new Port(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	Behavior: class extends Behavior {
		onDraw(port) {
			port.fillTexture(circleTexture, "red", 0, 0, 100, 100, 0, 0, 40, 40);
			port.fillTexture(circleTexture, "blue", 0, 100, 100, 100, 0, 0, 40, 20);
		}
	}
})

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ samplePort ]
});

screen.checkImage("88ed353b52f6e6a9370f4506c4f2033e");