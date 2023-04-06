/*---
description: 
flags: [onlyStrict]
---*/

const circleTexture = new Texture({ path: "circleish.png" });
const circleSkin = new Skin({
	texture: circleTexture,
	color: "red",
	height: 40, width: 40
});

const samplePort = new Port(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	skin: circleSkin,
	Behavior: class extends Behavior {
		onDraw(port) {
			port.drawContent(0, 0, circleSkin.width, circleSkin.height);
			port.drawContent(circleSkin.width, circleSkin.height, circleSkin.width, circleSkin.height);
			port.drawContent(circleSkin.width << 1, circleSkin.height << 1, circleSkin.width, circleSkin.height);
		}
	}
})

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ samplePort ]
});

screen.checkImage("25f3827ae055b8955716b4d0504c5e3a");
