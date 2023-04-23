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

screen.checkImage("ceaea8935eab9131dda6804c99d97563");

row.swap(redSquare, blueSquare);
screen.checkImage("878cfe26dcf61758cb1d9ae13525c399");

row.swap(redSquare, yellowSquare);
screen.checkImage("65a5bfe2a09f00eca76266b2a6888ec1");
