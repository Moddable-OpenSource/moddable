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

screen.checkImage("34287f8b33eb29cf19a5118004c8dde1");

container.sizeBy(-100, -100);
screen.checkImage("da0c91c0d5c38d6bf33c6c595ef7e934");

container.sizeBy(50, 50);
screen.checkImage("e11a2a5011b3bcce8f87b6a7bf8e5da6");