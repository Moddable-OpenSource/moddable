/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content(null, { left: 0, top: 0, height: 150, width: 150, skin: new Skin({ fill: "#ff000077"}) });
const two = new Content(null, { left: 0, top: 0, height: 150, width: 150, skin: new Skin({ fill: "#ff000077"}) });

const container1 = new Row(null, { 
	top: 0, left: 0, height: 100, width: 100,
	skin: new Skin({ fill: "black"}),
	contents: [ one ]
});

const container2 = new Row(null, { 
	top: 200, left: 0, height: 100, width: 100, clip: true,
	skin: new Skin({ fill: "black"}),
	contents: [ two ]
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ container1, container2 ]
});

screen.checkImage("ee416a0eadac3f5c7c3c851d422ea215");

one.height = 50;
one.width = 50;
screen.checkImage("dd9e60f20f3e926b5a39d61c980b5b5b");

one.height = 150;
one.width = 150;
screen.checkImage("5156343c6bea3b775568b90dcb9ac4e1");