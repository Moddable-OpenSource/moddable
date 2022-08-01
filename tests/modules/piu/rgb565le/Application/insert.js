/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "red"}) });
const two = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "yellow"}) });
const three = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "blue"}) });

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ three ]
});

screen.checkImage("550ec4111f6e15b7cc0952039bfa581a");

application.insert(one, three);
screen.checkImage("b8dfb300f5fa37e3d70d82bfdf87752f");

application.insert(two, three);
screen.checkImage("b8dfb300f5fa37e3d70d82bfdf87752f");