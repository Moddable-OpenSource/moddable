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
const verticalSemiCircleSkin = new Skin({
	texture: circleTexture,
	color: ["blue", "red"],
	height: 40, width: 20,
	variants: 20
});
const horizontalSemiCircleSkin = new Skin({
	texture: circleTexture,
	color: ["blue", "red"],
	height: 20, width: 40,
	states: 20
});

const samplePort = new Port(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	Behavior: class extends Behavior {
		onDraw(port) {
			port.drawSkin(circleSkin, 0, 0, 40, 40);
			port.drawSkin(verticalSemiCircleSkin, 0, 40, 20, 40, 0, 0);
			port.drawSkin(verticalSemiCircleSkin, 20, 40, 20, 40, 1, 1);
			port.drawSkin(horizontalSemiCircleSkin, 0, 80, 40, 20, 0, 0);
			port.drawSkin(horizontalSemiCircleSkin, 0, 100, 40, 20, 1, 1);
		}
	}
})

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ samplePort ]
});

screen.checkImage("14ef03b5881263382997cd3f17528199");