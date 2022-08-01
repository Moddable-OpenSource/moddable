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

screen.checkImage("1cbfdd2ce0bb314aa20b23d7e0b2c0f7");

container.insert(one, three);
screen.checkImage("53cfd1a7c064e964b72cc023428a7a0b");

container.insert(two, three);
screen.checkImage("53cfd1a7c064e964b72cc023428a7a0b");