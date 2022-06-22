/*---
description: 
flags: [onlyStrict]
---*/

const redSquare = new Content(null, {
	top: 10, bottom: 10, left: 10, right: 10,
	skin: new Skin({ fill: "red" })
});

const container = new Layout(null, {
	top: 0, height: 320, left: 0, width: 240,
	skin: new Skin({ fill: "white" }),
	contents: [ redSquare ]
});

new Application(null, { skin: new Skin({ fill: "black" }), contents: [container] });

screen.checkImage("bb5179635b6d1df9d3ffa7cc862c7ab8");

container.sizeBy(-100, -100);
screen.checkImage("972b0fd389d964c0abaf609592213d92");

container.sizeBy(50, 50);
screen.checkImage("b2b7511aaf9b6087834cfa730633de82");