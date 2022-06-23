/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "red"}) });
const two = new Content(null, { left: 0, top: 50, height: 50, width: 50, skin: new Skin({ fill: "yellow"}) });
const three = new Content(null, { left: 0, top: 0, height: 50, width: 50, skin: new Skin({ fill: "blue"}) });
const four = new Content(null, { left: 0, top: 50, height: 50, width: 50, skin: new Skin({ fill: "green"}) });

const container = new Die(null, { 
	top: 0, bottom: 0, left: 0, right: 0,
	contents: [one, two],
	Behavior: class extends Behavior {
		onDisplaying(die) {
			die.fill()
				.cut();
		}
	}
});

new Application(null, {
	skin: new Skin({ fill: "white" }),
	contents: [ container ]
});

screen.checkImage("51c9578efc445ab1ad1024a340fb827f");

container.replace(one, three);
screen.checkImage("b8dfb300f5fa37e3d70d82bfdf87752f");

container.replace(two, four);
screen.checkImage("216a84f4d014a62d15a85f82ae9a90c9");

container.replace(three, one);
screen.checkImage("cbf8af0968663edb065f00d55e5bc409");
