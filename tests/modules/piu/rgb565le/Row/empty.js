/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content(null, { left: 0, top: 0, height: 100, width: 100, skin: new Skin({ fill: "red"}) });
const two = new Content(null, { left: 0, top: 0, height: 100, width: 100, skin: new Skin({ fill: "yellow"}) });
const three = new Content(null, { left: 0, top: 200, height: 100, width: 100, skin: new Skin({ fill: "blue"}) });

const container = new Row(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	contents: [one, two]
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ container, three ]
});
screen.checkImage("bfe999ab97de176c4ea13d4ccd177a2c");

container.empty();
screen.checkImage("1e3612c0f8bbcec51213298b8905e34e");