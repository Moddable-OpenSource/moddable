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

const col = new Column(null, {
	top: 0, bottom: 0, left: 0, right: 0,
	skin: new Skin({ fill: "white" })
});

new Application(null, { contents: [col] });

col.add(redSquare);
col.add(yellowSquare);
col.add(blueSquare);
screen.checkImage("279bb31cdc6cfe54ca3022e7da2c781e");

col.remove(yellowSquare);
screen.checkImage("3ef1278cb21216b0db4b98bd7a151b6c");

col.remove(blueSquare);
screen.checkImage("8d615e72ef34da297ccb1581fcc463da");

col.remove(redSquare);
screen.checkImage("14e6edc6291901a47e0268a966b4d99e");

col.add(yellowSquare);
col.add(blueSquare);
col.add(redSquare);
screen.checkImage("de0701ab33d7ff2121dc69116d7d5fea");