/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "red"}) });
const two = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "yellow"}) });
const three = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "blue"}) });

const container = new Container(null, { 
	contents: [three]
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ container ]
});

screen.checkImage("efadef3a732f4534bcd45e3c3bfeffad");

container.insert(one, three);
screen.checkImage("93dfe2e1d2d85b88b8d28a902a1cd8e0");

container.insert(two, three);
screen.checkImage("93dfe2e1d2d85b88b8d28a902a1cd8e0");