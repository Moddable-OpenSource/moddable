/*---
description: 
flags: [onlyStrict]
---*/

const Square = Content.template($ => ({
	top: 10, bottom: 10, left: 10, right: 10,
	skin: new Skin({ fill: $ })
}));

const redSquare = new Square("red");
const yellowSquare = new Square("yellow");
const blueSquare = new Square("blue");

const col = new Column(null, {
	top: 0, height: 320, left: 0, width: 240,
	skin: new Skin({ fill: "white" }),
	contents: [ redSquare, yellowSquare, blueSquare ]
});

new Application(null, { skin: new Skin({ fill: "black" }), contents: [col] });

screen.checkImage("34d071ae70f3b1e3451f5bf496da42f6");

col.sizeBy(-100, -100);
screen.checkImage("d2832168ee83cc9013fe1b50531d4a8a");

col.sizeBy(50, 50);
screen.checkImage("fafce0c97cce20ff0f00233969149af1");