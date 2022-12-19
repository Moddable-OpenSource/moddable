/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content(null, { left: 0, top: 0, height: 100, width: 100, skin: new Skin({ fill: "red"}) });
const two = new Content(null, { left: 0, top: 100, height: 100, width: 100, skin: new Skin({ fill: "yellow"}) });
const three = new Content(null, { left: 0, top: 200, height: 100, width: 100, skin: new Skin({ fill: "blue"}) });

const die = new Die(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	contents: [one, two]
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ die, three ]
});
die.fill()
	.cut();
screen.checkImage("1a9c67bcbd5c7b3b7c5391222eddbfe6");

die.empty()
	.cut();
screen.checkImage("1e3612c0f8bbcec51213298b8905e34e");

die.empty()
	.set(25, 25, 50, 100)
	.cut();
screen.checkImage("85926e0f4ee82122fa67c27f8b98bcfa");
