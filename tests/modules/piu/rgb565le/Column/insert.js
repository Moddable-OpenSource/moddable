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

screen.checkImage("1cbfdd2ce0bb314aa20b23d7e0b2c0f7");

container.insert(one, three);
screen.checkImage("f3749dc5dc3cf46b30c819eba653fc5a");

container.insert(two, three);
screen.checkImage("9fc0550136fcd30cd91b0b200f2cd63e");