/*---
description: 
flags: [onlyStrict]
---*/

const redSquare = new Content(null, {
	top: 10, bottom: 10, left: 10, right: 10,
	skin: new Skin({ fill: "red" })
});

const container = new Container(null, {
	top: 0, height: 100, left: 0, width: 100,
	skin: new Skin({ fill: "white" }),
	contents: [ redSquare ]
});

new Application(null, { skin: new Skin({ fill: "black" }), contents: [container] });

screen.checkImage("76506edefdb9e9afba013993c91f8bae");

container.moveBy(100, 100);
screen.checkImage("123c5ffded1740123264e7741051741e");

container.moveBy(-150, -150);
screen.checkImage("3b1d45dc86e5ef14ac03589cd9be7e90");