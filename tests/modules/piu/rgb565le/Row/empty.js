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
screen.checkImage("5dfe5845794e15e5102a364354403c77");

container.empty();
screen.checkImage("810dbe035803d397a144531c4667c225");