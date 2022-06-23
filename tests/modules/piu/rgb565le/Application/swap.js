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

screen.checkImage("550ec4111f6e15b7cc0952039bfa581a");

application.swap(one, three);
screen.checkImage("cbf8af0968663edb065f00d55e5bc409");

application.swap(two, one);
screen.checkImage("36835b37f1e12930182c5bbfd684cfe1");

application.swap(two, three);
screen.checkImage("b8dfb300f5fa37e3d70d82bfdf87752f");