/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content(null, { left: 0, top: 0, height: 100, width: 100, skin: new Skin({ fill: "red"}) });
const two = new Content(null, { left: 0, top: 100, height: 100, width: 100, skin: new Skin({ fill: "yellow"}) });
const three = new Content(null, { left: 0, top: 200, height: 100, width: 100, skin: new Skin({ fill: "blue"}) });

const container = new Container(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	contents: [one, two]
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ container, three ]
});
screen.checkImage("4ccc01803b9dcd9f9f0da37617f9cbf0");

application.empty();
screen.checkImage("f29d3d7c7bace5943c2e44d85267b5d3");