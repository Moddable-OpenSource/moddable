/*---
description: 
flags: [onlyStrict]
---*/

const redSquare = new Content(null, {
	top: 10, bottom: 10, left: 10, right: 10,
	skin: new Skin({ fill: "red" })
});

const container = new Layout(null, {
	top: 0, height: 100, left: 0, width: 100,
	skin: new Skin({ fill: "white" }),
	contents: [ redSquare ]
});

new Application(null, { skin: new Skin({ fill: "black" }), contents: [container] });

screen.checkImage("e2efe63d5e2d6a939e5c41ad94640ac4");

container.moveBy(100, 100);
screen.checkImage("1ebff34e88f78dd508efff6e416dc997");

container.moveBy(-150, -150);
screen.checkImage("6d25d11af9cfe4ddec23ae3a870836f6");