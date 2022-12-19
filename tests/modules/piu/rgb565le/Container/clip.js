/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content(null, { left: 0, top: 0, height: 150, width: 150, skin: new Skin({ fill: "#ff000077"}) });
const two = new Content(null, { left: 0, top: 0, height: 150, width: 150, skin: new Skin({ fill: "#ff000077"}) });

const container1 = new Container(null, { 
	top: 0, left: 0, height: 100, width: 100,
	skin: new Skin({ fill: "black"}),
	contents: [ one ]
});

const container2 = new Container(null, { 
	top: 200, left: 0, height: 100, width: 100, clip: true,
	skin: new Skin({ fill: "black"}),
	contents: [ two ]
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ container1, container2 ]
});

screen.checkImage("b9defe4cbba8dab805368c13de4a266b");

one.height = 50;
one.width = 50;
screen.checkImage("f1dd85eb20df5f900845e1ae8b79aa25");

one.height = 150;
one.width = 150;
screen.checkImage("4e008cd76aa8c80e480d966a8aa91228");