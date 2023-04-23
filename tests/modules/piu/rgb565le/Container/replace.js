/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "red"}) });
const two = new Content(null, { left: 0, top: 50, height: 50, width: 50, skin: new Skin({ fill: "yellow"}) });
const three = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "blue"}) });
const four = new Content(null, { left: 0, top: 50, height: 50, width: 50, skin: new Skin({ fill: "green"}) });

const container = new Container(null, { 
	top: 0, bottom: 0, left: 0, right: 0,
	contents: [one, two]
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ container ]
});

screen.checkImage("e38aee4b2b12a786f0b9710e7b083fc9");

container.replace(one, three);
screen.checkImage("b9e1308f00f21ea2513a6d08e5768196");

container.replace(two, four);
screen.checkImage("2a1460d1c068a0f8d39a465e487543b0");

container.replace(three, one);
screen.checkImage("905092ce216e848a371e7236e4da8457");
