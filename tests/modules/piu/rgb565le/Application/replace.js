/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "red"}) });
const two = new Content(null, { left: 0, top: 50, height: 50, width: 50, skin: new Skin({ fill: "yellow"}) });
const three = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "blue"}) });
const four = new Content(null, { left: 0, top: 50, height: 50, width: 50, skin: new Skin({ fill: "green"}) });

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ one, two ]
});

screen.checkImage("51c9578efc445ab1ad1024a340fb827f");

application.replace(one, three);
screen.checkImage("b8dfb300f5fa37e3d70d82bfdf87752f");

application.replace(two, four);
screen.checkImage("216a84f4d014a62d15a85f82ae9a90c9");

application.replace(three, one);
screen.checkImage("cbf8af0968663edb065f00d55e5bc409");
