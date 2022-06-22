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

screen.checkImage("2d76c161ab4f559fb373ac668780fe5f");