/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "red"}) });
const two = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "yellow"}) });
const three = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "blue"}) });

const container = new Column(null, { 
	contents: [three]
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ container ]
});

screen.checkImage("efadef3a732f4534bcd45e3c3bfeffad");

container.insert(one, three);
screen.checkImage("0eebdd5f792d601c302c9d8485ab1ad9");

container.insert(two, three);
screen.checkImage("0851e0be512f5db0e76850118011f6fa");