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

const row = new Row(null, {
	top: 0, height: 320, left: 0, width: 240,
	skin: new Skin({ fill: "white" }),
	contents: [ redSquare, yellowSquare, blueSquare ]
});

new Application(null, { skin: new Skin({ fill: "black" }), contents: [row] });

screen.checkImage("e76be9a0ba276d134362d7b7c529325d");

row.sizeBy(-100, -100);
screen.checkImage("b37d7813528d7d2c8ca30a15187d4e41");

row.sizeBy(50, 50);
screen.checkImage("fbae6bf8930cd55a153ef0bba08ac9ee");