/*---
description: 
flags: [onlyStrict]
---*/

const circleTexture = new Texture({ path: "circleish.png" });

const samplePort = new Port(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	Behavior: class extends Behavior {
		onDraw(port) {
			port.drawTexture(circleTexture, "red", 0, 0, 0, 0, 40, 40);
			port.drawTexture(circleTexture, "yellow", 0, 40, 0, 0, 20, 40);
			port.drawTexture(circleTexture, "blue", 20, 40, 20, 0, 20, 40);
		}
	}
})

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ samplePort ]
});

screen.checkImage("365b479517f97a006608e68f5df88307");