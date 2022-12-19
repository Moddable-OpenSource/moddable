/*---
description: 
flags: [onlyStrict]
---*/

const Square = Content.template($ => ({
	top: 10, left: 10, width: 50, height: 50,
	skin: new Skin({ fill: $ })
}));

const redSquare = new Square("red");
const yellowSquare = new Square("yellow");
const blueSquare = new Square("blue");

const row = new Row(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	skin: new Skin({ fill: "white" }),
	contents: [ redSquare, yellowSquare, blueSquare ]
});

new Application(null, { contents: [row] });

screen.checkImage("09e9d41110204beebbb04f83a1f4c4d8");

row.swap(redSquare, blueSquare);
screen.checkImage("b656b586ab51fac830b3f0f17a1cd9d8");

row.swap(redSquare, yellowSquare);
screen.checkImage("163c8c4a00e54df0d0d65c21132bced6");
