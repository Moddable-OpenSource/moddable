/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "red"}) });
const two = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "yellow"}) });
const three = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "blue"}) });

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [one, two, three]
});

screen.checkImage("8e4f7bde403e94148a35744e08b4b123");

application.swap(one, three);
screen.checkImage("905092ce216e848a371e7236e4da8457");

application.swap(two, one);
screen.checkImage("f7e724fc1664cc89e258d6b94b358e54");

application.swap(two, three);
screen.checkImage("b9e1308f00f21ea2513a6d08e5768196");